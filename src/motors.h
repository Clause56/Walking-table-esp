#pragma once
#include <Arduino.h>

// One channel of the XY-160D: ENA is PWM speed, INa/INb set direction.
//   INa=H INb=L forward, INa=L INb=H reverse, both L = coast/stop.
class Motor {
 public:
  Motor(int pinEn, int pinInA, int pinInB, int pwmChannel, bool reversed);
  void begin();
  // Target speed -1000..1000. Applied gradually by update().
  void setTarget(int speed) { target_ = constrain(speed, -1000, 1000); }
  // Stop immediately without ramping.
  void stopNow();
  // Call every loop with elapsed seconds; ramps and drives the pins.
  void update(float dt);
  int current() const { return (int)current_; }
  int target() const { return target_; }

 private:
  void apply(int speed);

  int pinEn_, pinInA_, pinInB_, pwmChannel_;
  bool reversed_;
  int target_ = 0;
  float current_ = 0;
};
