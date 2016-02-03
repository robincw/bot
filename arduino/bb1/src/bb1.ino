#include <QueueArray.h>
#include <Servo.h>

/*
  Stepper motors driven by shift register

 Hardware:
 * 74HC595 shift register 
 * Two stepper motors attached to the outputs of the shift register
 * Servo to point sensors
 * Two sharp infrared range finder sensors at 90 degrees

 */
// LED
const int ledPin = 13;
//Pin connected to ST_CP of 74HC595
const int latchPin = 7;
//Pin connected to SH_CP of 74HC595
const int clockPin = 12;
////Pin connected to DS of 74HC595
const int dataPin = 8;

const int oaFaceServo   = 6;  // [pwm]
const int iaIRranger1   = A5; //
const int iaIRranger2   = A4; //

Servo faceServo;
int face_angle  = 90;

//Steps per wheel revolution
long fullWheelTurn = 2048;
const float wheelDia = 6.5; //cm
const float wheelSpan = 19.4; //cm
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
// The commands queued to be processed
QueueArray <String> commands;
// To pause movement
boolean pause = false;
 
void face(int angle) {
  if(face_angle != angle) {
    if(angle<0) angle = 0;
    if(angle>180) angle = 180;
    faceServo.write(angle);
    delay(abs(face_angle-angle)*2);
    face_angle = angle;
  }
}

void isort(double *a, int n) //  *a is an array pointer function
{
  for (int i = 1; i < n; ++i)
  {
    double j = a[i];
    int k;
    for (k = i - 1; (k >= 0) && (j < a[k]); k--)
    {
      a[k + 1] = a[k];
    }
    a[k + 1] = j;
  }
}

double readAnalogue(int iaPin, int numReadings = 8, int interval = 2, double aref = 4.26) {
  double volts   = 0;
  double readings[numReadings];
  for(int index = 0; index < numReadings; index++) // take x number of readings from the sensor and average them
  {
    volts   = analogRead(iaPin)*(aref/1024.0);  // value from sensor * (5/1024) - if running 3.3.volts then change 5 to 3.3
    readings[index] = volts;
    delay(interval);
  }
  isort(&readings[0], numReadings);
  double median = numReadings % 2 ? readings[numReadings / 2] : (readings[numReadings / 2 - 1] + readings[numReadings / 2]) / 2;
  return median;
}

int getDistance(int iaIRranger, int numReadings = 5, int interval = 2) {
    int distance  = 0;
    double median = readAnalogue(iaIRranger, numReadings, interval);
    if(iaIRranger==iaIRranger1)
    {
      median = 65*pow(median, -1.05); // worked out from graph 65 = theretical distance / (1/Volts)S - luckylarry.co.uk
    }
    else if(iaIRranger==iaIRranger2)
    {
      median = 60.495 * pow(median,-1.1904); //65*pow(volts, -1.10);  // worked out from graph 65 = theretical distance / (1/Volts)S - luckylarry.co.uk
    }
    distance = (int) median;
    distance = distance>150 ? 150 : distance;
    return distance;
}

String rangers() {
  String r = String((405-face_angle)%360);
  r=r+"=";
  r=r+String((getDistance(iaIRranger1) + getDistance(iaIRranger1))/2);
  r=r+",";
  r=r+String((495-face_angle)%360);
  r=r+"=";
  r=r+String((getDistance(iaIRranger2) + getDistance(iaIRranger2))/2);
  return r;
}

String scan(int angle) {
  commands.enqueue("s"+String(angle));
  face(angle);
  String r = rangers();
  finishCommand();
  return r;
}

String scanTo(int angle) {
  commands.enqueue("t"+String(angle));
  int step = angle < face_angle ? -1 : 1;
  String r = rangers();
  while(face_angle != angle) {
    face(face_angle + step);
    r = r + "," + rangers();
  }
  finishCommand();
  return r;
}

void setup() {
  //set pins to output because they are addressed in the main loop
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  
  // Sensors
  faceServo.attach(oaFaceServo);
  face(face_angle);

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
  //Serial.print("cm to steps:");
  //Serial.println(steps);
  return steps;
}
long angleToSteps(int angle) {
  long steps = (wheelSpan / wheelDia * fullWheelTurn) / (360 / angle);
  //Serial.print("angle to steps:");
  //Serial.println(steps);
  return steps;
}

void fwd(int cm) {
  commands.enqueue("f"+String(cm));
  long steps = cmToSteps(cm);
  nextMoves[0].enqueue(steps);
  nextMoves[1].enqueue(steps);
}
void back(int cm) {
  commands.enqueue("b"+String(cm));
  long steps = cmToSteps(cm);
  nextMoves[0].enqueue(-steps);
  nextMoves[1].enqueue(-steps);
}
void left(int angle) {
  commands.enqueue("l"+String(angle));
  long steps = angleToSteps(angle);
  nextMoves[0].enqueue(steps);
  nextMoves[1].enqueue(-steps);
}
void right(int angle) {
  commands.enqueue("r"+String(angle));
  long steps = angleToSteps(angle);
  nextMoves[0].enqueue(-steps);
  nextMoves[1].enqueue(steps);
}
void halt() {
  commands.enqueue("halt");
  stepsRemaining[0] = 0;
  stepsRemaining[1] = 0;
  while(!nextMoves[0].isEmpty()) {
    nextMoves[0].dequeue();
  }
  while(!nextMoves[1].isEmpty()) {
    nextMoves[1].dequeue();
  }
}

void blink() {
  digitalWrite(ledPin, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(10);               // wait for a second
  digitalWrite(ledPin, LOW);    // turn the LED off by making the voltage LOW
}

void finishCommand() {
  String command = commands.dequeue();
  if(Serial) {
    Serial.println(command);
  }
  blink();
}

bool consumeMotorData(int i) {
  bool finishedCommand = false;
  int datashift = i*4;
  stepperPos[i] = abs(stepsRemaining[i] % (stepMode==0 ? 8 : 4));
  
  if(stepsRemaining[i]==0) {
    finishedCommand = true;
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
  return finishedCommand;
}

void drive() {
  data = 0x00;

  if(!pause) {
    // populate the data to send to the shift register
    bool finishedCommand = consumeMotorData(0);
    finishedCommand = consumeMotorData(1) || finishedCommand;
    if(finishedCommand) {
      finishCommand();
    }
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
    switch (inByte) {
      case 'f':
      case 'F':
      case 'b':
      case 'B':
      case 'l':
      case 'L':
      case 'r':
      case 'R':
      case 's':
      case 'S':
      case 't':
      case 'T':
        // these commands are followed by an int
        val = Serial.parseInt();
      case 'h':
      case 'H':
      case 'p':
      case 'P':
      case 'g':
      case 'G':
        // remember the command
        cmd = inByte;
        break;
      case ';':
      case ' ':
      case '\n':
        // execute the command when terminated with ;
        switch(cmd) {
          case 'f':
          case 'F':
            fwd(val);
            break;
          case 'b':
          case 'B':
            back(val);
            break;
          case 'l':
          case 'L':
            left(val);
            break;
          case 'r':
          case 'R':
            right(val);
            break;
          case 'h':
          case 'H':
            halt();
            break;
          case 'p':
          case 'P':
            pause = true;
            break;
          case 'g':
          case 'G':
            pause = false;
            break;
          case 's':
          case 'S':
            Serial.println(scan(val));
            break;
          case 't':
          case 'T':
            Serial.println(scanTo(val));
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


