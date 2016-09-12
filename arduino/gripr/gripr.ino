#include <Servo.h>

Servo servos[5];
int pins[5];
int offsets[5];
double w,x,y,z;
double a,b,c,d,e;
const double B = 60;
const double C = 65;

void setup() {
  x=55;
  y=0;
  z=40;
  while(!Serial);
  Serial.begin(9600);
  Serial.println("Gripr");
  Serial.println("Commands: x,y,z,w,g followed by int, ended by ; or space or newline");
  initServos();
  pins[0]=3;
  pins[1]=5;
  pins[2]=6;
  pins[3]=11;
  pins[4]=9;
  offsets[0]=0;
  offsets[1]=-16;
  offsets[2]=30;
  offsets[3]=0;
  offsets[4]=5;
  for(int s=0; s<5; s++) {
    servos[s].attach(pins[s]);
  }
  
  for(double j=-100; j<100; j++) {
    x=j;y=50;z=40;
    moveArm();
    setArm();
    delay(50);
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
  a = a + x;
  
  b = b + y;
  
  c = z + (180 - b);

  d = w + (180 - a);
  
        Serial.print("{x:");
        Serial.print(x);
        Serial.print(",y:");
        Serial.print(y);
        Serial.print(",z:");
        Serial.print(z);
        Serial.println("}");
        Serial.print("{a:");
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
}

int rad2deg(double rad) {
  return (rad * 4068) / 71;
}
void initServos() {
  a=(90);
  b=(0);
  c=(0);
  d=(90);
  e=(45);
}

void base(double deg) {
  mov(0, 180-deg);
}
void shoulder(double deg) {
  mov(1, deg);
}
void elbow(double deg) {
  mov(2, deg);
}
void wrist(double deg) {
  mov(3, deg);
}
void grip(double deg) {
  mov(4, deg);
}
void mov(int s, double deg) {
  servos[s].write(deg+offsets[s]);
}

void readCommands() {
  char inByte = ' ';
  char cmd = 's';
  double  val = 0;
  while(Serial.available() > 0) {
    // read the incoming byte:
    inByte = Serial.read();
    switch (inByte) {
      case 'a':
      case 'b':
      case 'c':
      case 'd':
      case 'e':
      case 'w':
      case 'x':
      case 'y':
      case 'z':
      case 'g':
        // these commands are followed by an int
        val = (double)Serial.parseInt();
        // remember the command
        cmd = inByte;
        break;
      case ';':
      case ' ':
      case '\n':
        // execute the command when terminated with ;
        switch (cmd) {
          case 'w':
            w = val;
            break;
          case 'x':
            x = val;
            break;
          case 'y':
            y = val;
            break;
          case 'z':
            z = val;
            break;
          case 'g':
            e = (val);
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

