#include <Servo.h>

Servo servos[5];
int pins[5];
int offsets[5];
int w,x,y;
int l,m,r;
int a = 90;
int b = 2;
int c = 0;
int z = 180;
int d = 90;
int e = 45;

void setup() {
  while(!Serial);
  Serial.begin(9600);
  Serial.println("Gripr");
  Serial.println("Commands: x,y,z,w,g followed by int, ended by ; or space or newline");
  pins[0]=3;
  pins[1]=5;
  pins[2]=6;
  pins[3]=11;
  pins[4]=9;
  offsets[0]=0;
  offsets[1]=0;
  offsets[2]=30;
  offsets[3]=0;
  offsets[4]=5;
  for(int s=0; s<5; s++) {
    servos[s].attach(pins[s]);
  }
}

void loop() {
  readCommands();
  moveArm();
}

void moveArm() {
  base(a);
  shoulder(b);
  elbow(c);
  wrist(d);
  grip(e);
}
void setArm() {
  if(m == 1) {
    w = w + x;
  } else {    
    if(r == 1) {
      z = max(0, min(z + y, 180));
    } else {
      b = max(2, min(b - y, 130));
    }
    a = max(0, min(a + x, 180));
    c = max(0, min((180 - b) -z, 180));
  }
  if(b + c > 100) {
    d = max(0, min(w + (180 - a), 180));
  }
  
  if(l == 1) {
    e = 0;
  } else {
    e = 45;
  }
/*
  Serial.print("{x:");
  Serial.print(x);
  Serial.print(",y:");
  Serial.print(y);
  Serial.print(",l:");
  Serial.print(l);
  Serial.print(",m:");
  Serial.print(m);
  Serial.print(",r:");
  Serial.print(r);
  Serial.print(",a:");
  Serial.print(a);
  Serial.print(",b:");
  Serial.print(b);
  Serial.print(",c:");
  Serial.print(c);
  Serial.print(",d:");
  Serial.print(d);
  Serial.print(",e:");
  Serial.print(e);
  Serial.println("}");
*/
}

int rad2deg(int rad) {
  return (rad * 4068) / 71;
}

void base(int deg) {
  mov(0, 180-deg);
}
void shoulder(int deg) {
  mov(1, deg);
}
void elbow(int deg) {
  mov(2, deg);
}
void wrist(int deg) {
  mov(3, deg);
}
void grip(int deg) {
  mov(4, deg);
}
void mov(int s, int deg) {
  servos[s].write(deg+offsets[s]);
  delay(2);
}

void readCommands() {
  char inByte = ' ';
  char cmd = 's';
  int  val = 0;
  while(Serial.available() > 0) {
    // read the incoming byte:
    inByte = Serial.read();
    switch (inByte) {
      case 'a':
      case 'b':
      case 'c':
      case 'd':
      case 'e':
      
      case 'x':
      case 'y':
      case 'z':
      case 'l':
      case 'm':
      case 'r':
        // these commands are followed by an int
        val = Serial.parseInt();
        // remember the command
        cmd = inByte;
        break;
      case ';':
      case ' ':
      case '\n':
        // execute the command when terminated with ;
        switch (cmd) {
          case 'x':
            x = max(-1, min(val, 1));
            break;
          case 'y':
            y = max(-1, min(val, 1));
            break;
          case 'z':
            z = val;
            break;
          case 'l':
            l = val;
            break;
          case 'm':
            m = (val);
            break;
          case 'r':
            r = (val);
            break;
        }
        setArm();
        
        switch (cmd) {
          case 'a':
            a = val;
            break;
          case 'b':
            b = val;
            break;
          case 'c':
            c = val;
            break;
          case 'd':
            d = val;
            break;
          case 'e':
            e = val;
            break;
        }
        break;
    }
  }
}

