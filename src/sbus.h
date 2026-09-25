#pragma once
#include <Arduino.h>

// Minimal SBUS receiver for the ESP32.
// SBUS is 100000 baud, 8E2, logically inverted; the ESP32 UART can invert
// the RX line in hardware, so the FS-iA6B signal wire connects directly.
class SbusReceiver {
 public:
  static constexpr int NUM_CHANNELS = 16;

  void begin(HardwareSerial &serial, int rxPin);
  // Call often from loop(). Returns true when a new frame was decoded.
  bool poll();

  // Raw channel value (typically 172..1811 on FlySky), 1-based channel.
  uint16_t raw(int ch) const;
  // Channel as microseconds (~1000..2000), 1-based channel.
  int us(int ch) const;
  // Channel scaled to -1000..1000 around centre, 1-based channel.
  int scaled(int ch) const;

  // True when frames are arriving and the receiver reports no failsafe.
  bool connected(uint32_t timeoutMs) const;
  bool failsafe() const { return failsafe_; }
  bool frameLost() const { return frameLost_; }
  uint32_t frameCount() const { return frames_; }
  uint32_t msSinceFrame() const { return millis() - lastFrameMs_; }

 private:
  void decode();

  HardwareSerial *serial_ = nullptr;
  uint8_t buf_[25];
  uint8_t pos_ = 0;
  uint32_t lastByteUs_ = 0;
  uint16_t ch_[NUM_CHANNELS] = {0};
  bool failsafe_ = true;
  bool frameLost_ = false;
  uint32_t frames_ = 0;
  uint32_t lastFrameMs_ = 0;
};
