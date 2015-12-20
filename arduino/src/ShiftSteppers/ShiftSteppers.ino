#include <QueueArray.h>
/*
  Stepper motors driven by shift register

 Hardware:
 * 74HC595 shift register 
 * Two stepper motors attached to the outputs of the shift register

 */
// LED
int ledPin = 13;
//Pin connected to ST_CP of 74HC595
int latchPin = 7;
//Pin connected to SH_CP of 74HC595
int clockPin = 12;
////Pin connected to DS of 74HC595
int dataPin = 8;
// Step mode and delay
int stepMode  = 1; // 0 for half steps, 1 for full
int stepDelay = 3; // minimum is 3, higher has more torque
// 8 bits of data to send to the shift register
byte data;
// The sequence of steps to rotate the stepper motors
byte halfSteps[8] = { 0x01, 0x03, 0x02, 0x06, 0x04, 0x0c, 0x08, 0x09 };
byte fullSteps[4] = {       0x03,       0x06,       0x0c,       0x09 };
// The index in halfSteps of the current position of each motor
int stepperPos[2] = {0,0};
// The number of steps remaining (+/-) for both motors
int stepsRemaining[2] = {0,0};
// The queue of next remaining steps for both motors
QueueArray <int> nextMoves[2]; 
// To pause movement
boolean pause = false;

void setup() {
  //set pins to output because they are addressed in the main loop
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  //pause until serial is connected
  while(!Serial);
  Serial.begin(9600);
  for(int m=0; m<1; m++) {
    nextMoves[0].enqueue(1024);
    nextMoves[1].enqueue(512);
    
    nextMoves[1].enqueue(-512);
    nextMoves[0].enqueue(-1024);
  }
}

void loop() {  
  data = 0x00;
  Serial.print("[");
  for(int i=0; i<2; i++) {
    // maybe toggle pause
    //if(random(1000)==42) pause = !pause;
    
    if(!pause) {
      int datashift = i*4;
      Serial.print(datashift);
      Serial.print(":");
      stepperPos[i] = abs(stepsRemaining[i] % (stepMode==0 ? 8 : 4));
      
      // Get next moves
      if(false && nextMoves[i].count()<5) {
        int n = random(-1024,1024);
        //Serial.println(nextMoves[i].count());
        nextMoves[i].enqueue(n);
      }
      if(stepsRemaining[i]==0 && nextMoves[i].count()>0) {
        // finished moving stepper i, get next move
        stepsRemaining[i] = nextMoves[i].dequeue();
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
    Serial.print(stepsRemaining[i]);
    Serial.print(", ");
    Serial.print("0x"); if( data < 0x10){ Serial.print("0");} Serial.print(data, HEX);
    Serial.print(", ");
  
  }
  Serial.print("0x"); if( data < 0x10){ Serial.print("0");} Serial.print(data, HEX);
  Serial.println("]");
  
  digitalWrite(latchPin, 0);
  shiftOut(dataPin, clockPin, data);
  digitalWrite(latchPin, 1);
  
  delay(stepDelay);
}



// the heart of the program
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


