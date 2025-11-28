#include<ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#define ENA 14                         // Enable/speed motors Right GPIO14(D5)
#define ENB 12                        // Enable/speed motors Left   GPIO12(D6)
#define IN_1 15                      // L298N in1 motors Right     GPIO15(D8)
#define IN_2 13                     // L298N in2 motors Right     GPIO13(D7)
#define IN_3 2                     // L298N in3 motors Left         GPIO2(D4)
#define IN_4 0                    // L298N in4 motors Left         GPIO0(D3)
#define echo 16                  //D0 (GPIO16)
#define L D10                   //D10 is GPIO1 (TX)
#define R D9                   //D9 is GPIO3 (RX)
#define SERVO_PIN 4           // D2 (GPIO4)
#define trig 5               // D1 (GPIO5)


int speedCar=800;
int speed_Coeff = 3;
int wifiCo,obstacleAv,lineFo,go=1;
ESP8266WebServer server(80);
String command;

void handleRoot() {
server.send(200, "text/html", "");
}


void goAhead(){
digitalWrite(IN_1, LOW);
digitalWrite(IN_2, HIGH);
if(wifiCo) analogWrite(ENA, speedCar);
if(obstacleAv) analogWrite(ENA, 700);
if(lineFo) analogWrite(ENA, 500);
digitalWrite(IN_3, LOW);
digitalWrite(IN_4, HIGH);
if(wifiCo) analogWrite(ENB, speedCar);
if(obstacleAv) analogWrite(ENB, 700);
if(lineFo) analogWrite(ENB, 500);
handleRoot();
}


void goBack(){
digitalWrite(IN_1, HIGH);
digitalWrite(IN_2, LOW);
if(wifiCo) analogWrite(ENA, speedCar);
if(obstacleAv) analogWrite(ENA, 1023);
digitalWrite(IN_3, HIGH);
digitalWrite(IN_4, LOW);
if(wifiCo) analogWrite(ENB, speedCar);
if(obstacleAv) analogWrite(ENB, 1023);
handleRoot();
}


void goRight(){
digitalWrite(IN_1, HIGH);
digitalWrite(IN_2, LOW);
if(wifiCo) analogWrite(ENA, speedCar);
if(obstacleAv) analogWrite(ENA, 900);
if(lineFo) analogWrite(ENA, 900);
digitalWrite(IN_3, LOW);
digitalWrite(IN_4, HIGH);
if(wifiCo) analogWrite(ENB, speedCar);
if(obstacleAv) analogWrite(ENB, 900);
if(lineFo) analogWrite(ENB, 900);
handleRoot();
}


void goLeft(){
digitalWrite(IN_1, LOW);digitalWrite(IN_2, HIGH);
if(wifiCo) analogWrite(ENA, speedCar);
if(obstacleAv) analogWrite(ENA, 900);
if(lineFo) analogWrite(ENA, 900);
digitalWrite(IN_3, HIGH);
digitalWrite(IN_4, LOW);
if(wifiCo) analogWrite(ENB, speedCar);
if(obstacleAv) analogWrite(ENB, 900);
if(lineFo) analogWrite(ENB, 900);
handleRoot();
}


void goAheadRight(){
digitalWrite(IN_1, LOW);
digitalWrite(IN_2, HIGH);
analogWrite(ENA, speedCar/speed_Coeff);
digitalWrite(IN_3, LOW);
digitalWrite(IN_4, HIGH);
analogWrite(ENB, speedCar);
handleRoot();
}


void goAheadLeft(){
digitalWrite(IN_1, LOW);
digitalWrite(IN_2, HIGH);
analogWrite(ENA, speedCar);
digitalWrite(IN_3, LOW);
digitalWrite(IN_4, HIGH);
analogWrite(ENB, speedCar/speed_Coeff);
handleRoot();
}


void goBackRight(){
digitalWrite(IN_1, HIGH);
digitalWrite(IN_2, LOW);
analogWrite(ENA, speedCar/speed_Coeff);
digitalWrite(IN_3, HIGH);
digitalWrite(IN_4, LOW);
analogWrite(ENB, speedCar);
handleRoot();
}


void goBackLeft(){
digitalWrite(IN_1, HIGH);
digitalWrite(IN_2, LOW);
analogWrite(ENA, speedCar);digitalWrite(IN_3, HIGH);
digitalWrite(IN_4, LOW);
analogWrite(ENB, speedCar/speed_Coeff);
handleRoot();
}


void stopRobot(){
digitalWrite(IN_1, LOW);
digitalWrite(IN_2, LOW);
analogWrite(ENA, 0);
digitalWrite(IN_3, LOW);
digitalWrite(IN_4, LOW);
analogWrite(ENB, 0);
handleRoot();
}


