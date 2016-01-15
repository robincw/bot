#include <ps2.h>
#include <WiShield.h>
#include <Servo.h> 

/*
 * Socket App
 *
 * A simple socket application example using the WiShield 1.0
 */

#define WIRELESS_MODE_INFRA	1
//#define WIRELESS_MODE_ADHOC	2

// Wireless configuration parameters ----------------------------------------
unsigned char local_ip[] = {192,168,2,2};	// IP address of WiShield
unsigned char gateway_ip[] = {192,168,2,1};	// router or gateway IP address
unsigned char subnet_mask[] = {255,255,255,0};	// subnet mask for the local network
const prog_char ssid[] PROGMEM = {"Black"};		// max 32 bytes
unsigned char security_type = 3;	// 0 - open; 1 - WEP; 2 - WPA; 3 - WPA2
const prog_char security_passphrase[] PROGMEM = {"26##########"};	// WPA/WPA2 passphrase, max 64 characters
prog_uchar wep_keys[] PROGMEM = {	0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,	// Key 0
									0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00,	// Key 1
									0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00,	// Key 2
									0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00	// Key 3
								};
unsigned char wireless_mode = WIRELESS_MODE_INFRA;
unsigned char ssid_len;
unsigned char security_passphrase_len;

// TMP36 and WiShield: http://asynclabs.com/forums/viewtopic.php?t=409
// Frequency counter:  http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1231326297/60
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Pin numbers
const int MOUSE_DATA		= 3;//0;  // [Serial RX]
const int MOUSE_CLOCK		= 2;//1;  // [Serial TX]
//  WiShield              	= 2;  // [INT0 Interrupt]
//							= 3;  // [INT1 Interrupt, pwm]
const int odMotor1direction	= 4;  // jumped to 7
const int oaIRServo			= 5;  // [pwm]
const int oaMotor1speed		= 6;  // [pwm]
// WiShield					= 7;  // 
const int odMotor2direction	= 8;  // 
const int oaMotor2speed		= 9;  // [pwm]
// WiShield					= 10; // [pwm]
// WiShield					= 11; // [pwm]
// WiShield					= 12; // 
// WiShield         = 13; // [led]
const int	odLed		  = 13; // [led]
const int iaIRranger1		= A0; // 
const int iaIRranger2		= A1; // 
//							= A2; // 
//							= A3; // 
// 							= A4; // 
//							= A5; // 

//---------------------------------------------------------------------------
// Other variables
int wheelData[3];
int rangeData[3];
int scans[267];
int scanCount           = 0;
int furthestAngle       = 0;
int furthestDistance    = 0;
int closestAngle        = 0;
int closestDistance     = 150;
int motor1_target	= 0;
int motor2_target	= 0;
int motor1_tickCount	= 0;
int motor2_tickCount	= 0;
int face_angle			= -90;
int face_delta			= 0;
int speed				= 255;
PS2 mouse(MOUSE_CLOCK, MOUSE_DATA);
Servo irservo;
const int WHEELSPAN		= 20;// todo: measure wheelspan in cm
const int WHEELRADIUS	= 5;// todo: measure wheelradius in cm
const int LEFT			= 1;
const int RIGHT			= 2;
const int FWD			= 3;
const int BACK			= 4;
const int STOP			= 5;
const int TURN			= 6;
const boolean WIFI = false;
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void isort(int *a, int n) //  *a is an array pointer function
{
  for (int i = 1; i < n; ++i)
  {
    int j = a[i];
    int k;
    for (k = i - 1; (k >= 0) && (j < a[k]); k--)
    {
	a[k + 1] = a[k];
    }
    a[k + 1] = j;
  }
}
//---------------------------------------------------------------------------
int getDistance(int iaIRranger, int interval = 2)
{
    float volts		= 0;
    float total		= 0;
    int distance	= 0;
    int index		= 0;
    int numReadings     = 8;
    int readings[8];
    for(index = 0; index < numReadings; index++) // take x number of readings from the sensor and average them
    {
		volts		= analogRead(iaIRranger)*0.0048828125;  // value from sensor * (5/1024) - if running 3.3.volts then change 5 to 3.3
                if(iaIRranger==iaIRranger1)
                {
                		readings[index]	= 65*pow(volts, -1.05);                 // worked out from graph 65 = theretical distance / (1/Volts)S - luckylarry.co.uk
                }
                else if(iaIRranger==iaIRranger2)
                {
                		readings[index]	= 60.495 * pow(volts,-1.1904); //65*pow(volts, -1.10);                 // worked out from graph 65 = theretical distance / (1/Volts)S - luckylarry.co.uk
                }
		//total		= total + distance;                     // update total
		delay(interval);
    }
    isort(&readings[0], numReadings);
    double median = numReadings % 2 ? readings[numReadings / 2] : (readings[numReadings / 2 - 1] + readings[numReadings / 2]) / 2;
    // create average reading as int
    distance = (int) median;
    distance = distance>150 ? 150 : distance;
  return distance;
}

