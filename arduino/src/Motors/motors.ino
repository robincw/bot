#include <Wire.h>
//#include <ps2.h>
#include <Servo.h>

#define SLAVE_ADDRESS 0x04

//---------------------------------------------------------------------------
// Pin numbers
const int MOUSE_DATA    = 3;//0;  // [Serial RX]
const int MOUSE_CLOCK   = 2;//1;  // [Serial TX]
//  WiShield                = 2;  // [INT0 Interrupt]
//              = 3;  // [INT1 Interrupt, pwm]
const int odMotor1dir   = 7;  // jumped to 7 after WiShield
const int oaFaceServo   = 5;  // [pwm]
const int oaMotor1speed = 6;  // [pwm]
// WiShield         = 4;  //
const int odMotor2dir   = 8;  //
const int oaMotor2speed = 9;  // [pwm]
// WiShield         = 10; // [pwm]
// WiShield         = 11; // [pwm]
// WiShield         = 12; //
// WiShield         = 13; // [led]
const int odLed         = 13; // [led]
const int iaIRranger1   = A0; //
const int iaIRranger2   = A1; //
const int iaTemperature = A2; //
//              = A3; //
//              = A4; // I2C data
//              = A5; // I2C clock

//---------------------------------------------------------------------------
// Constants
const int LEFT      = 1;
const int RIGHT     = 2;
int  speed = 255;
//---------------------------------------------------------------------------
// Globals
Servo faceServo;
int face_angle	=90;
String motor1 = "-";
String motor2 = "-";
long ts        = 0;

void face(int angle) {
	if(angle<0) angle = 0;
	if(angle>180) angle = 180;
	faceServo.write(angle);
	delay(abs(face_angle-angle)*2);
	face_angle = angle;
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

void  Motor1(int  pwm,  boolean  reverse) {
  analogWrite(oaMotor1speed,pwm);  //set  pwm  control,  0  for  stop,  and  255  for  maximum  speed
  if(pwm==0) {
    motor1="-";
  } else {
    ts = millis();
    if(reverse) {
      digitalWrite(odMotor1dir,HIGH);
      motor1="v";
    } else {
      digitalWrite(odMotor1dir,LOW);
      motor1="^";
    }
  }
}
 
void  Motor2(int  pwm,  boolean  reverse) {
  analogWrite(oaMotor2speed,pwm);
  if(pwm==0) {
    motor2="-";
	} else {
    ts = millis();
    if(reverse) {
      digitalWrite(odMotor2dir,HIGH);
      motor2="v";
    } else {
      digitalWrite(odMotor2dir,LOW);
      motor2="^";
    }
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
      median = 65*pow(median, -1.05);                 // worked out from graph 65 = theretical distance / (1/Volts)S - luckylarry.co.uk
    }
    else if(iaIRranger==iaIRranger2)
    {
      median = 60.495 * pow(median,-1.1904); //65*pow(volts, -1.10);  // worked out from graph 65 = theretical distance / (1/Volts)S - luckylarry.co.uk
    }
    distance = (int) median;
    distance = distance>150 ? 150 : distance;
    return distance;
}

void  setup()
{
    // Sensors
    faceServo.attach(oaFaceServo);
    face(face_angle);

    // Motors
    int  i;
    for(i=6;i<=9;i++)  //For  Roboduino  Motor  Shield
    //for(i=4;i<=7;i++)  //For  Arduino  Motor  Shield
    pinMode(i,  OUTPUT);  //set  pin  4,5,6,7  to  output  mode
 
    Serial.begin(115200);
    Serial.println("Brrm...");
}

String rangers() {
	String r = "{\"f\":" + String(face_angle);
	r=r+",\"d\":";
	r=r+String((getDistance(iaIRranger1) + getDistance(iaIRranger1))/2);
	r=r+"},{\"f\":";
	r=r+String(face_angle+90);
        r=r+",\"d\":";
        r=r+String((getDistance(iaIRranger2) + getDistance(iaIRranger2))/2);
        r=r+"}";
	return r;
}
String motors() {
	String r = ("{\"m1\":\""+motor1+"\",\"m2\":\""+motor2+"\"}");
	return r;
}
 
String  report()
{
	String r = "{";
	r=r+"\"motors\":"+motors();
	r=r+",\"rangers\":["+rangers()+"]";
	r=r+",\"t\":"+String(millis()-ts)+"}";
  ts = millis();
	return r;
}
 
void  loop()
{
  int  x,delay_en, a,l,r;
  char  val;
  String o,sep;
  while(1)
  {
    val  =  Serial.read();
    if((int)val!=-1) {
      switch(val) {
        case  '&':
        case  'w'://Move  ahead
          Motor1(speed,false);  //You  can  change  the  speed,  such  as  Motor(50,true)
          Motor2(speed,false);
          Serial.println(report()); 
          break;

        case  '(':
        case  'x'://move  back
          Motor1(speed,true);
          Motor2(speed,true);
          Serial.println(report());
          break;

        case  '%':
        case  'a'://turn  left
          Motor1(speed,false);
          Motor2(speed,true);
          Serial.println(report());
          break;

        case  '\'':
        case  'd'://turn  right
          Motor1(speed,true);
          Motor2(speed,false);
          Serial.println(report());
          break;

        case  's'://stop
          Motor1(0,false);
          Motor2(0,false);
          Serial.println(report());
          break;

        case 'q': //face left
          face(face_angle - 1);
          Serial.println(report());
          break;

        case 'e': // face right
          face(face_angle + 1);
          Serial.println(report());
          break;

        case 'z': //scan left
          l=0; r=0; o="{"; sep="";
          o += "\"motors\":"+motors();
          o += ",\"rangers\":[";
          for(a=face_angle; face_angle>a-10 && face_angle>0; ) {
            o += sep+rangers();
            sep=",";
            face(face_angle-1);
          }
          o += "]}";
          Serial.println(o);
          break;

        case 'c': // scan right
          l=0; r=0; o="{"; sep="";
          o += "\"motors\":"+motors();
          o += ",\"rangers\":[";
          for(a=face_angle; face_angle<a+10 && face_angle<180; ) {
            o += sep+rangers();
            sep=","; 
            face(face_angle+1);
          }
          o += "]}";
          Serial.println(o);
          break;

        case 'f': //full scan
          l=0; r=0; o="{"; sep="";
          o += "\"motors\":"+motors();
          o += ",\"rangers\":[";
          if((face_angle>=90 && face_angle<180) || face_angle==0) {
            while(face_angle<180) {
              o += sep+rangers();
              if(o.length()>512) {
                Serial.print(o);
                o = "";
              }
              sep=","; 
              face(face_angle+1);
            }
          } else {
            while(face_angle>0) {
              o += sep+rangers();
              if(o.length()>512) {
                Serial.print(o);
                o = "";
              }
              sep=",";
              face(face_angle-1);
            }
          }
          o += "]}";
          Serial.println(o);
          break;

        case '-': //slower
          if(speed >=5) speed += -5;
          break;

        case '=': //faster
          if(speed <=250) speed += 5;
          break;

      }

    }

    if(millis()-ts >253) {
      Serial.println(report());
    }
  }
}
