#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// ============================================
// 1. PIN DEFINITIONS
// ============================================
#define ENA 14        // GPIO14 (D5)
#define ENB 12        // GPIO12 (D6)
#define IN_1 15       // GPIO15 (D8)
#define IN_2 13       // GPIO13 (D7)
#define IN_3 2        // GPIO2 (D4)
#define IN_4 0        // GPIO0 (D3)

#define ECHO 16       // GPIO16 (D0)
#define TRIG 5        // GPIO5 (D1)
#define SERVO_PIN 4   // GPIO4 (D2)
#define L_SENSOR 1    // GPIO1 (TX)
#define R_SENSOR 3    // GPIO3 (RX)

// ============================================
// 2. CONFIGURATION
// ============================================
int speedCar = 800;             // Default Speed (Changed by App Slider)
const int speedCoeff = 3;

// Fixed speeds for specific Obstacle maneuvers (User Request)
const int SPEED_OBSTACLE_BACK = 1023; // Max power to pull away
const int SPEED_OBSTACLE_TURN = 900;  // Fixed for consistent turning angles

// Speed mapping for App Slider (0-9)
const int SPEED_TABLE[] = {430, 470, 540, 610, 680, 750, 820, 890, 960, 1023};

// Mode Flags
bool wifiMode = true; // Default to WiFi
bool obstacleMode = false;
bool lineMode = false;

ESP8266WebServer server(80);

// ============================================
// 3. SMART DELAY (CRITICAL FOR APP)
// ============================================
// Checks for App commands while waiting
bool smartDelay(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    server.handleClient(); 
    // If mode changes, abort delay
    if (!obstacleMode && !lineMode && !wifiMode) return false; 
    if (obstacleMode && (wifiMode || lineMode)) return false;
    yield();
  }
  return true;
}

// ============================================
// 4. SERVO FUNCTIONS
// ============================================
// Quick pulse for startup sweep
void servoPulse(int angle) {
  int pulseWidth = map(angle, 0, 180, 500, 2500);
  for(int i=0; i<10; i++) {
    digitalWrite(SERVO_PIN, HIGH);
    delayMicroseconds(pulseWidth);
    digitalWrite(SERVO_PIN, LOW);
    delayMicroseconds(20000 - pulseWidth);
  }
}

// Reliable hold for operations
void setServoAngle(int angle) {
  int pulseWidth = map(angle, 0, 180, 500, 2500);
  for(int j=0; j<3; j++) { // Repeat 3 times for holding torque
    for(int i=0; i<10; i++) {
      digitalWrite(SERVO_PIN, HIGH);
      delayMicroseconds(pulseWidth);
      digitalWrite(SERVO_PIN, LOW);
      delayMicroseconds(20000 - pulseWidth);
    }
  }
}

// Exact sweep from your original code
void servoStartupSweep() {
  servoPulse(0);   delay(100);
  servoPulse(45);  delay(100);
  servoPulse(90);  delay(100);
  servoPulse(135); delay(100);
  servoPulse(180); delay(100);
  servoPulse(135); delay(100);
  servoPulse(90);  delay(100);
  servoPulse(45);  delay(100);
  servoPulse(0);   delay(100);
  setServoAngle(80); // Center
}

// ============================================
// 5. MOTOR FUNCTIONS (SPEED CONTROL INTEGRATED)
// ============================================
void stopRobot() {
  digitalWrite(IN_1, LOW); digitalWrite(IN_2, LOW);
  digitalWrite(IN_3, LOW); digitalWrite(IN_4, LOW);
  analogWrite(ENA, 0); analogWrite(ENB, 0);
}

void goAhead() {
  digitalWrite(IN_1, LOW); digitalWrite(IN_2, HIGH);
  digitalWrite(IN_3, LOW); digitalWrite(IN_4, HIGH);
  // USES APP SLIDER SPEED FOR ALL MODES
  analogWrite(ENA, speedCar); 
  analogWrite(ENB, speedCar);
}

