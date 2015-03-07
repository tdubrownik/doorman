#include <Arduino.h>
#include <SoftwareSerial.h>

#include "config.h"

/** keypad state - request pin */
#define SEND 0
/** keypad state - wait for response */
#define WAIT_RESP 1
/** Max length of message from keypad. */
#define KEYPAD_MAX_BYTES (5)
/** Max length of message from keypad. */
#define KEYPAD_RESP_SIZE (5)
/** Software serial port for communication with keypad. */
SoftwareSerial keypad(9, 8);
/** Global variable that holds last message from keypad. */
char keypad_bytes[KEYPAD_MAX_BYTES];
/** keypad state variable */
static byte keypad_state=SEND;

/**
 * Keypad initialization
 */

void keypad_init() {
  keypad.begin(19200);
}

/**
 * Tries to read pin code from keyboard.
 * @param p_pin Pointer to space for pin.
 * @return True if code was red.
 */
boolean keypad_pin_get(unsigned long  * p_pin) {  
  static byte byte_cntr=0;
  byte k;
#ifdef DEBUG_KEYPAD_SIM
  (*p_pin)=1234;
  return true;
#endif 
  switch (keypad_state){
    case SEND:
      //ask for pin
      keypad.print("#P");      
      byte_cntr=0;//clear input buffer
      keypad_state=WAIT_RESP;
      break;
      
    case WAIT_RESP:
      //wait for response
      if( keypad.available() ){
        //store byte
        keypad_bytes[byte_cntr]=keypad.read();
        byte_cntr++;  
        
        if(byte_cntr==KEYPAD_RESP_SIZE){
          //resp. received - store pin
          k=sscanf(keypad_bytes,"#%d",p_pin);
          keypad_state=SEND;
          if(k==1)
            //scanf successful - pin read
            return true;
        }
      }
      break;
     default:
       keypad_state=SEND;
       byte_cntr=0;
       break;
  } 
  return false;
}

/**
 * Turn off keyboard.
 */
void keypad_off(void) {
  keypad.print("XX");
  keypad_state=SEND;
}

/**
 * Pin correct command.
 */ 
void keypad_pin_ok() {
  keypad.print("#R");
}

/**
 * Pin wrong  command.
 */ 
void keypad_pin_wrong() {
  keypad.print("#W");
}
