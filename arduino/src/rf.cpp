#include <Arduino.h>

#include "config.h"

/** Global variable that holds last message from rfid reader. */
byte rf_bytes[RF_MAX_BYTES];

/**
 * Sends hald command to rfid reader.
 */
void rf_halt() {
  //Halt tag
  /*rfid.print(255, BYTE);
  rfid.print(0, BYTE);
  rfid.print(1, BYTE);
  rfid.print(147, BYTE);
  rfid.print(148, BYTE);*/
}

/**
 * Sends seek command to rfid reader.
 */
void rf_seek() {
  //search for RFID tag
  /*rfid.print(255, BYTE);
  rfid.print(0, BYTE);
  rfid.print(1, BYTE);
  rfid.print(130, BYTE);
  rfid.print(131, BYTE); */
}

/**
 * Function receives bytes from rfid reader and parses them into message
 * @return Returns true if message was received, false otherwise
 */
boolean rf_parse() {
  /*static int rf_idx = 0;
  while(rfid.available()){
    //Read the incoming byte.
    byte in = rfid.read();
    if (in==255 && rf_idx==0) {
      //Start byte
      rf_bytes[rf_idx]=in; 
      rf_idx+=1;                  
      continue;
    } 
    if (rf_idx<RF_MAX_BYTES){
      if (rf_idx>2){
        //Message length was received
        if (rf_idx>(rf_bytes[2]+3)){
          //This should not ever happen.
          rf_idx=0;
          continue;
        } else if (rf_idx==(rf_bytes[2]+3)) {
          //End of message
          rf_bytes[rf_idx]=in;          
          int sum=0;
          //Calculate checksum from message
          for (int ii=1;ii<(rf_idx);ii++){            
            sum+=rf_bytes[ii];
          }
          if ((sum & 0x0ff)!=rf_bytes[rf_idx]) {
#ifdef DEBUG
            Serial.println("Wrong checksum");
#endif //DEBUG
            rf_idx=0;
            return false;
          }
          //Message received!
          rf_idx=0;
          return true;
        } 
      }
      rf_bytes[rf_idx]=in; 
      rf_idx+=1;
      continue;
    } else{
#ifdef DEBUG
      Serial.println("Data too long");
#endif //DEBUG
      rf_idx=0;
      continue;
    }     
  }*/
  return false;
}

/**
 * Handles communication with rfid reader.
 * @param p_rfid Pointer to space where mifare id will be written.
 * @return Returns true when authorized user rfid was rd.
 */
boolean rf_comm(unsigned long * p_rfid) {
  static unsigned long last_millis = 0;
  static unsigned long new_millis = 0;
  (*p_rfid) = 0;
  new_millis = millis();
  if (new_millis<last_millis){
    last_millis=new_millis; 
  } else if ((millis()-last_millis ) > RF_PERIOD_MS ){
    rf_seek();
    last_millis = new_millis;
  }
  if (rf_parse()){
    //Message received
    if(rf_bytes[2] > 2 && rf_bytes[3]==0x82){
      //Message has id
      unsigned long data=(long(rf_bytes[8])<<24)|(long(rf_bytes[7])<<16)|(long(rf_bytes[6])<<8)|(long(rf_bytes[5]));
      (*p_rfid) = data;
      return true;
    }
  } 
  return false;
}

