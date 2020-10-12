#include <MIDIUSB.h>


// The first pin of consecutive pins to attach our switches to
const int buttonPinBase = 2;
const uint8_t numberOfButtons = 10;

// LEDs
const int ledPinBase = 14;

// Touch INs
const int touchInPinBase = 10;
const uint8_t numberOfTouch = 2;

// An array of ints to remember the button states from the last loop
// so we can detect state transitions
int lastButtonState[numberOfButtons];

int ccState[numberOfButtons];

// The "active" state is the pressed down state. This
// might be different for different buttons..
int activeButtonState[numberOfButtons];

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

void setup() 
{
  Serial.begin(9600);
  // We use INPUT_PULLUP to use the included pullup resistors 
  // in the microcontroller
  for (int button = 0; button < numberOfButtons; ++button)
  {
    pinMode(buttonPinBase + button, INPUT_PULLUP);
    lastButtonState[button] = LOW;

    activeButtonState[button] = LOW;
  }

  for (int led = 0; led < numberOfButtons; ++led)
  {
    ccState[led] = LOW;
    pinMode(ledPinBase + led, OUTPUT);
  }

  for (int index = 0; index < 3; ++index)
  {
    for (int button = 0; button < numberOfButtons; ++button)
    {
      digitalWrite(ledPinBase + button, HIGH);
      delay(50);
    }
    
    for (int button = 0; button < numberOfButtons; ++button)
    {
      digitalWrite(ledPinBase + button, LOW);
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
      int channel = in_packet.byte1 & 0x7F;
      int controller = in_packet.byte2;

      if (controller > numberOfButtons) {
        continue;
      }
      
      int value = in_packet.byte3;

      if (value != 0) {
        ccState[controller] = value;
      } else {
        ccState[controller] = value;
      }
    }
  } while (in_packet.header != 0);
  
  // Serial.write(".\n");

  /* PROCESS BUTTONS */
  for (int button = 0; button < numberOfButtons; ++button)
  {
    if (ccState[button] != 0) {
      // digitalWrite(ledPinBase + button, HIGH);
    } else {
      // digitalWrite(ledPinBase + button, LOW);
    }
    
    int state = digitalRead(buttonPinBase + button);

    // Detect state transitions    
    if (state != lastButtonState[button]) 
    {
      // Going from low to high (on push)
      if (state == activeButtonState[button]) 
      {
        midiEventPacket_t packet = {0x0B, 0xB0, (uint8_t)button, 127};
        MidiUSB.sendMIDI(packet);
        // digitalWrite(ledPinBase + button, HIGH);
      }
      else
      {
        midiEventPacket_t packet = {0x0B, 0xB0, (uint8_t)button, 0};
        MidiUSB.sendMIDI(packet);
        // digitalWrite(ledPinBase + button, LOW);
      }
    }

    // Store away the current state for the transition detection
    lastButtonState[button] = state;
  }

  /* READ EXPRESSION PEDALS AND FADE LEDS */
  analog_in1 = 0;
  analog_in2 = 0;
  for (int index = 0; index < analog_in_window_size; ++index) 
  {
    analog_in1 += (1.0 / analog_in_window_size) * (float)analogRead(A10);
    analog_in2 += (1.0 / analog_in_window_size) * (float)analogRead(A11);

    /* SCALE LED BRIGHTNESS BY RANDOM SAMPLING AND COMPARING WITH CC VALUE */
    for (int led = 0; led < numberOfButtons; ++led) {
      if (random(0,127) < ccState[led] || lastButtonState[led] == activeButtonState[led]) {
        digitalWrite(ledPinBase + led, HIGH);
      } else {
        digitalWrite(ledPinBase + led, LOW);
      }
    }
    
    delayMicroseconds(analog_in_delta_t_us);
  }

  // Serial.print(an1); Serial.print("\t"); Serial.print(an2); Serial.print("\n");

  if (abs(analog_in1_old - analog_in1) > analog_in_thresh) {
    // Serial.print((127.0/an_max)*an1); Serial.write("\t "); Serial.print((127.0/an_max)*an2); Serial.write("\n");
    midiEventPacket_t packet = {0x0B, (uint8_t)(0xB0 | 0), numberOfButtons, min(127, floor((127.0/an_max)*analog_in1))};
    MidiUSB.sendMIDI(packet);
    analog_in1_old = analog_in1;
  }
  
  if (abs(analog_in2_old - analog_in2) > analog_in_thresh) {
    // Serial.print((127.0/an_max)*an1); Serial.write("\t "); Serial.print((127.0/an_max)*an2); Serial.write("\n");
    midiEventPacket_t packet = {0x0B, (uint8_t)(0xB0 | 1), numberOfButtons+1, min(127, floor((127.0/an_max)*analog_in2))};
    MidiUSB.sendMIDI(packet);
    analog_in2_old = analog_in2;
  }
  
  MidiUSB.flush();
  // delayMicroseconds(20000);
}
