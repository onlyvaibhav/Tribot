#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// ---------------- PIN DEFINITIONS ----------------
#define ENA 14      // Enable/speed motors Right GPIO14(D5)
#define ENB 12      // Enable/speed motors Left  GPIO12(D6)
#define IN_1 15     // L298N in1 motors Right    GPIO15(D8)
#define IN_2 13     // L298N in2 motors Right    GPIO13(D7)
#define IN_3 2      // L298N in3 motors Left     GPIO2(D4)
#define IN_4 0      // L298N in4 motors Left     GPIO0(D3)

#define echo 16     // D0 (GPIO16)
#define L D10       // D10 is GPIO1 (TX) – left line sensor
#define R D9        // D9  is GPIO3 (RX) – right line sensor

#define SERVO_PIN 4 // D2 (GPIO4)
#define trig 5      // D1 (GPIO5)

// ---------------- GLOBAL STATE ----------------
int speedCar = 800;        // 0–1023, controlled by State=0..9 from app
int speed_Coeff = 3;       // used for curves/diagonals

// Mode flags
int wifiCo = 1;
int obstacleAv = 0;
int lineFo = 0;
int go = 1;                // used by line follower

ESP8266WebServer server(80);
String command;

// ---------------- BASIC HTTP RESPONSE ----------------
void handleRoot() {
  server.send(200, "text/html", "");
}

// ---------------- MOTOR HELPERS ----------------
void goAhead() {
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, HIGH);
  digitalWrite(IN_3, LOW);
  digitalWrite(IN_4, HIGH);

  // Use the same speedCar for all modes; app controls per-mode speed
  analogWrite(ENA, speedCar);
  analogWrite(ENB, speedCar);

  handleRoot();
}

void goBack() {
  digitalWrite(IN_1, HIGH);
  digitalWrite(IN_2, LOW);
  digitalWrite(IN_3, HIGH);
  digitalWrite(IN_4, LOW);

  analogWrite(ENA, speedCar);
  analogWrite(ENB, speedCar);

  handleRoot();
}

void goRight() {
  // Right turn: right wheel backward, left forward
  digitalWrite(IN_1, HIGH);
  digitalWrite(IN_2, LOW);
  digitalWrite(IN_3, LOW);
  digitalWrite(IN_4, HIGH);

  analogWrite(ENA, speedCar);
  analogWrite(ENB, speedCar);

  handleRoot();
}

void goLeft() {
  // Left turn: right wheel forward, left backward
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, HIGH);
  digitalWrite(IN_3, HIGH);
  digitalWrite(IN_4, LOW);

  analogWrite(ENA, speedCar);
  analogWrite(ENB, speedCar);

  handleRoot();
}

// Diagonal / curve motions use speed_Coeff
void goAheadRight() {
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, HIGH);
  digitalWrite(IN_3, LOW);
  digitalWrite(IN_4, HIGH);

  analogWrite(ENA, speedCar / speed_Coeff);  // right slower
  analogWrite(ENB, speedCar);               // left normal

  handleRoot();
}

void goAheadLeft() {
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, HIGH);
  digitalWrite(IN_3, LOW);
  digitalWrite(IN_4, HIGH);

  analogWrite(ENA, speedCar);               // right normal
  analogWrite(ENB, speedCar / speed_Coeff); // left slower

  handleRoot();
}

void goBackRight() {
  digitalWrite(IN_1, HIGH);
  digitalWrite(IN_2, LOW);
  digitalWrite(IN_3, HIGH);
  digitalWrite(IN_4, LOW);

  analogWrite(ENA, speedCar / speed_Coeff);
  analogWrite(ENB, speedCar);

  handleRoot();
}

void goBackLeft() {
  digitalWrite(IN_1, HIGH);
  digitalWrite(IN_2, LOW);
  digitalWrite(IN_3, HIGH);
  digitalWrite(IN_4, LOW);

  analogWrite(ENA, speedCar);
  analogWrite(ENB, speedCar / speed_Coeff);

  handleRoot();
}

void stopRobot() {
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, LOW);
  digitalWrite(IN_3, LOW);
  digitalWrite(IN_4, LOW);

  analogWrite(ENA, 0);
  analogWrite(ENB, 0);

  handleRoot();
}

// ---------------- SERVO + ULTRASONIC ----------------
void setServoAngle(int angle) {
  int pulseWidth = map(angle, 0, 180, 500, 2500); // 500µs to 2500µs
  for (int i = 1; i <= 10; i++) {
    digitalWrite(SERVO_PIN, HIGH);
    delayMicroseconds(pulseWidth);
    digitalWrite(SERVO_PIN, LOW);
    delayMicroseconds(20000 - pulseWidth); // 50Hz signal
  }
}

int getDistance() {
  unsigned long duration;
  float distance;

  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  duration = pulseIn(echo, HIGH, 30000); // timeout 30ms
  distance = duration * 0.034 / 2.0;
  return (int)distance;
}

