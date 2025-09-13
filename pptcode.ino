// ----------------- Libraries -----------------
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h> 
#include <Servo.h>
// ----------------- Pin Definitions & Connections -----------------
#define SS_PIN 53
#define RST_PIN 5
#define GREEN_LED 6
#define RED_LED 7
#define BUZZER 8
#define SERVO_PIN 9
// ----------------- Functions -----------------
const int NUM_USERS = 2; 
String authorizedUIDs[NUM_USERS] = {"66D2B302", "1122AABB"};
String userMessages[NUM_USERS] = {"Patient1-Take MedA 10mg", "Patient2-Take MedB 5mg"};

MFRC522 rfid(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo dispenserServo;
int scanCount = 0;

void dispense(String message, int angle) {
  Serial.println("Dispensing medication...");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Dispensing...");
  
  lcd.setCursor(0, 1);
  if (message.length() > 16) {
    lcd.print(message.substring(0, 16));
    lcd.setCursor(0, 1);
    lcd.print(message.substring(16));
  } else {
    lcd.print(message);
  }
  delay(1500);

  // Rotate the servo
  dispenserServo.write(angle);
  delay(2000);
  dispenserServo.write(0); 
}

// This function handles unauthorized access
void unauthorized_access() {
  Serial.println("Access Denied.");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Access Denied");
  
  digitalWrite(RED_LED, HIGH);
  for (int i = 0; i < 3; i++) {
    tone(BUZZER, 500, 200);
    delay(300);
  }
  digitalWrite(RED_LED, LOW);
  noTone(BUZZER);
}

// ----------------- Setup -----------------
void setup() {
  Serial.begin(9600);
  while (!Serial);

  SPI.begin();
  rfid.PCD_Init();
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready");
  lcd.setCursor(0, 1);
  lcd.print("Place your card");

  Serial.println("RFID Medicine Dispenser Initialized");
  
  
  // Servo
dispenserServo.attach(SERVO_PIN);
  dispenserServo.write(0);

  // LEDs & Buzzer
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, LOW);
  digitalWrite(BUZZER, LOW);
}

// ----------------- Loop -----------------
void loop() {
  // Wait for new card to be present
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return; 
  }

//read card uid and convert it to a string for comparison
  String uidString = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uidString += "0";
    uidString += String(rfid.uid.uidByte[i], HEX);
  }
  uidString.toUpperCase();
  Serial.print("\nCard UID Scanned: ");
  Serial.println(uidString);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Card Scanned");
  lcd.setCursor(0, 1);
  lcd.print(uidString);

  //-------------check manual uid list--------------
  bool authorized = false;
  String displayMessage = "";
  for (int i = 0; i < NUM_USERS; i++) {
    if (uidString == authorizedUIDs[i]) {
      authorized = true;
      displayMessage = userMessages[i];
      break; 
    }
  }


//----------------action--------------
  if (authorized) {
    scanCount++;

    Serial.print("Scan Count: ");
    Serial.println(scanCount);
    Serial.println("Access Granted.");
    digitalWrite(GREEN_LED, HIGH);
    tone(BUZZER, 1000, 200);
    delay(500);
    noTone(BUZZER);
    digitalWrite(GREEN_LED, LOW);
  
    if (scanCount == 1) {
      Serial.println("First scan detected. Rotating to 45 degrees.");
      dispense(displayMessage, 45); 
    } else if (scanCount == 2) {
      Serial.println("Second scan detected. Rotating to 90 degrees.");
      dispense("Second dose complete", 90); 
      scanCount = 0;
    }
    
  } else {
    unauthorized_access();
    scanCount = 0;
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  delay(2000); 

  //reset for the next scan
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Place your card");
  Serial.println("\n------------------------------------");
  Serial.println("System ready. Place your card.");
}