void goBack() {
  digitalWrite(IN_1, HIGH); digitalWrite(IN_2, LOW);
  digitalWrite(IN_3, HIGH); digitalWrite(IN_4, LOW);
  
  if (obstacleMode) {
    // Obstacle uses MAX speed to retreat
    analogWrite(ENA, SPEED_OBSTACLE_BACK); 
    analogWrite(ENB, SPEED_OBSTACLE_BACK);
  } else {
    // WiFi/Line uses App speed
    analogWrite(ENA, speedCar); 
    analogWrite(ENB, speedCar);
  }
}

void goRight() {
  digitalWrite(IN_1, HIGH); digitalWrite(IN_2, LOW);
  digitalWrite(IN_3, LOW); digitalWrite(IN_4, HIGH);
  
  if (obstacleMode) {
    // Obstacle uses FIXED speed for accurate angles
    analogWrite(ENA, SPEED_OBSTACLE_TURN); 
    analogWrite(ENB, SPEED_OBSTACLE_TURN);
  } else {
    // WiFi/Line uses App speed (overcome friction)
    analogWrite(ENA, speedCar); 
    analogWrite(ENB, speedCar);
  }
}

void goLeft() {
  digitalWrite(IN_1, LOW); digitalWrite(IN_2, HIGH);
  digitalWrite(IN_3, HIGH); digitalWrite(IN_4, LOW);
  
  if (obstacleMode) {
    analogWrite(ENA, SPEED_OBSTACLE_TURN); 
    analogWrite(ENB, SPEED_OBSTACLE_TURN);
  } else {
    analogWrite(ENA, speedCar); 
    analogWrite(ENB, speedCar);
  }
}

// Diagonal Moves (WiFi Only)
void goAheadRight() {
  digitalWrite(IN_1, LOW); digitalWrite(IN_2, HIGH);
  digitalWrite(IN_3, LOW); digitalWrite(IN_4, HIGH);
  analogWrite(ENA, speedCar/speedCoeff); analogWrite(ENB, speedCar);
}
void goAheadLeft() {
  digitalWrite(IN_1, LOW); digitalWrite(IN_2, HIGH);
  digitalWrite(IN_3, LOW); digitalWrite(IN_4, HIGH);
  analogWrite(ENA, speedCar); analogWrite(ENB, speedCar/speedCoeff);
}
void goBackRight() {
  digitalWrite(IN_1, HIGH); digitalWrite(IN_2, LOW);
  digitalWrite(IN_3, HIGH); digitalWrite(IN_4, LOW);
  analogWrite(ENA, speedCar/speedCoeff); analogWrite(ENB, speedCar);
}
void goBackLeft() {
  digitalWrite(IN_1, HIGH); digitalWrite(IN_2, LOW);
  digitalWrite(IN_3, HIGH); digitalWrite(IN_4, LOW);
  analogWrite(ENA, speedCar); analogWrite(ENB, speedCar/speedCoeff);
}

// ============================================
// 6. SENSORS
// ============================================
int getDistance() {
  digitalWrite(TRIG, LOW); delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  unsigned long duration = pulseIn(ECHO, HIGH, 30000);
  return (int)(duration * 0.034 / 2);
}

