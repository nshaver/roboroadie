#include <SoftwareSerial.h>
// init pin variables
byte btRxPin=2;       // BT receive pin
byte btTxPin=3;       // BT transmit pin
byte lEscPin=9;    // left motor ESC pin
byte rEscPin=10;   // left motor ESC pin
byte ledPin = 13;  // LED pin

// init bluetooth stuff
// BT programmed baud rate, was set via AT+BAUDx (19200=5). Default=9600 for HC-06.
// old original bt module is 19200
// new bt module is 9600
int baudRate=19200;
SoftwareSerial mySerial(btRxPin, btTxPin);

// init servo stuff
#include <SoftwareServo.h>
SoftwareServo s1;    // declare variables for up to eight servos
SoftwareServo s2;    // declare variables for up to eight servos

// init misc variables that will be used
boolean connected;
char serialData[32];
unsigned long nextTimeout;
unsigned long thisTime;
unsigned long lastSpeedTime;
char lastAction;
int timeoutTime=2000;
int speedMultiplier=100;  // 50=max out at 50% of full speed
char buf[100];

void setup() {
    // setup LED
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);

    // setup bluetooth serial
    mySerial.begin(baudRate);
    Serial.begin(9600);

    // attach servos
    s1.attach(lEscPin);
    s1.setMinimumPulse(1000);
    s1.setMaximumPulse(2000);
    s2.attach(rEscPin);
    s2.setMinimumPulse(1000);
    s2.setMaximumPulse(2000);

    // setup usb serial
    Serial.println("Setup complete...");
    nextTimeout=millis()+timeoutTime;
}

void loop() {
  // grab current time, don't want to use millis any more than we have to
  thisTime=millis();
  if(mySerial.available()>0){
    // all seems to be good - bump up nextTimeout
    nextTimeout=thisTime+timeoutTime;

    if (connected==false){
      // seeing connection for first time
      connected = true;
      digitalWrite(ledPin, HIGH);
      Serial.println("Started new connection...");
    }

    // read up to 31 bytes of serial data
    mySerial.readBytesUntil('\n', serialData, 31);

    // send heartbeats back to blueberry android app or it will give up
    mySerial.println("ok");

    if (serialData[0]=='s'){
      // appears to be a speed command
      drive(serialData);
      //Serial.println(serialData);
      lastAction='g';
      lastSpeedTime=thisTime;
    } else {
      Serial.println(serialData);
      // unrecognized command, if speed isn't sent quickly enough, assume a stop
      if (lastAction!='s' && thisTime>lastSpeedTime+1000){
        // been too long, just stop
        drive("stop");
        Serial.println("stop");
        lastAction='s';
      }
    }

    // reset the serial input variable
    memset(serialData, 0, sizeof(serialData));
  } else {
    // no serial received, may be a problem if this goes on too long
    if (thisTime>nextTimeout){
      // no signal received in nextTimeout
      if (connected){
        // was connected, connection just now went away
        connected=false;
        drive("stop");
        Serial.println("connection lost");
        digitalWrite(ledPin, LOW);
        nextTimeout=thisTime+timeoutTime;
      } else {
        // not connected
        Serial.println("still not connected");
        nextTimeout=thisTime+timeoutTime;
      }
    }
  }
}

/*
 * drive - activate motors with a max speed of speedMultipler percent, (50 = 50%)
 * expected input strings:
 *   s,123,123 = left and right motor values, each value an int between -100 and 100
 *   stop      = send whatever servo values stop the motors
 *
 */
void drive(String s){
  int inLSpeed;
  int inRSpeed;
  int outLSpeed;
  int outRSpeed;

  // parse input string into two comma separated values
  if (s=="stop"){
    inLSpeed=0;
    inRSpeed=0;
  } else if (s.substring(0, 2)=="s,"){
    s=s.substring(2);
    for (int i=0; i<s.length(); i++){
      if (s.substring(i, i+1)==","){
        inLSpeed=s.substring(0, i).toInt();
        inRSpeed=s.substring(i+1).toInt();
        break;
      }
    }
  } else {
    return;
  }

  if (inLSpeed>100) inLSpeed=100;
  if (inLSpeed<-100) inLSpeed=-100;
  if (inRSpeed>100) inRSpeed=100;
  if (inRSpeed<-100) inRSpeed=-100;

  // move servos, max out at speedMultiplier * speed
  if (inLSpeed>1){
    outLSpeed=map(speedMultiplier*inLSpeed/100, 0, 100, 90, 180);
  } else if (inLSpeed<-1) {
    outLSpeed=map(speedMultiplier*inLSpeed/100, 0, -100, 90, 0);
  } else {
    outLSpeed=90;
  }

  if (inRSpeed>1){
    outRSpeed=map(speedMultiplier*inRSpeed/100, 0, 100, 90, 180);
  } else if (inRSpeed<-1) {
    outRSpeed=map(speedMultiplier*inRSpeed/100, 0, -100, 90, 0);
  } else {
    outRSpeed=90;
  }

  s1.write(outLSpeed);
  s2.write(outRSpeed);

  sprintf(buf, "in: %03d,%03d  out: %03d,%03d", inLSpeed, inRSpeed, outLSpeed, outRSpeed);
  Serial.println(buf);

  s1.refresh();
  s2.refresh();
}
