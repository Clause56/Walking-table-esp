// Walking table controller
// ESP32 DevKit v1 driving an XY-160D dual H-bridge from a FlySky FS-iA6B
// (SBUS), with a WiFi status page, enable/disable, and manual fallback drive.

#include <Arduino.h>

#include "config.h"
#include "motors.h"
#include "sbus.h"
#include "state.h"
#include "web.h"

namespace {
constexpr int PIN_STATUS_LED = 2;  // on-board blue LED

SbusReceiver sbus;
Motor motorLeft(PIN_ENA1, PIN_IN1, PIN_IN2, PWM_CH_A, MOTOR_A_REVERSED);
Motor motorRight(PIN_ENA2, PIN_IN3, PIN_IN4, PWM_CH_B, MOTOR_B_REVERSED);
ControllerState state;

int applyDeadband(int v) {
  if (abs(v) < RC_DEADBAND) return 0;
  // Rescale so output still reaches +-1000 at full stick.
  int sign = v > 0 ? 1 : -1;
  return sign * (abs(v) - RC_DEADBAND) * 1000 / (1000 - RC_DEADBAND);
}

// Arcade mix: throttle + steer -> left/right, scaled so neither exceeds 1000.
void arcadeMix(int throttle, int steer, int &left, int &right) {
  left = throttle + steer;
  right = throttle - steer;
  int m = max(abs(left), abs(right));
  if (m > 1000) {
    left = left * 1000 / m;
    right = right * 1000 / m;
  }
}

bool rcArmed() {
  if (RC_CH_ARM <= 0) return true;
  return sbus.us(RC_CH_ARM) > 1500;
}

void rcCommand(int &left, int &right) {
  if (RC_MIX_MODE == MixMode::Tank) {
    left = applyDeadband(sbus.scaled(RC_CH_LEFT));
    right = applyDeadband(sbus.scaled(RC_CH_RIGHT));
  } else {
    arcadeMix(applyDeadband(sbus.scaled(RC_CH_THROTTLE)),
              applyDeadband(sbus.scaled(RC_CH_STEER)), left, right);
  }
}

void updateStatusLed(bool rcOk) {
  // Solid: RC connected. Slow blink: enabled, no RC. Fast blink: disabled.
  uint32_t period = !state.enabled ? 200 : 1000;
  bool on = rcOk || ((millis() / (period / 2)) % 2 == 0);
  digitalWrite(PIN_STATUS_LED, on ? HIGH : LOW);
}
}  // namespace

void setup() {
  Serial.begin(115200);
  motorLeft.begin();
  motorRight.begin();
  pinMode(PIN_STATUS_LED, OUTPUT);

  sbus.begin(Serial2, PIN_SBUS_RX);
  webBegin(state, sbus, motorLeft, motorRight);
  Serial.println("Walking table controller ready");
}

void loop() {
  static uint32_t lastUs = micros();
  uint32_t nowUs = micros();
  float dt = (nowUs - lastUs) / 1e6f;
  lastUs = nowUs;

  sbus.poll();
  webLoop();

  bool rcOk = sbus.connected(SBUS_TIMEOUT_MS);
  int left = 0, right = 0;

  if (!state.enabled) {
    state.source = DriveSource::Stopped;
    motorLeft.stopNow();
    motorRight.stopNow();
  } else {
    if (rcOk) {
      // RC always has priority over the web page.
      state.source = rcArmed() ? DriveSource::Rc : DriveSource::Stopped;
      if (state.source == DriveSource::Rc) rcCommand(left, right);
    } else if (state.webCmdValid && millis() - state.webCmdMs < WEB_CMD_TIMEOUT_MS) {
      state.source = DriveSource::Web;
      left = state.webLeft;
      right = state.webRight;
    } else {
      // No RC and no fresh web command: failsafe stop.
      state.source = DriveSource::Stopped;
    }
    if (state.source == DriveSource::Stopped) {
      // Lost link or disarmed: cut power immediately rather than ramping.
      motorLeft.stopNow();
      motorRight.stopNow();
    } else {
      motorLeft.setTarget(left);
      motorRight.setTarget(right);
      motorLeft.update(dt);
      motorRight.update(dt);
    }
  }

  updateStatusLed(rcOk);
}
