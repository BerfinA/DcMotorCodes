# Making Room — ESP32 Dev Log
# Occupancy, Summer 2026

## Log by Qilmeg

---

## 2. Arduino IDE Setup (ESP32)

  Board package: esp32 by Espressif Systems (install via Boards Manager)
  Board selection: ESP32 Dev Module
  Board settings (under Tools menu, visible only after selecting ESP32 Dev Module):
  - Upload Speed: 921600
  - CPU Frequency: 240MHz
  - Flash Size: 4MB (32Mb)
  - Partition Scheme: Default 4MB with spiffs
  - Port: whichever COM port the ESP32 appears as

  Library required: ESP32Encoder by Kevin Harrington (install via Library Manager)
  NOTE: do NOT use "Encoder by Paul Stoffregen" — causes GPIO IPC crash on ESP32 boot.
  WiFi.h and WebServer.h come bundled with the ESP32 board package.

---

## 3. Driver Setup — Elegoo ESP32 Type-C

  This board uses a CP2102 USB-to-UART chip (NOT CH340).
  Windows does not include this driver by default.

  If the board shows as unknown device with yellow exclamation mark in Device Manager:
  - Search: "CP210x driver Silicon Labs download" (official page is on silabs.com)
  - Install the CP210x Universal Windows Driver
  - Unplug and replug the ESP32 — it should now appear under Ports (COM & LPT)

  DO NOT install the CH340 driver — wrong chip for this board.

  To confirm the right COM port: open Device Manager → Ports (COM & LPT)
  It should say "Silicon Labs CP210x USB to UART Bridge (COMx)"
  Select that port in Arduino IDE under Tools → Port.

---

## 4. Upload Troubleshooting

  If upload fails with "Write timeout" / "Connecting...":
  - Make sure Serial Monitor is closed before uploading (it holds the COM port)
  - Make sure the correct COM port is selected (not a Bluetooth port)
  - Try the two-button method: hold BOOT → press+release EN/RST → release BOOT → click Upload
  - Try lowering upload speed to 115200 under Tools → Upload Speed
  - Use a data cable, not a charge-only cable (SSD/file-transfer cables work)

---

## 5. WiFi Setup and Test

  Using phone hotspot — this works fine.
  Both the ESP32 and the computer testing it must be connected to the same hotspot.

  Important:
  - Turn the hotspot ON before powering/resetting the ESP32
    (ESP32 tries to connect immediately at boot — if hotspot isn't up yet it times out)
  - The code gives 10 seconds to connect, then continues without WiFi
    (motor cycle still works, only remote trigger is unavailable)
  - Check SSID spelling exactly — apostrophes and special characters must match precisely
  - If WiFi fails, Serial Monitor will say so and the motor cycle still runs normally

  Once connected, Serial Monitor prints the IP address.
  Open that IP in a browser on the same hotspot network:
  - http://<IP>/status   → shows current position and state
  - http://<IP>/trigger  → motor drives to open (max) position and holds
  - http://<IP>/clear    → re-homes and resumes normal cycle

---

## 1. Pinout Mapping: Arduino → ESP32

  ┌────────────────┬─────────────┬─────────────────┬────────────────────────────────────┐
  │    Function    │ Arduino Pin │   ESP32 GPIO    │               Notes                │
  ├────────────────┼─────────────┼─────────────────┼────────────────────────────────────┤
  │ Limit Switch 1 │ 7           │ GPIO 18         │ INPUT_PULLUP — safe 3.3V pin       │
  ├────────────────┼─────────────┼─────────────────┼────────────────────────────────────┤
  │ Limit Switch 2 │ 6           │ GPIO 19         │ INPUT_PULLUP — safe 3.3V pin       │
  ├────────────────┼─────────────┼─────────────────┼────────────────────────────────────┤
  │ Motor IN1      │ 9           │ GPIO 25         │ Digital output                     │
  ├────────────────┼─────────────┼─────────────────┼────────────────────────────────────┤
  │ Motor IN2      │ 10          │ GPIO 26         │ Digital output                     │
  ├────────────────┼─────────────┼─────────────────┼────────────────────────────────────┤
  │ Encoder A      │ 2           │ GPIO 32         │ All ESP32 pins support interrupts  │
  ├────────────────┼─────────────┼─────────────────┼────────────────────────────────────┤
  │ Encoder B      │ 3           │ GPIO 33         │ All ESP32 pins support interrupts  │
  ├────────────────┼─────────────┼─────────────────┼────────────────────────────────────┤
  │ Power          │ 5V          │ VIN             │ ESP32 powered via VIN (5V input)   │
  ├────────────────┼─────────────┼─────────────────┼────────────────────────────────────┤
  │ GND            │ GND         │ GND             │ Common ground — connect everything │
  └────────────────┴─────────────┴─────────────────┴────────────────────────────────────┘

  Power setup (confirmed):
  - ESP32 is powered via VIN pin (5V external supply) — NOT the 3.3V pin
  - VIN has an onboard regulator that steps 5V down to 3.3V for the chip
  - GPIO logic is still 3.3V — motor driver IN1/IN2 receive 3.3V signals
  - Motor driver still powered from its own 12V external supply
  - Common GND between ESP32, motor driver, and power supplies

  Other hardware notes:
  - Avoid GPIO 0, 2, 15 (boot pins), GPIO 6–11 (internal flash), GPIO 34–39 (output-only)
  - Serial monitor baud rate: 115200
