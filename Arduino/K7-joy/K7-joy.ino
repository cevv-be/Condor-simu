#include <Adafruit_ADS1X15.h>
#include "Joystick.h"

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID, 
  JOYSTICK_TYPE_JOYSTICK, 12, 0,
  true, true, true, false, true, false,
  false, false, false, true, false);

//Joystick_ Joystick( JOYSTICK_DEFAULT_REPORT_ID, 
//  JOYSTICK_TYPE_JOYSTICK, 12, 0,
//  true, true, true, false, false, false,
//  true, true, false, false, false);

// buttons pin mapping.
// Buttons definitions (see spreadsheet)
#define L1pin 14
#define L2pin 21
#define L3pin 15
#define L4pin 20
#define R1pin 10
#define R2pin 8
#define R3pin 16
#define R4pin 9

// Vario control
#define  SWpin 18
#define  DTpin 19
#define  CLKpin 7

// Tost Hall sensor, digital
#define  Tost 4

#define DeadZone 15

// This maps pins to buttons.
byte button2pin[] = { 
    L1pin,
    L2pin,
    L3pin,
    L4pin,
    R1pin,
    R2pin,
    R3pin,
    R4pin,
    Tost,
    SWpin
    // encoder down
    // encoder up
}; 
int Vplus=sizeof(button2pin);
int Vminus=sizeof(button2pin)+1;

int lastX = 0; 
int lastY = 0;
int lastZ = 0;
int lastAF = 0;
int lastTrim = 0;

byte lastButtonState[sizeof(button2pin)];

// 
volatile boolean TurnDetected;  // need volatile for Interrupts
volatile boolean rotationdirection;  // CW or CCW rotation
            

const unsigned long gcCycleDelta = 1000;
const unsigned long gcAnalogDelta = 25;
const unsigned long gcButtonDelta = 500;
unsigned long gNextTime = 0;
unsigned int gCurrentStep = 0;

Adafruit_ADS1115 ads0;  /* Use this for the 16-bit version */
Adafruit_ADS1115 ads1;  /* Use this for the 16-bit version */

void setup() {

  Serial.println("Hello Olivier !");

  // put your setup code here, to run once:
  pinMode(L1pin, INPUT_PULLUP);
  pinMode(L2pin, INPUT_PULLUP);
  pinMode(L3pin, INPUT_PULLUP);
  pinMode(L4pin, INPUT_PULLUP);

  pinMode(R1pin, INPUT_PULLUP);
  pinMode(R2pin, INPUT_PULLUP);
  pinMode(R3pin, INPUT_PULLUP);
  pinMode(R4pin, INPUT_PULLUP);

  // Vario level encoder
  pinMode(CLKpin,INPUT);
  pinMode(DTpin,INPUT);  
  pinMode(SWpin,INPUT_PULLUP);
  attachInterrupt (4,isr,FALLING); // interrupt 4 always connected to pin 7 on Arduino pro micro
  
  ads0.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  ads1.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV

  ads0.setDataRate(RATE_ADS1115_860SPS); // or 860
  ads1.setDataRate(RATE_ADS1115_860SPS); // or 860


  if (!ads0.begin( )) {
    Serial.println("Failed to initialize ADS 0.");
    while (1);
  }
  
  if (!ads1.begin(0x49)) {
    Serial.println("Failed to initialize ADS 1.");
    while (1);
  }


  Serial.println("Start up Joystick !");
  Joystick.begin(false);
  Serial.println("Joystick Started");
  Joystick.setXAxisRange(0, 30000);
  Joystick.setYAxisRange(0, 30000);
  Joystick.setZAxisRange(0, 30000);
  Joystick.setRyAxisRange(0, 30000);
  Joystick.setBrakeRange(0, 30000);
  

}

// Interrupt routine runs if CLK goes from HIGH to LOW
void isr ()  {
  delay(4);  // delay for Debouncing
  if (digitalRead(CLKpin))
    rotationdirection= digitalRead(DTpin);
  else
    rotationdirection= !digitalRead(DTpin);
  TurnDetected = true;
}

void loop() {

  int change=0;
  int X; 
  int Y;
  int Z;
  int AF;
  int Trim;

  //unsigned long start = millis();

    //Serial.println("Main loop !, read buttons");
   // loop on buttons, read pin.
  for (byte index = 0; index < sizeof(button2pin); index = index + 1) {
    int currentButtonState = !digitalRead(button2pin[index] );
    if (currentButtonState != lastButtonState[index])
    {
      Joystick.setButton(index, currentButtonState);
      lastButtonState[index] = currentButtonState;
      change++;
    }
  }

  if(TurnDetected){
    Joystick.setButton(Vplus, rotationdirection);
    Joystick.setButton(Vminus, !rotationdirection);
    change++;
    TurnDetected = false;
  } else {
    Joystick.setButton(Vplus, false);
    Joystick.setButton(Vminus, false);
  }
  //Serial.println("Main loop, read adc !");
  X = ads0.readADC_SingleEnded(1); 
  Y = ads0.readADC_SingleEnded(0);
  Z = ads0.readADC_SingleEnded(2);
  AF = ads0.readADC_SingleEnded(3);
  Trim = ads1.readADC_SingleEnded(0);

  //Serial.println("Set axis !");
  if(abs(X -lastX) > DeadZone){
    Joystick.setXAxis(X );
    lastX=X;
    change++;
  }
  if(abs(Y -lastY) > DeadZone){
    Joystick.setYAxis(Y);
    lastY=Y;
    change++;
  }
  if(abs(Z-lastZ) > DeadZone){
    Joystick.setZAxis(Z);
    lastZ=Z;
    change++;
  }
  if(abs(Trim-lastTrim) >DeadZone){
    Joystick.setRyAxis(Trim);
    lastTrim=Trim;
    change++;
  }
  if(abs(AF-lastAF) >  DeadZone){
    Joystick.setBrake(AF);
    lastAF=AF;
    change++;
  }

  // joystick is only update in cas of change
  //Serial.println("Send State");
  if(change >0){
    Joystick.sendState();
  }

  //unsigned long duration=millis() - start;

    
  //Serial.print(duration); Serial.print(" ms: ");
  //Serial.print("Changes: "),Serial.print(change);
  //Serial.print(" X: ");Serial.print(X);
  //Serial.print(" Y: ");Serial.print(Y);
  //Serial.print(" Z:  "); Serial.print(Z);
  //Serial.print(" Tr: ");Serial.print(Trim);
  //Serial.print(" AF: ");Serial.print(AF);
  //Serial.println();
  //delay(5);
}
