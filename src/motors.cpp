#include "motors.h"
#include "config.h"

Motor::Motor(int pinEn, int pinInA, int pinInB, int pwmChannel, bool reversed)
    : pinEn_(pinEn), pinInA_(pinInA), pinInB_(pinInB), pwmChannel_(pwmChannel), reversed_(reversed) {}

void Motor::begin() {
  pinMode(pinInA_, OUTPUT);
  pinMode(pinInB_, OUTPUT);
  digitalWrite(pinInA_, LOW);
  digitalWrite(pinInB_, LOW);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttachChannel(pinEn_, PWM_FREQ_HZ, PWM_BITS, pwmChannel_);
#else
  ledcSetup(pwmChannel_, PWM_FREQ_HZ, PWM_BITS);
  ledcAttachPin(pinEn_, pwmChannel_);
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
