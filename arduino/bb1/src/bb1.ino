#include <QueueArray.h>
/*
  Stepper motors driven by shift register

 Hardware:
 * 74HC595 shift register 
 * Two stepper motors attached to the outputs of the shift register

 */
// LED
const int ledPin = 13;
//Pin connected to ST_CP of 74HC595
const int latchPin = 7;
//Pin connected to SH_CP of 74HC595
const int clockPin = 12;
////Pin connected to DS of 74HC595
const int dataPin = 8;
//Steps per wheel revolution
long fullWheelTurn = 2048;
const float wheelDia = 6.5; //cm
const float wheelSpan = 19.5; //cm
const float pi = 3.14159265359;

// Step mode and delay
int stepMode  = 1; // 0 for half steps (8 steps per motor revolution), 1 for full steps (4 steps per motor revolution)
int stepDelay = 3; // minimum is 3, higher has more torque
// 8 bits of data to send to the shift register
byte data;
// The sequence of steps to rotate the stepper motors
const byte halfSteps[8] = { 0x01, 0x03, 0x02, 0x06, 0x04, 0x0c, 0x08, 0x09 };
const byte fullSteps[4] = {       0x03,       0x06,       0x0c,       0x09 };
// The index in halfSteps of the current position of each motor
int stepperPos[2] = {0,0};
// The number of steps remaining (+/-) for both motors
long stepsRemaining[2] = {0,0};
// The queue of next remaining steps for both motors
QueueArray <long> nextMoves[2]; 
// To pause movement
boolean pause = false;

void setup() {
  //set pins to output because they are addressed in the main loop
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  
  //adjust steps per wheel turn for stepping mode
  fullWheelTurn = fullWheelTurn * (2 - stepMode);
  
  //pause until serial is connected
  //if(false) {
    while(!Serial);
    Serial.begin(9600);
    Serial.println("BoxBot1");
  //}
  
  fwd(1);
  left(5);
  right(5);
  back(1);
}

long cmToSteps(int cm) {
  long steps = fullWheelTurn * cm / (wheelDia * pi);
  Serial.print("cm to steps:");
  Serial.println(steps);
  return steps;
}
long angleToSteps(int angle) {
  long steps = (wheelSpan / wheelDia * fullWheelTurn) / (360 / angle);
  Serial.print("angle to steps:");
  Serial.println(steps);
  return steps;
}

void fwd(int cm) {
  long steps = cmToSteps(cm);
  nextMoves[0].enqueue(steps);
  nextMoves[1].enqueue(steps);
  if(Serial) {
    Serial.print("f");
    Serial.print(cm);
    Serial.println(";");
  }
}
void back(int cm) {
  long steps = cmToSteps(cm);
  nextMoves[0].enqueue(-steps);
  nextMoves[1].enqueue(-steps);
  if(Serial) {
    Serial.print("b");
    Serial.print(cm);
    Serial.println(";");
  }
}
void left(int angle) {
  long steps = angleToSteps(angle);
  nextMoves[0].enqueue(steps);
  nextMoves[1].enqueue(-steps);
  if(Serial) {
    Serial.print("l");
    Serial.print(angle);
    Serial.println(";");
  }
}
void right(int angle) {
  long steps = angleToSteps(angle);
  nextMoves[0].enqueue(-steps);
  nextMoves[1].enqueue(steps);
  if(Serial) {
    Serial.print("r");
    Serial.print(angle);
    Serial.println(";");
  }
}
void stop() {
  stepsRemaining[0] = 0;
  stepsRemaining[1] = 0;
  while(!nextMoves[0].isEmpty()) {
    nextMoves[0].dequeue();
  }
  while(!nextMoves[1].isEmpty()) {
    nextMoves[1].dequeue();
  }
  if(Serial) {
    Serial.println("s;");
  }
}

