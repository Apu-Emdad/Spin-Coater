#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Servo.h>

// Set up the I2C LCD display
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Define the pins for the ESC and IR sensor
#define ESC_PIN 9
#define IR_SENSOR_PIN 2

// Define the rows and columns for the keypad
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {5, 4, 3, 2}; // connect to the row pinouts of the keypad
byte colPins[COLS] = {A0, A1, A2, A3}; // connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

Servo esc;

unsigned long duration = 0;
unsigned int rpm = 0;
unsigned int currentRPM = 0;
unsigned long previousMillis = 0;

// Set the maximum RPM limit
const unsigned int MAX_RPM = 5000;

// Set the minimum duration limit (in minutes)
const unsigned int MIN_DURATION = 1;

void setup() {
  // Initialize the LCD
  lcd.begin();
  lcd.backlight();

  // Display the heading
  lcd.setCursor(0, 0);
  lcd.print("Low Cost Spin");
  lcd.setCursor(0, 1);
  lcd.print("   Coater");

  delay(2000); // Display the heading for 2 seconds
  lcd.clear();

  // Display the max RPM on the third row
  lcd.setCursor(0, 2);
  lcd.print("MAX RPM=5000");

  // Display the minimum duration on the fourth row
  lcd.setCursor(0, 3);
  lcd.print("MIN DUR=1 min");
  // Initialize the ESC
  esc.attach(ESC_PIN, 1000, 2000); // attaches the ESC on pin 9, min pulse width = 1000, max pulse width = 2000
  esc.writeMicroseconds(1000); // Start at zero throttle

  // Initialize the IR sensor pin
  pinMode(IR_SENSOR_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(IR_SENSOR_PIN), countRPM, FALLING);

  lcd.setCursor(0, 0);
  lcd.print("Enter RPM:");
  rpm = getRPMInput();

  lcd.setCursor(0, 1);
  lcd.print("Enter Duration:");
  duration = getDurationInput();

  lcd.setCursor(0, 2);
  lcd.print("* Start, # Cancel");
}

void loop() {
  char key = keypad.getKey();

  if (key == '*') {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Running...");
    runCoater(rpm, duration);
  } else if (key == '#') {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Cancelled");
    esc.writeMicroseconds(1000); // Stop motor
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Enter RPM:");
    rpm = getRPMInput();
    lcd.setCursor(0, 1);
    lcd.print("Enter Duration:");
    duration = getDurationInput();
    lcd.setCursor(0, 2);
    lcd.print("* Start, # Cancel");
  }
}

void runCoater(int targetRPM, unsigned long runTime) {
  unsigned long startTime = millis();
  unsigned long currentTime = 0;

  esc.writeMicroseconds(map(targetRPM, 0, 10000, 1000, 2000));

  while (currentTime < runTime) {
    currentTime = millis() - startTime;

    lcd.setCursor(0, 1);
    lcd.print("Current RPM: ");
    lcd.print(currentRPM);

    if (currentRPM > targetRPM) {
      esc.writeMicroseconds(esc.readMicroseconds() - 10);
    } else if (currentRPM < targetRPM) {
      esc.writeMicroseconds(esc.readMicroseconds() + 10);
    }
  }

  esc.writeMicroseconds(1000); // Stop motor
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Completed");
  delay(2000);
  lcd.clear();
}

unsigned int getRPMInput() {
  unsigned int input = 0;
  char key = keypad.getKey();
  while (key != '*' && key != '#') {
    if (key >= '0' && key <= '9') {
      input = input * 10 + (key - '0');
      lcd.print(key);
    }
    key = keypad.getKey();
  }

  // Check if the RPM exceeds the MAX_RPM
  if (input > MAX_RPM) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MAX RPM is 5000");
    delay(1000); // Show the message for 1 second
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Enter RPM:");
    input = getRPMInput(); // Prompt the user to enter RPM again
  }

  return input;
}

unsigned long getDurationInput() {
  unsigned long input = 0;
  char key = keypad.getKey();
  while (key != '*' && key != '#') {
    if (key >= '0' && key <= '9') {
      input = input * 10 + (key - '0');
      lcd.print(key);
    }
    key = keypad.getKey();
  }

  // Check if the duration is less than the MIN_DURATION
  if (input < MIN_DURATION) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MIN DURATION is 1");
    delay(1000); // Show the message for 1 second
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Enter Duration:");
    input = getDurationInput(); // Prompt the user to enter duration again
  }

  return input * 60000; // Convert minutes to milliseconds
}

void countRPM() {
  static unsigned long lastPulseTime = 0;
  unsigned long currentTime = millis();
  currentRPM = 60000 / (currentTime - lastPulseTime);
  lastPulseTime = currentTime;
}
