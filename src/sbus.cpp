#include "sbus.h"

namespace {
constexpr uint8_t SBUS_HEADER = 0x0F;
constexpr uint8_t SBUS_FOOTER = 0x00;
constexpr uint8_t FLAG_FRAME_LOST = 0x04;
constexpr uint8_t FLAG_FAILSAFE = 0x08;
// Frames are sent every 7-14 ms; bytes within a frame are ~120 us apart.
// A gap longer than this means we are at the start of a new frame.
constexpr uint32_t INTER_FRAME_GAP_US = 3000;
}  // namespace

void SbusReceiver::begin(HardwareSerial &serial, int rxPin) {
  serial_ = &serial;
  // 100000 baud, 8 data bits, even parity, 2 stop bits, RX inverted.
  serial_->begin(100000, SERIAL_8E2, rxPin, -1, true);
}

bool SbusReceiver::poll() {
  bool gotFrame = false;
  while (serial_ && serial_->available()) {
    uint8_t b = serial_->read();
    uint32_t now = micros();
    if (now - lastByteUs_ > INTER_FRAME_GAP_US) pos_ = 0;
    lastByteUs_ = now;

    if (pos_ == 0 && b != SBUS_HEADER) continue;
    buf_[pos_++] = b;
    if (pos_ == sizeof(buf_)) {
      pos_ = 0;
      // Some receivers use 0x04/0x14/... footers for SBUS2; accept low nibble 4 too.
      if (buf_[24] == SBUS_FOOTER || (buf_[24] & 0x0F) == 0x04) {
        decode();
        gotFrame = true;
      }
    }
  }
  return gotFrame;
}

void SbusReceiver::decode() {
  const uint8_t *d = buf_ + 1;
  ch_[0]  = ((d[0]       | d[1] << 8)                 & 0x07FF);
  ch_[1]  = ((d[1] >> 3  | d[2] << 5)                 & 0x07FF);
  ch_[2]  = ((d[2] >> 6  | d[3] << 2 | d[4] << 10)    & 0x07FF);
  ch_[3]  = ((d[4] >> 1  | d[5] << 7)                 & 0x07FF);
  ch_[4]  = ((d[5] >> 4  | d[6] << 4)                 & 0x07FF);
  ch_[5]  = ((d[6] >> 7  | d[7] << 1 | d[8] << 9)     & 0x07FF);
  ch_[6]  = ((d[8] >> 2  | d[9] << 6)                 & 0x07FF);
  ch_[7]  = ((d[9] >> 5  | d[10] << 3)                & 0x07FF);
  ch_[8]  = ((d[11]      | d[12] << 8)                & 0x07FF);
  ch_[9]  = ((d[12] >> 3 | d[13] << 5)                & 0x07FF);
  ch_[10] = ((d[13] >> 6 | d[14] << 2 | d[15] << 10)  & 0x07FF);
  ch_[11] = ((d[15] >> 1 | d[16] << 7)                & 0x07FF);
  ch_[12] = ((d[16] >> 4 | d[17] << 4)                & 0x07FF);
  ch_[13] = ((d[17] >> 7 | d[18] << 1 | d[19] << 9)   & 0x07FF);
  ch_[14] = ((d[19] >> 2 | d[20] << 6)                & 0x07FF);
  ch_[15] = ((d[20] >> 5 | d[21] << 3)                & 0x07FF);

  uint8_t flags = buf_[23];
  frameLost_ = flags & FLAG_FRAME_LOST;
  failsafe_ = flags & FLAG_FAILSAFE;
  frames_++;
  lastFrameMs_ = millis();
}

uint16_t SbusReceiver::raw(int ch) const {
  if (ch < 1 || ch > NUM_CHANNELS) return 0;
  return ch_[ch - 1];
}

int SbusReceiver::us(int ch) const {
  // Standard SBUS mapping: 172 -> 988 us, 992 -> 1500 us, 1811 -> 2012 us.
  return (int)raw(ch) * 5 / 8 + 880;
}

int SbusReceiver::scaled(int ch) const {
  int v = (us(ch) - 1500) * 2;  // +-500 us -> +-1000
  return constrain(v, -1000, 1000);
}

bool SbusReceiver::connected(uint32_t timeoutMs) const {
  return frames_ > 0 && !failsafe_ && msSinceFrame() < timeoutMs;
}