// ---------------- MODES ----------------
void obstacleAvoidingMode() {
  int distance = getDistance();

  if (distance >= 15 || distance == 0) {
    // free path
    goAhead();
  } else {
    // obstacle logic (same as your original)
    goBack();
    delay(500);
    stopRobot();

    goAhead();
    delay(15);
    stopRobot();

    // look left
    setServoAngle(170);
    setServoAngle(170);
    setServoAngle(170);
    delay(500);
    int leftDistance = getDistance();
    delay(500);

    // look right
    setServoAngle(0);
    setServoAngle(0);
    setServoAngle(0);
    delay(1000);
    int rightDistance = getDistance();
    delay(500);

    // center
    setServoAngle(80);
    setServoAngle(80);
    setServoAngle(80);
    delay(500);

    if (rightDistance != 0 && leftDistance >= rightDistance) {
      goLeft();
      delay(400);
      stopRobot();
    } else if (leftDistance != 0 && leftDistance <= rightDistance) {
      goRight();
      delay(400);
      stopRobot();
    } else if (leftDistance == 0 && rightDistance == 0) {
      goLeft();
      delay(400);
      stopRobot();
    } else if (rightDistance == 0) {
      goRight();
      delay(400);
      stopRobot();
    } else if (leftDistance == 0) {
      goLeft();
      delay(400);
      stopRobot();
    } else {
      stopRobot();
    }
  }
}

void lineFollowerMode() {
  if (go == 1) {
    byte l1 = 0, r1 = 0;
    byte w = 0, b = 1;

    l1 = digitalRead(L);
    r1 = digitalRead(R);

    if (l1 == w && r1 == w) {
      goAhead();
    } else if (l1 == w && r1 == b) {
      goRight();
    } else if (l1 == b && r1 == w) {
      goLeft();
    } else {
      stopRobot();
      go = 0;  // stop following until mode is re-selected from app
    }
  }
}

// ---------------- HTTP COMMAND HANDLING ----------------

// Map digit '0'..'9' to speedCar values (same as your original)
void setSpeedFromDigit(char d) {
  switch (d) {
    case '0': speedCar = 430; break;
    case '1': speedCar = 470; break;
    case '2': speedCar = 540; break;
    case '3': speedCar = 610; break;
    case '4': speedCar = 680; break;
    case '5': speedCar = 750; break;
    case '6': speedCar = 820; break;
    case '7': speedCar = 890; break;
    case '8': speedCar = 960; break;
    case '9': speedCar = 1023; break;
    default: break;
  }
}

// Handle Mode=W/O/F and State commands from the app
void handleHttpCommands() {
  server.handleClient();

  // ----- MODE: W / O / F -----
  if (server.hasArg("Mode")) {
    command = server.arg("Mode");

    if (command == "W") {
      wifiCo = 1;
      obstacleAv = 0;
      lineFo = 0;
      go = 1;
    } else if (command == "O") {
      wifiCo = 0;
      obstacleAv = 1;
      lineFo = 0;
      go = 1;
    } else if (command == "F") {
      wifiCo = 0;
      obstacleAv = 0;
      lineFo = 1;
      go = 1;
    }
    handleRoot();
  }

  // ----- STATE: movement, speed, stop -----
  if (server.hasArg("State")) {
    command = server.arg("State");

    // Speed digits 0..9 (work in all modes)
    if (command.length() == 1 && command[0] >= '0' && command[0] <= '9') {
      setSpeedFromDigit(command[0]);
      handleRoot();
      return;
    }

    // Stop – valid in all modes
    if (command == "S") {
      stopRobot();
      return;
    }

    // Directional movement only in Wi-Fi manual mode
    if (wifiCo) {
      if (command == "A") goAhead();
      else if (command == "B") goBack();
      else if (command == "L") goLeft();
      else if (command == "R") goRight();
      else if (command == "I") goAheadRight();
      else if (command == "G") goAheadLeft();
      else if (command == "J") goBackRight();
      else if (command == "H") goBackLeft();
      // unknown commands ignored
    }
  }
}

// ---------------- SETUP & LOOP ----------------
void setup() {
  WiFi.softAP("NODE_mcu", "1234567890");
  server.on("/", handleRoot);
  server.onNotFound(handleRoot);
  server.begin();

  // Motor pins
  pinMode(IN_1, OUTPUT);
  pinMode(IN_2, OUTPUT);
  pinMode(IN_3, OUTPUT);
  pinMode(IN_4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  analogWriteRange(1023);

  // Sensors & servo
  pinMode(SERVO_PIN, OUTPUT);
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  pinMode(R, INPUT);
  pinMode(L, INPUT);

  // Servo sweep on startup
  setServoAngle(0);
  setServoAngle(45);
  setServoAngle(90);
  setServoAngle(135);
  setServoAngle(180);
  setServoAngle(135);
  setServoAngle(90);
  setServoAngle(45);
  setServoAngle(0);
  setServoAngle(80);
}

void loop() {
  // Always handle incoming HTTP commands first
  handleHttpCommands();

  // Then run the active mode
  if (wifiCo) {
    // Manual mode is event-driven by State commands,
    // so we don't need to do anything here.
  } else if (obstacleAv) {
    obstacleAvoidingMode();
  } else if (lineFo) {
    lineFollowerMode();
  }
}