// ============================================
// 7. MODES
// ============================================
void obstacleAvoidingMode() {
  int distance = getDistance();
  
  if (distance >= 15 || distance == 0) {
    goAhead(); // Uses App Speed
  } else {
    goBack(); // Uses Max Speed
    if(!smartDelay(500)) { stopRobot(); return; }
    stopRobot();
    
    goAhead(); // Nudge (App Speed)
    delay(15); 
    stopRobot();
    
    // Scan Sequence
    setServoAngle(170);
    if(!smartDelay(500)) { stopRobot(); return; }
    int leftDistance = getDistance();
    
    setServoAngle(0);
    if(!smartDelay(1000)) { stopRobot(); return; }
    int rightDistance = getDistance();
    
    setServoAngle(80);
    if(!smartDelay(500)) { stopRobot(); return; }
    
    // Decision
    if (rightDistance != 0 && leftDistance >= rightDistance) {
      goLeft();
      if(!smartDelay(400)) { stopRobot(); return; }
    } else if (leftDistance != 0 && leftDistance <= rightDistance) {
      goRight();
      if(!smartDelay(400)) { stopRobot(); return; }
    } else if (leftDistance == 0 && rightDistance == 0) {
      goLeft();
      if(!smartDelay(400)) { stopRobot(); return; }
    } else if (rightDistance == 0) {
      goRight();
      if(!smartDelay(400)) { stopRobot(); return; }
    } else if (leftDistance == 0) {
      goLeft();
      if(!smartDelay(400)) { stopRobot(); return; }
    }
    stopRobot();
  }
}

void lineFollowerMode() {
  int l1 = digitalRead(L_SENSOR);
  int r1 = digitalRead(R_SENSOR);
  
  // Uses App Speed for ALL moves (helps friction)
  if (l1 == 0 && r1 == 0) goAhead();
  else if (l1 == 0 && r1 == 1) goRight();
  else if (l1 == 1 && r1 == 0) goLeft();
  else stopRobot();
}

// ============================================
// 8. APP COMMAND HANDLER
// ============================================
void handleCommand() {
  // A. Mode Selection
  if (server.hasArg("Mode")) {
    String mode = server.arg("Mode");
    wifiMode = false; obstacleMode = false; lineMode = false;
    stopRobot();

    if (mode == "W") wifiMode = true;
    else if (mode == "O") { obstacleMode = true; setServoAngle(80); }
    else if (mode == "L" || mode == "F") lineMode = true; 
    // Mode X (Stop) falls through with all flags false
    
    server.send(200, "text/plain", "OK");
    return;
  }

  // B. Speed & Manual Movement
  if (server.hasArg("State")) {
    String cmd = server.arg("State");
    char c = cmd[0];
    
    // 1. Update Speed (Applies to ALL modes immediately)
    if (c >= '0' && c <= '9') {
      speedCar = SPEED_TABLE[c - '0'];
      server.send(200, "text/plain", "OK");
      return;
    }

    // 2. WiFi Movement (Only works if wifiMode is active)
    if (wifiMode) {
      if (c == 'S') stopRobot();
      else if (c == 'A') goAhead();
      else if (c == 'B') goBack();
      else if (c == 'L') goLeft();
      else if (c == 'R') goRight();
      else if (c == 'I') goAheadRight();
      else if (c == 'G') goAheadLeft();
      else if (c == 'J') goBackRight();
      else if (c == 'H') goBackLeft();
    }
    server.send(200, "text/plain", "OK");
  } else {
    server.send(200, "text/plain", "OK");
  }
}

void handleRoot() {
  server.send(200, "text/html", "<h1>TriBot Ready</h1>");
}

// ============================================
// 9. SETUP & LOOP
// ============================================
void setup() {
  pinMode(IN_1, OUTPUT); pinMode(IN_2, OUTPUT);
  pinMode(IN_3, OUTPUT); pinMode(IN_4, OUTPUT);
  pinMode(ENA, OUTPUT);  pinMode(ENB, OUTPUT);
  pinMode(SERVO_PIN, OUTPUT);
  pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT);
  pinMode(L_SENSOR, INPUT); pinMode(R_SENSOR, INPUT);
  
  analogWriteRange(1023);
  stopRobot();
  
  // Run the sweep animation
  servoStartupSweep();
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP("TriBot_WiFi", "12345678");
  
  server.on("/", handleRoot);
  server.on("/action", handleCommand);
  server.onNotFound(handleCommand);
  server.begin();
}

void loop() {
  server.handleClient();
  
  if (obstacleMode) obstacleAvoidingMode();
  else if (lineMode) lineFollowerMode();
  // WiFi mode is event-driven inside handleCommand
}