void blink() {
  digitalWrite(ledPin, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(10);               // wait for a second
  digitalWrite(ledPin, LOW);    // turn the LED off by making the voltage LOW
}

void consumeMotorData(int i) {
  int datashift = i*4;
  stepperPos[i] = abs(stepsRemaining[i] % (stepMode==0 ? 8 : 4));
  
  if(stepsRemaining[i]==0) {
    blink();
    if(nextMoves[i].count()>0) {
      // finished moving stepper i, get next move
      stepsRemaining[i] = nextMoves[i].dequeue();
    }
    // stepper position might be too wrong now so calculate the offset
    // TODO: use stepperPos[i] - (stepsRemaining[i] % 8);
    data = data + (0 <<datashift);
    
  } else if(stepsRemaining[i]>0) {
    // reverse direction
    stepsRemaining[i] += -1;
    if(stepMode==0) {
      data = data + (halfSteps[stepperPos[i]] <<datashift);
    } else {
      data = data + (fullSteps[stepperPos[i]] <<datashift);
    }
    
  } else if(stepsRemaining[i]<0) {
    // forward direction
    stepsRemaining[i] += 1;
    if(stepMode==0) {
      data = data + (halfSteps[7-stepperPos[i]] <<datashift);
    } else {
      data = data + (fullSteps[3-stepperPos[i]] <<datashift);
    }
    
  }
}

void drive() {
  data = 0x00;

  if(!pause) {
    // populate the data to send to the shift register
    consumeMotorData(0);
    consumeMotorData(1);
  }
  
  digitalWrite(latchPin, 0);
  shiftOut(dataPin, clockPin, data);
  digitalWrite(latchPin, 1);

  delay(stepDelay);
}

void readCommands() {
  char inByte = ' ';
  char cmd = 's';
  int  val = 0;
  while(Serial.available() > 0) {
    // read the incoming byte:
    inByte = Serial.read();
    Serial.print(inByte);
    switch (inByte) {
      case 'f':
      case 'b':
      case 'l':
      case 'r':
        // these movement commands are followed by an int
        val = Serial.parseInt();
        Serial.print(val);
      case 's':
      case 'p':
      case 'g':
        // remember the command
        cmd = inByte;
        break;
      case ';':
         Serial.println(inByte);
        // execute the command when terminated with ;
        switch(cmd) {
          case 'f':
            fwd(val);
            break;
          case 'b':
            back(val);
            break;
          case 'l':
            left(val);
            break;
          case 'r':
            right(val);
            break;
          case 's':
            stop();
            break;
          case 'p':
            pause = true;
            break;
          case 'g':
            pause = false;
            break;
        }
        break;
    }
  }
}

void loop() {
  // Move motors
  drive();
  
  // Get next moves
  readCommands();

}

void shiftOut(int myDataPin, int myClockPin, byte myDataOut) {
  // This shifts 8 bits out MSB first, 
  //on the rising edge of the clock,
  //clock idles low

  //internal function setup
  int i=0;
  int pinState;
  pinMode(myClockPin, OUTPUT);
  pinMode(myDataPin, OUTPUT);

  //clear everything out just in case to
  //prepare shift register for bit shifting
  digitalWrite(myDataPin, 0);
  digitalWrite(myClockPin, 0);

  //for each bit in the byte myDataOutï¿½
  //NOTICE THAT WE ARE COUNTING DOWN in our for loop
  //This means that %00000001 or "1" will go through such
  //that it will be pin Q0 that lights. 
  for (i=7; i>=0; i--)  {
    digitalWrite(myClockPin, 0);

    //if the value passed to myDataOut and a bitmask result 
    // true then... so if we are at i=6 and our value is
    // %11010100 it would the code compares it to %01000000 
    // and proceeds to set pinState to 1.
    if ( myDataOut & (1<<i) ) {
      pinState= 1;
    }
    else {  
      pinState= 0;
    }

    //Sets the pin to HIGH or LOW depending on pinState
    digitalWrite(myDataPin, pinState);
    //register shifts bits on upstroke of clock pin  
    digitalWrite(myClockPin, 1);
    //zero the data pin after shift to prevent bleed through
    digitalWrite(myDataPin, 0);
  }

  //stop shifting
  digitalWrite(myClockPin, 0);
}


