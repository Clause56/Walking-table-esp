#include "motors.h"
#include "config.h"

Motor::Motor(int pinEn, int pinInA, int pinInB, int pwmChannel, bool reversed, int pinEnAlt)
    : pinEn_(pinEn), pinInA_(pinInA), pinInB_(pinInB), pwmChannel_(pwmChannel),
      pinEnAlt_(pinEnAlt), reversed_(reversed) {}

void Motor::holdOff() {
  // Write the level before switching to output so the pin never glitches HIGH.
  for (int pin : {pinEn_, pinInA_, pinInB_, pinEnAlt_}) {
    if (pin < 0) continue;
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
}

void Motor::begin() {
  holdOff();
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttachChannel(pinEn_, PWM_FREQ_HZ, PWM_BITS, pwmChannel_);
  // Same channel, so ledcWrite on pinEn_ drives both pins.
  if (pinEnAlt_ >= 0) ledcAttachChannel(pinEnAlt_, PWM_FREQ_HZ, PWM_BITS, pwmChannel_);
#else
  ledcSetup(pwmChannel_, PWM_FREQ_HZ, PWM_BITS);
  ledcAttachPin(pinEn_, pwmChannel_);
  if (pinEnAlt_ >= 0) ledcAttachPin(pinEnAlt_, pwmChannel_);
#endif
  apply(0);
}

void Motor::stopNow() {
  target_ = 0;
  current_ = 0;
  apply(0);
}

void Motor::update(float dt) {
  float step = RAMP_PER_SEC * dt;
  float diff = target_ - current_;
  if (diff > step) diff = step;
  if (diff < -step) diff = -step;
  current_ += diff;
  apply((int)current_);
}

void Motor::apply(int speed) {
  if (reversed_) speed = -speed;
  const uint32_t maxDuty = (1u << PWM_BITS) - 1;
  uint32_t duty = (uint32_t)abs(speed) * maxDuty / 1000;

  if (speed > 0) {
    digitalWrite(pinInA_, HIGH);
    digitalWrite(pinInB_, LOW);
  } else if (speed < 0) {
    digitalWrite(pinInA_, LOW);
    digitalWrite(pinInB_, HIGH);
  } else {
    digitalWrite(pinInA_, LOW);
    digitalWrite(pinInB_, LOW);
  }
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(pinEn_, duty);
#else
  ledcWrite(pwmChannel_, duty);
#endif
}
