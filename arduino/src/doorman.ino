/**
 * 
 * Simple RFID lock controller, based on:
 *  RFID Eval 13.56MHz Shield example sketch v10
 *  Aaron Weiss, aaron at sparkfun dot com
 *  OSHW license: http://freedomdefined.org/OSHW
 * 
 * @note Works with 13.56MHz MiFare 1k tags.
 * @note RFID Reset attached to D13 (aka status LED)  
 * @note Be sure include the NewSoftSerial lib, http://arduiniana.org/libraries/newsoftserial/  
 * @note Sha256 library is required, http://code.google.com/p/cryptosuite/
 */

#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <sha256.h>

#include "config.h"
#include "keypad.h"
#include "pc.h"
#include "rf.h"
#include "emem.h"

// Global var
/** Used to pass current command hash **/
unsigned char g_Hash[EMEM_HASH_SIZE];
/** Door control pin. */
#define DOOR_CTRN (4)

// From pc.cpp
/** Flag set when next scan should send mid. */
extern boolean pc_send_flag;

/** Setup function. */
void setup()  {
  Serial.begin(19200);
  keypad_init();
  delay(10);
  rf_halt();
  pinMode(DOOR_CTRN, OUTPUT);
#ifdef DEBUG  
  Serial.println("Ready");
#endif //DEBUG  
}
/** Idle. */
#define STATE_IDLE 0
/** Waiting for pin code. */
#define STATE_PIN 1
/** Waiting for mifare. */
#define STATE_MIFARE 2
/** State variable. */
int state=STATE_IDLE;
/** Start time for timeout calculation. */
unsigned long state_time=0;
/** Timeout value. */
#define STATE_TIMEOUT 20000
/** Defines how long door should be open in ms. */
#define STATE_DOORTIME 4000

/** Main loop. */
void loop(){     
  unsigned long pin=0;
  unsigned long rfid=0;  
  unsigned long current_time;
  while (1) {
    switch (state){
      case STATE_PIN:
        if (keypad_pin_get(&pin)){
          //Pin code was read 
          //Check if authorised      
          char str[8+1+8+1];             
          for(int ii=0;ii<8+1+8+1;ii++) str[ii]=0;       
          snprintf(str,8+1+8+1,"%08lx:%08lx",pin,rfid);
#ifdef DEBUG          
          Serial.print("Got pin: ");
          Serial.print(pin);        
          Serial.print(" and rfid: ");
          Serial.println(rfid);
          Serial.print("Rfid and pin: ");
          Serial.println(str);
#endif //DEBUG         
          Sha256.init();         
          Sha256.print(str);          
          uint8_t * hash=Sha256.result();
#ifdef DEBUG          
          pc_print_hash(hash);
          Serial.println();          
#endif //DEBUG
          //react
          unsigned int fid = emem_find_data(hash);
          if (pc_send_flag){
            pc_send_flag = false;
            pc_print('s',fid,hash);
          }
          if(fid!=EMEM_INV_ADR) {
            //"success" beep
            keypad_pin_ok();
            //open door
#ifdef DEBUG            
            Serial.println("Door opened");           
#endif //DEBUG
            digitalWrite(DOOR_CTRN, HIGH);  
            delay(STATE_DOORTIME);                  
            digitalWrite(DOOR_CTRN, LOW); 
#ifdef DEBUG
            Serial.println("Door closed");           
#endif //DEBUG            
          } else {
            //"error" beep
            keypad_pin_wrong();
#ifdef DEBUG
            Serial.println("Wrong rfid or pin");            
#endif //DEBUG
          }
          
          pin=0;
          rfid=0;
          state=STATE_IDLE;
          continue;
        }
        current_time=millis();
        if (state_time<current_time){
           if (current_time-state_time>STATE_TIMEOUT){
             //turn keypad off
             keypad_off();
#ifdef DEBUG             
             Serial.println("Keyboard timeout");             
#endif //DEBUG             
             pin=0;
             rfid=0;
             state=STATE_IDLE;
             continue;
           }
        } else {
           if (state_time-current_time>STATE_TIMEOUT){
             //turn keypad off
             keypad_off();
#ifdef DEBUG             
             Serial.println("Keyboard timeout");             
#endif //DEBUG            
             pin=0;
             rfid=0;
             state=STATE_IDLE;
             continue;
           }        
        }
        break;
      case STATE_MIFARE:
        if (rf_comm(&rfid)){
#ifdef DEBUG             
          Serial.println("Got rfid: ");
          Serial.println(rfid,HEX);
#endif //DEBUG                      
          //Rfid was read
          state=STATE_PIN;           
          state_time=millis();
        }
        break; 
      case STATE_IDLE:    
      default:      
        pin=0;
        rfid=0;
        state=STATE_MIFARE;
        break; 
    }  
    //Communication with PC (optional).
    pc_comm();    
  }
}