//---------------------------------------------------------------------------
void setMotor(int motor, int speed, boolean forward)
{
	int speedPin = oaMotor1speed;
  	int dirPin   = odMotor1direction;
	if(motor==RIGHT)
	{
		speedPin	= oaMotor2speed;
		dirPin		= odMotor2direction;
	}
	analogWrite(speedPin, speed); //set pwm control, 0 for stop, and 255 for maximum speed
	if(forward)
	{
		digitalWrite(dirPin,HIGH);    
	}
	else
	{
		digitalWrite(dirPin,LOW);    
	}
}

//---------------------------------------------------------------------------
void moveNext()
{
	// check wheel positions
//    mouse.write(0xeb);  // give me data!
//    mouse.read();      // ignore ack
  wheelData[0]     = 0;//mouse.read();
  wheelData[LEFT]  = 0;//mouse.read();
  wheelData[RIGHT] = 0;//mouse.read();
  motor1_tickCount -= wheelData[RIGHT];
  motor2_tickCount -= wheelData[LEFT];

	// set left motor
	if(motor1_tickCount == motor1_target)
	{
		setMotor(LEFT, 0, motor1_tickCount > motor1_target);
		motor1_tickCount	= 0;
		motor1_target		= 0;
	}
	else
	{
    scanCount           = 0;
    furthestAngle       = 0;
    furthestDistance    = 0;
    closestAngle        = 0;
    closestDistance     = 150;
		setMotor(LEFT, speed, motor1_tickCount > motor1_target);
	}
	// set right motor
	if(motor2_tickCount == motor2_target)
	{
		setMotor(RIGHT, 0, motor2_tickCount > motor2_target);
		motor2_tickCount	= 0;
		motor2_target		= 0;
	}
	else
	{
    scanCount           = 0;
    furthestAngle       = 0;
    furthestDistance    = 0;
    closestAngle        = 0;
    closestDistance     = 150;
		setMotor(RIGHT, speed, motor2_tickCount < motor2_target);
	}
        //return wheelData;
}

//---------------------------------------------------------------------------
void face(int angle)
{
    irservo.write(angle+90);
    delay(abs(face_angle-angle)*1);
    face_angle = angle;
}

//---------------------------------------------------------------------------
void lookNext()
{
	if(face_delta>0 && face_angle>=90)
	{
		//finished moving face left to right
		//face_delta --;
		face_delta = -face_delta;
	}
	else if(face_delta<0 && face_angle<=-90)
	{
		//finished moving face right to left
		face_delta = -face_delta;
	}
	if(face_delta != 0 )
    {
      face(face_angle + face_delta);
    }
    //return rangeData;
	rangeData[0]	= face_angle;
	rangeData[LEFT]	= getDistance(iaIRranger1);
	rangeData[RIGHT]= getDistance(iaIRranger2);
    /*
    scans[face_angle-43] = rangeData[LEFT];
    scans[face_angle+43] = rangeData[RIGHT];
    */
    //scanCount ++;
    //if(face_angle < -47 || face_angle>47) scanCount ++;
    
    if(rangeData[LEFT]>furthestDistance)
    {
      furthestDistance = rangeData[LEFT];
      furthestAngle    = rangeData[0]-43;
    }
    if(rangeData[RIGHT]>furthestDistance)
    {
      furthestDistance = rangeData[RIGHT];
      furthestAngle    = rangeData[0]+43;
    }
    if(rangeData[LEFT]<closestDistance)
    {
      closestDistance = rangeData[LEFT];
      closestAngle    = rangeData[0]-43;
    }
    if(rangeData[RIGHT]<closestDistance)
    {
      closestDistance = rangeData[RIGHT];
      closestAngle    = rangeData[0]+43;
    }
}

