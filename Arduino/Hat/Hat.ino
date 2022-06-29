
//--------------------------------------------------------------------

#include <Joystick.h>
#include <Wire.h>
#include "nunchuck.h"
#include "Mouse.h"
#include "Keyboard.h"

// Sensitivity for Nunchuck as mouse
#define angleSensitivity 130
#define angleZero 10

#define joystickSensitivity 3
#define joystickZero 3

#define DELAY 50

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

// keyPress map joystick positions to key to send
// could be merged to three dimensional array but no code improvment...
uint8_t key1Press [3][3] = {
  { KEY_F2,  KEY_F1, KEY_F4 },
  { KEY_F3, 0, KEY_F5 },
  { KEY_F6,  KEY_LEFT_CTRL , KEY_F7}
} ;
uint8_t key2Press [3][3] = {
  { 0,  0, 0 },
  { 0, 0, 0 },
  { 0, KEY_F1 , 0}
} ;

void setup() {

  // Initialize Button Pins
  pinMode(Rpin, INPUT_PULLUP);
  pinMode(Uppin, INPUT_PULLUP);
  pinMode(Downpin, INPUT_PULLUP);
  pinMode(Lpin, INPUT_PULLUP);
  pinMode(SWpin, INPUT_PULLUP);

  // Initialize Joystick Library
  Joystick.begin(false);

  // initialize Wire
  Wire.begin();

  // initialize nunchuck communication
  nunchuk_init();

  // Mouse
  Mouse.begin();

  // Keyboard
  Keyboard.begin();
}

// Last state of the pins
int lastButtonState[5] = {0,0,0,0,0};

int16_t lastX,lastY;

int16_t range= 3;

void loop() {

  int change = 0;
  int16_t Zero;
  int16_t X,Y;
  int16_t KX,KY;
  int Xindex;Yindex:
  boolean toRelease;

  // Nunchuck processing
  if (nunchuk_read()) {

    // Z button swtch between joystick or mouse
    if(nunchuk_buttonZ()){

       // possibility to move the pointer using rotation when Z is pressed.
       X=nunchuk_roll()*angleSensitivity;
       Y=nunchuk_pitch()*angleSensitivity;
       Zero= angleZero;

        // Funny mapping of the joystick to function keys
        // map the ranges (-127,127) of the joystick to 0..2 to index the arrays (key1 and key2)
        KX=map(nunchuk_joystickX(),-127,127,0,2);
        KY=map(nunchuk_joystickY(),-127,127,2,0);

        if( (KX != lastX) || (KY != lastY)){
          lastX=KX;
          lastY=KY;
          // Is there a keypress to send
          if( key1Press[KY][KX] > 0 ){
              Keyboard.press(key1Press[KY][KX]);
              //Serial.print(key1Press[KY][KX]);
              if(key2Press[KY][KX] > 0 ){
                   Keyboard.press(key2Press[KY][KX]);
                   //Serial.print(",");Serial.println(key2Press[KY][KX]);
              }
              toRelease=true;
              // Release is delayed to the end after the delay to leave the key active for some time.
              //Serial.println("ReleaseAll");
          }
        }
    } else {
       // use nunchuck joystick as pointing device
       X=nunchuk_joystickX()/joystickSensitivity;
       Y=-nunchuk_joystickY()/joystickSensitivity;
       Zero= joystickZero;
    }

    if(abs(X) > Zero||abs(Y)>Zero ){
       Mouse.move(X,Y,0);
    }

    if(nunchuk_buttonC()){
        if(!Mouse.isPressed(MOUSE_LEFT)){
         Mouse.press(MOUSE_LEFT);
        }
    } else {
        if(Mouse.isPressed(MOUSE_LEFT)){
         Mouse.release(MOUSE_LEFT);
        }
    }
  }

  // HAT buttons
  // Read pin values, record if there are changes
  for (byte index = 0; index < sizeof(button2pin); index = index + 1) {
    int currentButtonState = !digitalRead(button2pin[index] );
    if (currentButtonState != lastButtonState[index])
    {
      lastButtonState[index] = currentButtonState;
      change++;
    }
  }

  // 45 degree with two buttons pressed, and cases where buttons are not pressed simultaneously
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
    // normalize angle (if more than 2 buttons, a bit random)...
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

  // read SW, the HAT has a center switch
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
  delay(DELAY);

  // release keypress after a delay
  if(toRelease){
     Keyboard.releaseAll();
     toRelease=false;
  }
}
