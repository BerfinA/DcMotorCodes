  // Making room — ESP32 port with WiFi control
  // Project: MET, Qilmeg / Rodrigo / Berfin, Summer 2026
  #include <Encoder.h>
  #include <WiFi.h>
  #include <WebServer.h>

  // ─── WiFi credentials ────────────────────────────────────────────────────────
  const char* ssid     = "YOUR_SSID";
  const char* password = "YOUR_PASSWORD";

  // ─── Pins (ESP32) ────────────────────────────────────────────────────────────
  #define limitSwitchPin  18
  #define limitSwitchPin2 19
  const uint8_t MOTOR_IN1 = 25;
  const uint8_t MOTOR_IN2 = 26;
  Encoder encoder(32, 33);  // all ESP32 GPIOs support interrupts

  // ─── Position limits ─────────────────────────────────────────────────────────
  const long maxValue = 2000;
  const long minValue = 100;

  // ─── State ───────────────────────────────────────────────────────────────────
  int           cycle           = 0;
  bool          GoingDown       = false;
  bool          spinDirection   = false;
  bool          normalMode      = true;
  bool          pausedAtTop     = false;
  long          currentPosition = -999;
  const long    delay_time      = 2000;
  unsigned long prev_time;

  // ─── WiFi trigger ────────────────────────────────────────────────────────────
  volatile bool triggered     = false;
  bool          triggeredPrev = false;
  WebServer     server(80);

  // ─── Motor helpers ───────────────────────────────────────────────────────────
  void goForward()  { digitalWrite(MOTOR_IN1, LOW);  digitalWrite(MOTOR_IN2, HIGH); }
  void goBackward() { digitalWrite(MOTOR_IN1, HIGH); digitalWrite(MOTOR_IN2, LOW);  }
  void motorStop()  { digitalWrite(MOTOR_IN1, LOW);  digitalWrite(MOTOR_IN2, LOW);  }

  void updateCounter() {
    long p = abs(encoder.read());
    if (p != currentPosition) {
      currentPosition = p;
      Serial.print("Pos: "); Serial.println(currentPosition);
    }
  }

  // ─── Homing ──────────────────────────────────────────────────────────────────
  void initial_position() {
    Serial.println("Homing...");
    goBackward();
    while (digitalRead(limitSwitchPin) == HIGH && digitalRead(limitSwitchPin2) == HIGH) {}
    encoder.write(minValue);
    goForward();
    spinDirection = true;
    delay(50);
    while (digitalRead(limitSwitchPin) == LOW || digitalRead(limitSwitchPin2) == LOW) {}
    GoingDown = true;
    prev_time = millis();
    Serial.println("Homing done.");
  }

  // ─── HTTP handlers ───────────────────────────────────────────────────────────
  void handleTrigger() {
    triggered = true;
    server.send(200, "text/plain", "Triggered — making room!");
    Serial.println("[WiFi] Trigger ON");
  }

  void handleClear() {
    triggered = false;
    server.send(200, "text/plain", "Cleared — resuming cycle.");
    Serial.println("[WiFi] Trigger OFF");
  }

  void handleStatus() {
    String s = "pos=" + String(currentPosition)
             + " triggered=" + String(triggered ? 1 : 0)
             + " cycle=" + String(cycle);
    server.send(200, "text/plain", s);
  }

  // ─── Setup ───────────────────────────────────────────────────────────────────
  void setup() {
    Serial.begin(115200);
    pinMode(limitSwitchPin,  INPUT_PULLUP);
    pinMode(limitSwitchPin2, INPUT_PULLUP);
    pinMode(MOTOR_IN1, OUTPUT);
    pinMode(MOTOR_IN2, OUTPUT);

    WiFi.begin(ssid, password);
    Serial.print("WiFi connecting");
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    Serial.println("\nIP: " + WiFi.localIP().toString());

    server.on("/trigger", HTTP_GET, handleTrigger);
    server.on("/clear",   HTTP_GET, handleClear);
    server.on("/status",  HTTP_GET, handleStatus);
    server.begin();

    initial_position();
  }

  // ─── Main loop ───────────────────────────────────────────────────────────────
  void loop() {
    server.handleClient();  // must stay here — processes incoming HTTP requests
    updateCounter();

    // ── Triggered: drive to open position and hold ──────────────────────────
    if (triggered) {
      if (!triggeredPrev) {
        motorStop();
        encoder.write(currentPosition);  // force positive count before going forward
        spinDirection = true;
        GoingDown     = true;
        triggeredPrev = true;
        goForward();
        Serial.println("Triggered — opening!");
      }
      if (currentPosition >= maxValue) motorStop();
      return;  // skip normal cycle while triggered
    }

    // Trigger just cleared → re-home for a clean known state
    if (triggeredPrev && !triggered) {
      triggeredPrev = false;
      Serial.println("Trigger cleared — re-homing.");
      initial_position();
      return;
    }

    // ── Normal cycle ─────────────────────────────────────────────────────────
    if (currentPosition >= maxValue && GoingDown) {
      motorStop();
      if (!pausedAtTop) { prev_time = millis(); pausedAtTop = true; }
      if ((millis() - prev_time) > delay_time) {
        pausedAtTop = false;
        if (spinDirection) { goBackward(); encoder.write(maxValue);  spinDirection = false; }
        else               { goForward();  encoder.write(-maxValue); spinDirection = true;  }
        GoingDown = false;
        Serial.println("Going Up!");
      }
      return;
    }

    if (cycle == 4) { normalMode = false; cycle = 0; }

    if ((digitalRead(limitSwitchPin) == LOW)  ||
        (digitalRead(limitSwitchPin2) == LOW) ||
        (normalMode && currentPosition <= minValue)) {
      Serial.println("Reset to min");
      cycle++;
      if (!normalMode) normalMode = true;
      if (spinDirection) { goBackward(); encoder.write(-minValue); spinDirection = false; }
      else               { goForward();  encoder.write(minValue);  spinDirection = true;  }
      GoingDown = true;
      Serial.println("Going Down!");
      prev_time = millis();
      delay(50);
      while (digitalRead(limitSwitchPin) == LOW || digitalRead(limitSwitchPin2) == LOW) {}
    }
  }