//---------------------------------------------------------------------------
int findTarget()
{
  int prev_delta = face_delta;
  boolean scannedLeft = (prev_delta<0 && face_delta>0);
  boolean scannedRight = (prev_delta>0 && face_delta<0);
  lookNext();
  moveNext();
  // set movement
  if(!scannedLeft) scannedLeft = (prev_delta<0 && face_delta>0);
  if(!scannedRight) scannedRight = (prev_delta>0 && face_delta<0);
  if(scannedLeft || scannedRight)
  {
    face(closestAngle);
    delay(1000);
    scannedLeft = false;
    scannedRight = false;
    closestDistance = 150;
  }
  return closestAngle;
}
//---------------------------------------------------------------------------
void move(int movement, int distance = 10)
{
	motor1_tickCount = 0;
  motor2_tickCount = 0;
  switch(movement)
	{
		case TURN:
			if(distance < 0)
			{
				move(LEFT, distance);
			}
			else if(distance > 0)
			{
				move(RIGHT, distance);
			}
			break;
		case LEFT://turn left
			distance = (WHEELSPAN/2)*(distance*0.0174532925); // (pi/180)=0.0174532925
			motor1_target = -distance;
			motor2_target = distance;
			break;       
		case RIGHT://turn right
			distance = (WHEELSPAN/2)*(distance*0.0174532925); // (pi/180)=0.0174532925
			motor1_target = distance;
			motor2_target = -distance;
			break;   
		case FWD://Move ahead
			motor1_target = distance;
			motor2_target = distance;
			break;
		case BACK://move back
			motor1_target = -distance;
			motor2_target = -distance;
			break;
		case STOP://stop
			motor1_target = 0;
			motor2_target = 0;
			break;
	}
}
void blink(int times = 1) {
  int b;
  for(b=0; b<times; b++)
  {
    digitalWrite(odLed, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(80);               // wait for a second
    digitalWrite(odLed, LOW);    // turn the LED off by making the voltage LOW
    delay(120);               // wait for a second
  }
  delay(200);
}
//---------------------------------------------------------------------------
void error(char buffer[128]) {
    sprintf(buffer, "%02X %d %d" , wheelData[0], wheelData[LEFT], wheelData[RIGHT]);
    Serial.println(buffer);
}
//---------------------------------------------------------------------------
void mouse_init()
{
  if(mouse.isResponding()) {
    mouse.write(0xff);  // reset
    mouse.read();  // ack byte
    mouse.read();  // blank */
    mouse.read();  // blank */
    mouse.write(0xf0);  // remote mode
    mouse.read();  // ack
    delayMicroseconds(100);
  } else {
    error('Mouse is not responding');
  }
}
void setup()
{
  mouse_init();
//mouse.set_resolution(10); // todo: tune this so distances are in cm
	irservo.attach(oaIRServo);
	pinMode(odMotor1direction	, OUTPUT);
	pinMode(oaMotor1speed		, OUTPUT);
	pinMode(odMotor2direction	, OUTPUT);
	pinMode(oaMotor2speed		, OUTPUT);
        // this connects to wifi todo: allow serial/wifi selection switch on pin 3?
	if(WIFI)
  {
    //WiFi.init();
  }
  else
  {
    Serial.begin(9600);
  }
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void loop()
{
  //findTarget();
  if(motor1_target == 0 && motor2_target == 0)
  {
    delay(3000);
    move(FWD, 10);
  }
	if(WIFI)
  {
    //WiFi.run();
  }
  else
  {
    char buffer[128];
    //sprintf(buffer, "%d %d, %d %d, %d %d" , closestAngle, closestDistance, motor1_tickCount, motor1_target, face_delta, prev_delta);
    //Serial.println(buffer);
    //sprintf(buffer, "%d 2 %d %d" , face_delta, rangeData[0]+43, rangeData[2]);
    //Serial.println(buffer);
    sprintf(buffer, "%02X %d %d" , wheelData[0], wheelData[LEFT], wheelData[RIGHT]);
    Serial.println(buffer);
  }   
}
