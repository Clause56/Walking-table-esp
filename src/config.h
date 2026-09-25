#pragma once

// ---------------------------------------------------------------------------
// Pin mapping (ESP32 DevKit v1 -> XY-160D)
// ---------------------------------------------------------------------------
// Motor A
constexpr int PIN_ENA1 = 13;  // PWM speed
constexpr int PIN_IN1  = 12;  // direction (strapping pin, see README)
constexpr int PIN_IN2  = 14;  // direction
// Motor B
constexpr int PIN_ENA2 = 27;  // PWM speed
constexpr int PIN_IN3  = 26;  // direction
constexpr int PIN_IN4  = 25;  // direction

// SBUS input from FS-iA6B
constexpr int PIN_SBUS_RX = 15;

// Flip a motor's direction here instead of re-wiring it.
constexpr bool MOTOR_A_REVERSED = false;
constexpr bool MOTOR_B_REVERSED = false;

// ---------------------------------------------------------------------------
// PWM
// ---------------------------------------------------------------------------
constexpr uint32_t PWM_FREQ_HZ  = 8000;  // XY-160D accepts up to 10 kHz
constexpr uint8_t  PWM_BITS     = 8;     // duty 0..255
constexpr int      PWM_CH_A     = 0;     // LEDC channels (core 2.x API)
constexpr int      PWM_CH_B     = 1;

// Maximum change in motor command per second (full scale = 1000).
// 2000 means 0 -> full speed in 0.5 s. Protects the gearboxes and supply.
constexpr float RAMP_PER_SEC = 2000.0f;

// ---------------------------------------------------------------------------
// RC (SBUS) mapping. Channels are 1-based as shown on the transmitter.
// FS-i6 mode 2 defaults: CH1 = right stick X, CH2 = right stick Y,
// CH3 = left stick Y (throttle), CH4 = left stick X, CH5/CH6 = switches/knobs.
// ---------------------------------------------------------------------------
enum class MixMode { Arcade, Tank };
constexpr MixMode RC_MIX_MODE = MixMode::Arcade;

// Arcade: one stick drives, the other axis steers
constexpr int RC_CH_THROTTLE = 2;
constexpr int RC_CH_STEER    = 1;
// Tank: one channel per motor
constexpr int RC_CH_LEFT     = 3;
constexpr int RC_CH_RIGHT    = 2;

// Optional arm switch on the transmitter (0 disables). When set, the motors
// only run from RC while this channel is above ~1500 us.
constexpr int RC_CH_ARM = 0;

constexpr int RC_DEADBAND = 40;  // in command units (full scale 1000)

// No valid SBUS frame for this long => RC considered lost, motors stop.
constexpr uint32_t SBUS_TIMEOUT_MS = 100;

// ---------------------------------------------------------------------------
// WiFi (access point). Connect to this network and browse to 192.168.4.1
// ---------------------------------------------------------------------------
constexpr const char *WIFI_AP_SSID     = "WalkingTable";
constexpr const char *WIFI_AP_PASSWORD = "walktable123";  // >= 8 chars

// Web manual drive: motors stop if the page stops sending for this long.
constexpr uint32_t WEB_CMD_TIMEOUT_MS = 400;