void setServoAngle(int angle) {
int pulseWidth = map(angle, 0, 180, 500, 2500); // Convert angle to pulse width (500µs to 2500µs)
for(int i=1;i<=10;i++){
digitalWrite(SERVO_PIN, HIGH);
delayMicroseconds(pulseWidth); // ON time
digitalWrite(SERVO_PIN, LOW);
delayMicroseconds(20000 - pulseWidth); // OFF time to maintain 50Hz frequency
}
}
int getDistance(){
unsigned long duration;
float distance; 
digitalWrite(trig, LOW); // Trigger the ultrasonic pulse
delayMicroseconds(2);
digitalWrite(trig, HIGH);
delayMicroseconds(10);
digitalWrite(trig, LOW); 
duration = pulseIn(echo, HIGH, 30000); // Read the echo pin with a timeout of 30ms
distance = duration * 0.034 / 2;
return (int)distance;
}


void obstacleAvoidingMode(){
int distance=getDistance();
if(distance >= 15 || distance==0 ){
goAhead();
}
else {
goBack();delay(500);
stopRobot();
goAhead();delay(15);stopRobot();
setServoAngle(170);
setServoAngle(170);
setServoAngle(170);
delay(500);
int leftDistance=getDistance();
delay(500);
setServoAngle(0);
setServoAngle(0);
setServoAngle(0);
delay(1000);
int rightDistance=getDistance();
delay(500);
setServoAngle(80);
setServoAngle(80);
setServoAngle(80);
delay(500);
if(rightDistance !=0 && leftDistance>=rightDistance){
goLeft();delay(400);
stopRobot();
}
else if(leftDistance !=0 && leftDistance<=rightDistance){
goRight();delay(400);
stopRobot();
}
else if(leftDistance ==0 && rightDistance==0){
goLeft();delay(400);
stopRobot();
}
else if(rightDistance ==0){
goRight();delay(400);
stopRobot();
}
else if (leftDistance== 0){
goLeft();delay(400);
stopRobot();
}
else{
stopRobot();
}
}
}


void lineFollowerMode(){
if(go==1){
byte l1=0,r1=0;
byte w=0,b=1;
l1=digitalRead(L);
r1=digitalRead(R);
if(l1==w && r1==w){
goAhead();
}
else if(l1==w && r1==b){
goRight();
}else if(l1==b && r1==w){
goLeft();
}
else {
stopRobot();
go=0;
}
}
}


void wifiControllerMode(){
server.handleClient();
if (server.hasArg("State")) { // Check if a new argument is received
command = server.arg("State");
if (command == "A") goAhead();
else if (command == "B") goBack();
else if (command == "L") goLeft();
else if (command == "R") goRight();
else if (command == "I") goAheadRight();
else if (command == "G") goAheadLeft();
else if (command == "J") goBackRight();
else if (command == "H") goBackLeft();
else if (command == "0"){ speedCar = 430;handleRoot();}
else if (command == "1") {speedCar = 470;handleRoot();}
else if (command == "2") {speedCar = 540;handleRoot();}
else if (command == "3") {speedCar = 610;handleRoot();}
else if (command == "4") {speedCar = 680;handleRoot();}
else if (command == "5") {speedCar = 750;handleRoot();}
else if (command == "6") {speedCar = 820;handleRoot();}
else if (command == "7") {speedCar = 890;handleRoot();}
else if (command == "8") {speedCar = 960;handleRoot();}
else if (command == "9") {speedCar = 1023;handleRoot();}
else if (command == "S") stopRobot();
}
}


void setup() {
WiFi.softAP("NODE_mcu","1234567890");
server.on("/", handleRoot);
server.onNotFound(handleRoot);
server.begin();
pinMode(IN_1, OUTPUT); // Motor Right (IN1)
pinMode(IN_2, OUTPUT); // Motor Right (IN2)
pinMode(IN_3, OUTPUT); // Motor Left (IN3)
pinMode(IN_4, OUTPUT); // Motor Left (IN4)
// Set PWM Speed Control Pins as Outputs
pinMode(ENA, OUTPUT);
// Motor Right Speed (PWM)
pinMode(ENB, OUTPUT);
// Motor Left Speed (PWM)
pinMode(SERVO_PIN,OUTPUT);
pinMode(trig,OUTPUT);
pinMode(echo,INPUT);
pinMode(R,INPUT);
pinMode(L,INPUT);analogWriteRange(1023);
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


void loop(){
server.handleClient();
if(server.hasArg("Mode")){
command=server.arg("Mode");
if(command=="W"){
wifiCo=1,obstacleAv=0,lineFo=0,go=1;
}
else if(command=="O"){
wifiCo=0,obstacleAv=1,lineFo=0,go=1;
}
else if(command=="F"){
wifiCo=0,obstacleAv=0,lineFo=1,go=1;
}
}
if(wifiCo) wifiControllerMode();
else if(obstacleAv) obstacleAvoidingMode();
else if(lineFo) lineFollowerMode();

}