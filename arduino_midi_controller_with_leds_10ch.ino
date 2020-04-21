#include <MIDIUSB.h>


// The first pin of consecutive pins to attach our switches to
const int buttonPinBase = 2;
const uint8_t numberOfButtons = 10;

// LEDs
const int ledPinBase = 14;
const uint8_t numberOfLEDs = 10;

// Touch INs
const int touchInPinBase = 10;
const uint8_t numberOfTouch = 2;

// An array of ints to remember the button states from the last loop
// so we can detect state transitions
int lastButtonState[numberOfButtons];// = { LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW, LOW };

int isButtonProgramChange[numberOfButtons] = { 1 1 1 1 1 0 0 0 0 };

int isButtonMomentary[numberOfButtons] = { 0 0 0 0 0 0 0 1 1 1 };

// An array of ints to remember the last CC we sent, so we can send 
// the corresponding different one on the next press
int lastMidiState[numberOfButtons];// = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

// The "active" state is the pressed down state. This
// might be different for different buttons..
int activeButtonState[numberOfButtons];

const int num_an_avg = 50;
int avg_index = 0;

float an1 = 0;
float an2 = 0;

float an1_old = 0;
float an2_old = 0;

float an_thresh = 4.0;

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
    lastMidiState[button] = 0;

    activeButtonState[button] = LOW;
  }
  
  for (int button = 0; button < numberOfLEDs; ++button)
  {
    pinMode(ledPinBase + button, OUTPUT);
    digitalWrite(ledPinBase + button, HIGH);
  }
  
  delay(500);
  
  for (int button = 0; button < numberOfLEDs; ++button)
  {
    digitalWrite(ledPinBase + button, LOW);
  }

  pinMode(A10, INPUT);
  pinMode(A11, INPUT);
}

void loop() {
  // Serial.write(".\n");
  for (int button = 0; button < numberOfButtons; ++button)
  {
    int state = digitalRead(buttonPinBase + button);

    // Detect state transitions    
    if (state != lastButtonState[button]) 
    {
      // Going from low to high (on push)
      if (state == activeButtonState[button]) 
      {
        // Check the last CC value we sent so we can send the right one now
        if (lastMidiState[button] == 0)
        {
          midiEventPacket_t packet = {0x0B, (uint8_t)(0xB0 | button), 0, 127};
          MidiUSB.sendMIDI(packet);
          lastMidiState[button] = 127;
          digitalWrite(ledPinBase + button, HIGH);
        }
        else
        {
          midiEventPacket_t packet = {0x0B, (uint8_t)(0xB0 | button), 0, 0};
          MidiUSB.sendMIDI(packet);
          lastMidiState[button] = 0;
          digitalWrite(ledPinBase + button, LOW);
        }
        MidiUSB.flush();
      } 
    }

    // Store away the current state for the transition detection
    lastButtonState[button] = state;
  }

  an1 = 0;
  an2 = 0;
  for (int index = 0; index < num_an_avg; ++index) {

    an1 += (1.0 / num_an_avg) * (float)analogRead(A10);
    an2 += (1.0 / num_an_avg) * (float)analogRead(A11);

    delayMicroseconds(50);
  }

  // Serial.print(an1); Serial.print("\t"); Serial.print(an2); Serial.print("\n");

  if (abs(an1_old - an1) > an_thresh) {
    // Serial.print((127.0/an_max)*an1); Serial.write("\t "); Serial.print((127.0/an_max)*an2); Serial.write("\n");
    midiEventPacket_t packet = {0x0B, (uint8_t)(0xB0 | 0), 0, min(127, floor((127.0/an_max)*an1))};
    MidiUSB.sendMIDI(packet);
    an1_old = an1;
  }
  
  if (abs(an2_old - an2) > an_thresh) {
    // Serial.print((127.0/an_max)*an1); Serial.write("\t "); Serial.print((127.0/an_max)*an2); Serial.write("\n");
    midiEventPacket_t packet = {0x0B, (uint8_t)(0xB0 | 1), 0, min(127, floor((127.0/an_max)*an2))};
    MidiUSB.sendMIDI(packet);
    an2_old = an2;
  }
}
