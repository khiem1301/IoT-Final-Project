#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <SoftwareSerial.h>

// LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Servo
Servo servoIn, servoOut;

// HC-SR04 Ultrasonic Sensors
#define trigIn 4
#define echoIn 6
#define trigOut 7
#define echoOut 8

// Servo Pins
#define servoInPin 5
#define servoOutPin 9

// Bluetooth (HC-05) dùng D2 (RX), D3 (TX)
#define BLUETOOTH_RX 2
#define BLUETOOTH_TX 3
SoftwareSerial btSerial(BLUETOOTH_RX, BLUETOOTH_TX);

// System Variables
const int totalSpots = 3;
int occupiedSpots = 0;
int totalCarsEntered = 0;
const long parkingFee = 5000;

bool systemRunning = true;
bool flagIn = false;
bool flagOut = false;

// --- đo khoảng cách ---
long measureDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);
  return (duration == 0) ? 999 : duration * 0.034 / 2;
}

// --- Update LCD Display ---
void updateDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);
  if (!systemRunning) {
    lcd.print("System Stopped");
    lcd.setCursor(0, 1);
    lcd.print("Use 'o' to start");
  } else if (occupiedSpots >= totalSpots) {
    lcd.print("   SORRY :(   ");
    lcd.setCursor(0, 1);
    lcd.print(" Parking Full ");
  } else {
    lcd.print("  WELCOME!  ");
    lcd.setCursor(0, 1);
    lcd.print("Slots Left: ");
    lcd.print(totalSpots - occupiedSpots);
  }
}

// --- Send Bluetooth Data (tạm dừng servo tránh giật) ---
void sendBluetoothData() {
  long revenue = (long)totalCarsEntered * parkingFee;
  String msg = "Cars: " + String(totalCarsEntered) + " | Money: " + String(revenue);

  //  Tạm detach servo để tránh giật
  servoIn.detach();
  servoOut.detach();

  btSerial.println(msg);
  delay(50); // Cho HC-05 thời gian gửi

  //  Gắn lại servo sau khi gửi xong
  servoIn.attach(servoInPin);
  servoOut.attach(servoOutPin);
  servoIn.write(90);
  servoOut.write(90);
}

void setup() {
  btSerial.begin(9600);
  lcd.init();
  lcd.backlight();

  pinMode(trigIn, OUTPUT);
  pinMode(echoIn, INPUT);
  pinMode(trigOut, OUTPUT);
  pinMode(echoOut, INPUT);

  servoIn.attach(servoInPin);
  servoIn.write(90);
  servoOut.attach(servoOutPin);
  servoOut.write(90);

  lcd.setCursor(0, 0);
  lcd.print(" SMART PARKING ");
  lcd.setCursor(0, 1);
  lcd.print("  Initializing... ");
  delay(1500);
  updateDisplay();
}

void loop() {
  if (btSerial.available()) {
    char cmd = btSerial.read();
    if (cmd == 's') {
      systemRunning = false;
      updateDisplay();
    } else if (cmd == 'o') {
      systemRunning = true;
      updateDisplay();
    }
  }

  if (!systemRunning) return;

  long distIn = measureDistance(trigIn, echoIn);
  long distOut = measureDistance(trigOut, echoOut);

  // Phát hiện xe vào
  if (distIn < 10 && !flagIn) {
    flagIn = true;
    if (occupiedSpots < totalSpots) {
      occupiedSpots++;
      totalCarsEntered++;
      servoIn.write(0);
      delay(1200);
      servoIn.write(90);
      updateDisplay();
      sendBluetoothData();  //  Gửi khi có xe vào
    }
  }
  if (distIn > 20) flagIn = false;

  // Phát hiện xe ra
  if (distOut < 10 && !flagOut) {
    flagOut = true;
    if (occupiedSpots > 0) {
      occupiedSpots--;
      servoOut.write(0);
      delay(1200);
      servoOut.write(90);
      updateDisplay();
    }
  }
  if (distOut > 20) flagOut = false;

  delay(300);
}
