#pragma once
#include <Arduino.h>

enum class DriveSource { Stopped, Rc, Web };

// Shared between the control loop and the web interface.
struct ControllerState {
  bool enabled = true;  // web enable/disable; defaults to enabled at boot
  DriveSource source = DriveSource::Stopped;

  // Latest manual command from the web page, -1000..1000 per side.
  int webLeft = 0;
  int webRight = 0;
  uint32_t webCmdMs = 0;
  bool webCmdValid = false;
};

inline const char *sourceName(DriveSource s) {
  switch (s) {
    case DriveSource::Rc: return "rc";
    case DriveSource::Web: return "web";
    default: return "stopped";
  }
}
