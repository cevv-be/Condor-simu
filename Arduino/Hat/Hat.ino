
//--------------------------------------------------------------------

#include <Joystick.h>

// 1 hat switch, 1 button
Joystick_ Joystick(0x04, 
  JOYSTICK_TYPE_JOYSTICK, 1,
  1,
  false, false, false, false, false, false,
  false, false, false, false, false);

#define Rpin 21
#define Uppin 20
#define Downpin 15
#define Lpin 14
#define SWpin 18

byte button2pin[] = { 
    Uppin,
    Rpin,
    Downpin,
    Lpin
};

int angle2pin[] = {
  0,
  90,
  180,
  270
};

void setup() {
  
  // Initialize Button Pins
  pinMode(Rpin, INPUT_PULLUP);
  pinMode(Uppin, INPUT_PULLUP);
  pinMode(Downpin, INPUT_PULLUP);
  pinMode(Lpin, INPUT_PULLUP);
  pinMode(SWpin, INPUT_PULLUP);

  // Initialize Joystick Library
  Joystick.begin(false);
}

// Last state of the pins
int lastButtonState[5] = {0,0,0,0,0};

void loop() {

  int change = 0;
  
  // Read pin values, record if there are changes
  for (byte index = 0; index < sizeof(button2pin); index = index + 1) {
    int currentButtonState = !digitalRead(button2pin[index] );
    if (currentButtonState != lastButtonState[index])
    {
      lastButtonState[index] = currentButtonState;
      change++;
    }
  }

  // Try to support 45 degree with two button pressed, and  cases where buttons are not pressed simultaneously
  if(change > 0){
    // sum all pressed buttons
    int pressed = 0; // number of button presses
    int result = 0;
    for(byte index = 0; index < sizeof(button2pin); index = index + 1) {
      if(lastButtonState[index] > 0){
        result += angle2pin[index];
        pressed++;
      }
    }
    // normalize angle (if more than 2 buttons, a bit random...
    if(pressed>1){
      //special case: up pressed (0)
      if(lastButtonState[0] == 0){
         result=result/pressed;
      } else {
        if(result == 270){
          result = 315;
        } else {
          result = 45;
        }
      }
    } else if (pressed == 0){
      result=-1;
    }

    Joystick.setHatSwitch(0,result);
  } 

  // read SW, the hat has a center switch
  int currentButtonState = !digitalRead(SWpin );
  if (currentButtonState != lastButtonState[sizeof(button2pin)])
    {
      Joystick.setButton(0, currentButtonState);
      lastButtonState[sizeof(button2pin)] = currentButtonState;
      change++;
    }
    
  //Serial.println("Send State");
  if(change >0){
    Joystick.sendState();
  }
  delay(50);
}
