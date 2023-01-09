#include <MIDIUSB.h>
#include <EEPROM.h>

const uint8_t numberOfButtons = 10;

const int buttonPins[]      = {  2,  3,   4,  5,   6,  7,   8,  9,  10, 11 };
const int ledPins[]         = { 14, 15,  16, 17,  18, 19,  20, 21,  22, 23 };
uint8_t isButtonMomentary[] = {  0,  0,   0,  0,   0,  0,   0,  0,   0,  0 };

// The "active" state is the pressed down state. This
// might be different for different buttons..
const int activeButtonState[] = { LOW, LOW,  LOW, LOW,  LOW, LOW,  LOW, LOW,  LOW, LOW };

// EEPROM magic number
const uint64_t magic = 0xdeadbeef;

enum { NORMAL = 0, PROGRAMMING_MOMENTARY } mode;

// An array of ints to remember the button states from the last loop
// so we can detect state transitions
int lastButtonState[numberOfButtons];
int buttonState[numberOfButtons];

const int analog_in_window_size = 500;
const int analog_in_delta_t_us = 5;
int avg_index = 0;

float analog_in1 = 0;
float analog_in2 = 0;

float analog_in1_old = 0;
float analog_in2_old = 0;

float analog_in_thresh = 4.0;

const float t0 = 0.1;

const float an_max = 560.0;
const float an_min = 0.1;

void saveSetup()
{
  EEPROM.put(0, magic);
  for (int button = 0; button < numberOfButtons; ++button)
  {
    EEPROM.put(8 + button, (uint8_t)0);
  }
}

void setup() 
{
  Serial.begin(9600);

  mode = NORMAL;
  
  // Check for magic number in EEPROM
  long long f;
  EEPROM.get(0, f);
  if (f != magic)
  {
    saveSetup();
  }
  
  for (int button = 0; button < numberOfButtons; ++button)
  {
    int value = EEPROM.read(8 + button);
    isButtonMomentary[button] = value;
  }
  
  // We use INPUT_PULLUP to use the included pullup resistors 
  // in the microcontroller
  for (int button = 0; button < numberOfButtons; ++button)
  {
    pinMode(buttonPins[button], INPUT_PULLUP);
    lastButtonState[button] = LOW;
    
    buttonState[button] = 0;
    pinMode(ledPins[button], OUTPUT);
  }

  for (int led = 0; led < numberOfButtons; ++led)
  {
  }

  for (int index = 0; index < 3; ++index)
  {
    for (int button = 0; button < numberOfButtons; ++button)
    {
      digitalWrite(ledPins[button], HIGH);
      delay(50);
    }
    
    for (int button = 0; button < numberOfButtons; ++button)
    {
      digitalWrite(ledPins[button], LOW);
      delay(50);
    }
  }
  
  pinMode(A10, INPUT);
  pinMode(A11, INPUT);
}

void loop() {
  /* MIDI IN */
  midiEventPacket_t in_packet;
  
  do {
    in_packet = MidiUSB.read();
    if (in_packet.header == 0x0B) {
      // int channel = in_packet.byte1 & 0x7F;
      int controller = in_packet.byte2;

      if (controller > numberOfButtons) {
        continue;
      }
      
      int value = in_packet.byte3;

      if (value != 0) {
        buttonState[controller] = value;
      } else {
        buttonState[controller] = value;
      }
    }
  } while (in_packet.header != 0);
  
  // Serial.write(".\n");

  /* PROCESS BUTTONS */
  for (int button = 0; button < numberOfButtons; ++button)
  {
    int state = digitalRead(buttonPins[button]);

    if (isButtonMomentary[button]) {
      if (state != lastButtonState[button]) 
      {
        if (state == activeButtonState[button]) 
        {
          midiEventPacket_t packet = {0x0B, 0xB0, (uint8_t)button, 127};
          MidiUSB.sendMIDI(packet);
          buttonState[button] = 127;
        } else {
          midiEventPacket_t packet = {0x0B, 0xB0, (uint8_t)button, 0};
          MidiUSB.sendMIDI(packet);
          buttonState[button] = 0;
        }
      }
    } else { // isButtonMomentary == false
      if (state != lastButtonState[button]) 
      {
        if (state == activeButtonState[button]) 
        {
          if (buttonState[button] == 0) {
            midiEventPacket_t packet = {0x0B, 0xB0, (uint8_t)button, 127};
            MidiUSB.sendMIDI(packet);
            buttonState[button] = 127;
          } else {
            midiEventPacket_t packet = {0x0B, 0xB0, (uint8_t)button, 0};
            MidiUSB.sendMIDI(packet);
            buttonState[button] = 0;
          }
        }
      }
    }

    // Store away the current state for the transition detection
    lastButtonState[button] = state;
  }

  if (buttonState[0] != 0 && buttonState[9] != 0 && mode == NORMAL) {
    mode = PROGRAMMING_MOMENTARY;
  }

  if (mode == PROGRAMMING_MOMENTARY) {
    for (int button = 0; button < numberOfButtons; ++button)
    {
      if (buttonState[button] != 0)
      {
        isButtonMomentary[button] = !isButtonMomentary[button];
        mode = NORMAL;
      }
    }
  }

  /* READ EXPRESSION PEDALS AND FADE LEDS */
  analog_in1 = 0;
  analog_in2 = 0;
  for (int index = 0; index < analog_in_window_size; ++index) 
  {
    analog_in1 += (1.0 / analog_in_window_size) * (float)analogRead(A10);
    analog_in2 += (1.0 / analog_in_window_size) * (float)analogRead(A11);

    /* SCALE LED BRIGHTNESS BY RANDOM SAMPLING AND COMPARING WITH CC VALUE */
    for (int button = 0; button < numberOfButtons; ++button) {
      if (random(0,127) < buttonState[button] || lastButtonState[button] == activeButtonState[button]) {
        digitalWrite(ledPins[button], HIGH);
      } else {
        digitalWrite(ledPins[button], LOW);
      }
    }
    
    delayMicroseconds(analog_in_delta_t_us);
  }

  // Serial.print(an1); Serial.print("\t"); Serial.print(an2); Serial.print("\n");

  if (abs(analog_in1_old - analog_in1) > analog_in_thresh) {
    // Serial.print((127.0/an_max)*an1); Serial.write("\t "); Serial.print((127.0/an_max)*an2); Serial.write("\n");
    midiEventPacket_t packet = {0x0B, (uint8_t)(0xB0 | 0), numberOfButtons, min((uint8_t)127, (uint8_t)floor((127.0/an_max)*analog_in1))};
    MidiUSB.sendMIDI(packet);
    analog_in1_old = analog_in1;
  }
  
  if (abs(analog_in2_old - analog_in2) > analog_in_thresh) {
    // Serial.print((127.0/an_max)*an1); Serial.write("\t "); Serial.print((127.0/an_max)*an2); Serial.write("\n");
    midiEventPacket_t packet = {0x0B, (uint8_t)(0xB0 | 1), numberOfButtons+1, min((uint8_t)127, (uint8_t)floor((127.0/an_max)*analog_in2))};
    MidiUSB.sendMIDI(packet);
    analog_in2_old = analog_in2;
  }
  
  MidiUSB.flush();
  // delayMicroseconds(20000);
}
