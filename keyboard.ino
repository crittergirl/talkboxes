#include "SoftwareSerialWithHalfDuplex.h"

#define ID 'b'

#if defined(ARDUINO_AVR_MEGA2560)
const int BUTTONS[26] = {
  2, 4, 6, 8, 10, 12, // abcdef
  14, 16, 18, 20, // ghij
  22, 24, 26, 28, // klmn
  30, 32, 34, 36, // opqr
  38, 40, 42, 44, // stuv
  46, 48, 50, 52, // wxyz
};
const int LEDS[26]    = {
  3, 5, 7, 9, 11, 13, // abcdef
  15, 17, 19, 21, // ghij
  23, 25, 27, 29, // klmn
  31, 33, 35, 37, // opqr
  39, 41, 43, 45, // stuv
  47, 49, 51, 53, // wxyz
};
#elif defined(ARDUINO_AVR_UNO)
const int BUTTONS[3] = { 2 };
const int LEDS[3]    = { 3 };
#endif

#define BUTTONS_LENGTH (sizeof(BUTTONS)/sizeof(int))
#define LEDS_LENGTH BUTTONS_LENGTH

String rx = "";
bool rx_isready = false;

void rx_clear() {
  rx = "";
  rx_isready = false;
}

void rx_init() {
  rx.reserve(64);
}

long localstate = 0L; // bitmask of buttons, where button 0 is least significant bit
long remotestate = 0L;

SoftwareSerialWithHalfDuplex connection(A15, A15); // half duplex

void setup() {
  rx_init();
  connection.begin(1200);
  Serial.begin(9600);

  for (int i = 0; i < BUTTONS_LENGTH; i++) {
    pinMode(BUTTONS[i], INPUT_PULLUP);
    pinMode(LEDS[i], OUTPUT);
  }

  // on sequence
  for (int i = 0; i < LEDS_LENGTH; i++) {
    digitalWrite(LEDS[i], 1);
    delay(10);
  }
  delay(1000);
    for (int i = 0; i < LEDS_LENGTH; i++) {
    digitalWrite(LEDS[i], 0);
    delay(10);
  }
}

void loop() {
  // read serial
  while (connection.available()) {
    char c = (char) connection.read();
    if (c == '\n') {
      rx_isready = true;
      break; // don't put newline in buffer
    }
    rx += c;
  }

  // read buttons (local state)
  long newlocalstate = 0;
  for (int i = 0; i < BUTTONS_LENGTH; i++) {
    int button = BUTTONS[i];
    if (!digitalRead(button)) newlocalstate |= ((long) 1) << i;
  }
  int islocalstatechanged = 0;
  if (localstate != newlocalstate) {
    islocalstatechanged = 1;
  }
  // int islocalstatechanged = (localstate == newlocalstate); // 0 if unchanged, 1 if changed
  localstate = newlocalstate;

  // read serial (remote state)
  long newremotestate = remotestate;
  if (rx_isready) {
    String data = rx.substring(1); // ignore id character
    newremotestate = data.toInt();
    rx_clear();
  }
  int isremotestatechanged = 0;
  if (remotestate != newremotestate) {
    isremotestatechanged = 1;
  }
  remotestate = newremotestate;

  long sharedstate = localstate | remotestate;
  // sharedstate = 2147483647L >> 15;

  // write leds (shared state)
  for (int i = 0; i < LEDS_LENGTH; i++) {
    int led = LEDS[i];
    if (sharedstate & (((long) 1) << i)) {
      digitalWrite(led, HIGH);
    } else {
      digitalWrite(led, LOW);
    }
  }

  // send local state
  if (islocalstatechanged) {
    connection.print(ID);
    connection.println(localstate);
  }

  // serial debug
  if (islocalstatechanged || isremotestatechanged) {
    Serial.print(ID);
    Serial.print('\t');
    Serial.print(localstate);
    Serial.print("\t");
    Serial.print(remotestate);
    Serial.println();
  }

  // Serial.print(localstate, BIN);
  // Serial.print(" ");
  // Serial.print(remotestate, BIN);
  // Serial.print(" ");
  // Serial.print(sharedstate, BIN);
  // Serial.println();
}

// see https://docs.arduino.cc/built-in-examples/communication/SerialEvent/
// void serialEvent() {
//   if (rx_isready) return; // if buffer is already ready, don't overwrite it

//   while (Serial.available()) {
//     char c = (char) Serial.read();
//     if (c == '\n') {
//       rx_isready = true;
//       return; // don't put newline in buffer
//     }
//     rx += c;
//   }
// }
