# Walking table controller (ESP32)

ESP32 DevKit v1 firmware that drives two brushed DC motors through an
**XY-160D** dual H-bridge, controlled by a **FlySky FS-iA6B** receiver over
**SBUS**, with a WiFi status page, an enable/disable switch and a manual
drive fallback for when the RC transmitter is not connected.

## Wiring

| ESP32 pin | Connects to | Purpose |
|-----------|-------------|---------|
| D13 | XY-160D ENA1 | Motor A (left) speed, PWM |
| D12 | XY-160D IN1 | Motor A direction |
| D14 | XY-160D IN2 | Motor A direction |
| D27 | XY-160D ENA2 | Motor B (right) speed, PWM |
| D26 | XY-160D IN3 | Motor B direction |
| D25 | XY-160D IN4 | Motor B direction |
| D15 | FS-iA6B SERVO/i-BUS "SBUS" signal | RC input |
| GND | XY-160D GND and receiver GND | Common ground (required) |

Power the receiver from 5 V (the FS-iA6B accepts 4.0-6.5 V). The SBUS signal
is inverted serial; the ESP32 UART inverts it in hardware, so no inverter
circuit is needed.

**Boot pin note:** GPIO12 (IN1) is an ESP32 strapping pin. If the motor
driver pulls it high while the ESP32 powers up, the board will fail to boot
(flash voltage is set to 1.8 V). If you see boot loops with the driver
connected, move IN1 to another pin (e.g. GPIO33) and update `src/config.h`.
GPIO15 (SBUS) is also a strapping pin but only affects boot log output.

## Behaviour

- **RC has priority.** While valid SBUS frames arrive, the sticks drive the motors.
- **Default mix is arcade**: CH2 (right stick up/down) is forward/back, CH1
  (right stick left/right) steers. Switch to tank mode or remap channels in
  `src/config.h`. An optional arm switch channel can be set with `RC_CH_ARM`.
- **Failsafe:** no SBUS frame for 100 ms, or the receiver's failsafe flag set,
  stops both motors immediately. Also set the receiver's failsafe on the
  transmitter (System > Failsafe) so the throttle channels go to centre.
- Speed changes are ramped (0 to full in 0.5 s) to protect the gearboxes.
- On-board blue LED: solid = RC connected, slow blink = enabled without RC,
  fast blink = disabled.

## WiFi interface

The ESP32 starts its own access point:

- Network: `WalkingTable`
- Password: `walktable123`
- Page: http://192.168.4.1/

The page shows RC link state, which source is driving, motor outputs and
channel values. The big button enables or disables the controller (enabled
at every power-up). When no RC signal is present and the controller is
enabled, the on-screen joystick drives the table; the motors stop if the
joystick is released or the page stops sending commands for 400 ms.

Change the network name and password in `src/config.h`.

## Flashing and testing

1. **Install the tools.** Install [VS Code](https://code.visualstudio.com/),
   then the **PlatformIO IDE** extension from the Extensions panel. If your
   computer doesn't see the board, install the USB-serial driver for the chip
   on your DevKit (CP210x or CH340).
2. **Get the code.** Clone the repo with
   `git clone https://github.com/Clause56/Walking-table-esp`, or download it
   as a ZIP from GitHub. Until the pull request is merged the code is on the
   `motor-controller` branch (`git checkout motor-controller`, or pick that
   branch before downloading the ZIP).
3. **Open the folder** in VS Code (File > Open Folder). PlatformIO sees
   `platformio.ini` and downloads the ESP32 toolchain the first time, which
   takes a few minutes.
4. **Connect the ESP32** with a USB data cable (some cables are charge-only).
   For the first test, lift the table so the wheels/legs are off the ground,
   or leave the motor power supply off.
5. **Upload.** Click the right-arrow "Upload" button in the blue status bar,
   or run `pio run -t upload`. If it stops at `Connecting....___`, hold the
   **BOOT** button on the board until the upload starts. If it fails to
   connect at all, unplug the wire on D12 (IN1) and try again (see the boot
   pin note above).
6. **Watch the serial monitor.** Click the plug icon in the status bar, or
   run `pio device monitor` (115200 baud). You should see
   `Walking table controller ready` and the WiFi address.
7. **Test.** Join the `WalkingTable` WiFi network (password `walktable123`),
   open http://192.168.4.1/, and check the RC link shows *connected* when the
   transmitter is on. Move the sticks gently and check each motor turns the
   right way; flip `MOTOR_A_REVERSED` / `MOTOR_B_REVERSED` in
   `src/config.h` if one runs backwards. Turn the transmitter off to check
   the motors stop and the page's joystick takes over.
