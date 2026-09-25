#pragma once
#include "motors.h"
#include "sbus.h"
#include "state.h"

// WiFi access point + status page with enable/disable and manual drive.
void webBegin(ControllerState &state, SbusReceiver &sbus, Motor &left, Motor &right);
void webLoop();
