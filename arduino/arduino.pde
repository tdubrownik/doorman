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
 */

#include <NewSoftSerial.h>
#include <EEPROM.h>
#include <sha256.h>

//Defines
/** 
 * Record size. 
 * @todo This should be EMEM_HASH_SIZE+1, but since we are low on epprom we save and check only first 6 bytes of hash. 
 */
#define EMEM_RECORD_SIZE (7)
/** First record ID. */
#define ENEM_MIN_ADR (1)
/** Last record ID. */
#define EMEM_MAX_ADR (100)
/** Invalid record ID. */
#define EMEM_INV_ADR (0)
/** Size of stored hash. */
#define EMEM_HASH_SIZE (8)
/** Flag that indicates (if defined) verbose mode (for diagnostic) */
#define DEBUG 1
/** Max length of message from rfid reader. */
#define RF_MAX_BYTES (32)
/** Max length of message from or to PC. */
#define PC_MAX_BYTES (2+1+4+1+16+1+8+1)
/** Period for quering rfid reader. */
#define RF_PERIOD_MS (500)
//Global var
/** Software serial port for communication with rfid reader. */
NewSoftSerial rfid(7, 8);
/** Global variable that holds last message from rfid reader. */
byte rf_bytes[RF_MAX_BYTES];
/** Global vatiable that holds last message from PC. Lenght is plus one for zero termination. */
char pc_bytes[PC_MAX_BYTES+1];
/** Token. */
unsigned long pc_token = 0x386855c3;
/** Flag set when next scan should send mid. */
boolean pc_send_flag = false;

/** keypad state - request pin */
#define SEND 0
/** keypad state - wait for response */
#define WAIT_RESP 1
/** Max length of message from keypad. */
#define KEYPAD_MAX_BYTES (5)
/** Max length of message from keypad. */
#define KEYPAD_RESP_SIZE (5)
/** Software serial port for communication with keypad. */
NewSoftSerial keypad(2, 3);
/** Global variable that holds last message from keypad. */
char keypad_bytes[KEYPAD_MAX_BYTES];
/** keypad state variable */
static byte keypad_state=SEND;
/** Pin correct */ 
#define keypad_pin_ok() keypad.print("#R")
/** Pin wrong  */ 
#define keypad_pin_wrong() keypad.print("#W")

//Old protocol
//$g,00000000,0000,0000,386855c3
//$p,00000000,0000,0000,386855c3
//$a,00000000,0000,0000,386855c3
//Protocol with hashes
//$g,0000,0000000000000000,386855c3
//$p,0000,0000000000000000,386855c3
//$a,0000,0000000000000000,386855c3

/** Setup function. */
void setup()  {
  Serial.begin(19200);
  rfid.begin(19200);
  keypad.begin(19200);
  delay(10);
  rf_halt();
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
unsigned long state_time=0;
#define STATE_TIMEOUT 20000
/** Main loop. */
void loop(){     
  unsigned long pin=0;
  unsigned long rfid=0;
  unsigned long current_time;
  while (1) {
    switch (state){
      case STATE_PIN:
        if (keypad_pin_get(&pin)){
          Serial.print("Got pin.");
          Serial.println(pin);        
          //Pin code was read 
          //Check if authorised      
          
          //react
          if(0)
          {
            //"success" beep
            keypad_pin_ok();
            //open door
            
          }
          else
          {
            //"error" beep
            keypad_pin_wrong();
          }
          state=STATE_IDLE;
          continue;
        }
        current_time=millis();
        if (state_time<current_time){
           if (current_time-state_time>STATE_TIMEOUT){
             //turn keypad off
             keypad_off();
             Serial.println("Timeout");             
             state=STATE_IDLE;
             continue;
           }
        } else {
           if (state_time-current_time>STATE_TIMEOUT){
             //turn keypad off
             keypad_off();
             Serial.println("Timeout");
             state=STATE_IDLE;
             continue;
           }        
        }
        break;
      case STATE_MIFARE:
        if (rf_comm(&rfid)){
          Serial.println("Got rfid.");
          Serial.println(rfid);
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

/**
 * Function reads record with given id from EEPROM. 
 * @param adr Address of record to read, or NULL.
 * @param id Red id from memory.
 * @param data Pointer to space reserved for hash.
 * @return Function returns true when data was red, false otherwise.
 */
boolean emem_get_record(unsigned int adr,unsigned int * id,unsigned char * data){
  if (adr>EMEM_MAX_ADR || adr<ENEM_MIN_ADR) {
    //Address out of range.
    return false;  
  }
  (*id)=EEPROM.read(adr*EMEM_RECORD_SIZE);
  for (int ii=1;ii<EMEM_RECORD_SIZE;ii++){
    *(data+ii-1) = EEPROM.read(adr*EMEM_RECORD_SIZE+ii);
  }
  return true;  
}

/** 
 * Function deletes record with given address.
 * @param adr Record address.
 * @return True if record was deleted.
 */
boolean emem_del_record(unsigned int adr){
  if (adr>EMEM_MAX_ADR || adr<ENEM_MIN_ADR) {
    //Address out of range.
    return false;  
  }
  for (int ii=0;ii<EMEM_RECORD_SIZE;ii++){
    EEPROM.write(adr*EMEM_RECORD_SIZE+ii,0);
  }
  return true;  
}

/**
 * Finds record with given ID. 
 * @param id Record id.
 * @return Address of found record or EMEM_INV_ADR.
 */
unsigned int emem_find_id(unsigned int id){
  if (id>EMEM_MAX_ADR || id<ENEM_MIN_ADR) {
    //Address out of range.
    return EMEM_INV_ADR;  
  }
  if (EEPROM.read(id*EMEM_RECORD_SIZE)==id) return id;
  return EMEM_INV_ADR;
}

/**
 * Finds record containing given data.
 * @param data Data to be found.
 * @return Address of found record or EMEM_INV_ADR.
 */
unsigned int emem_find_data(unsigned char * data) {  
  for (int adr=ENEM_MIN_ADR;adr<=EMEM_MAX_ADR;adr++){    
    for (int ii=1;ii<EMEM_RECORD_SIZE;ii++){
      if (EEPROM.read(adr*EMEM_RECORD_SIZE+ii)==*(data+ii)){
        return adr;
      } else {
        break;
      }
    }    
  } 
  return EMEM_INV_ADR; 
}

/**
 * Finds first free record.
 * @return Record address or EMEM_INV_ADR if no space left.
 */
unsigned int emem_find_free(){
  for (int adr=ENEM_MIN_ADR;adr<=EMEM_MAX_ADR;adr++){    
    if (EEPROM.read(adr*EMEM_RECORD_SIZE)==0){
      return adr;
    }        
  } 
  return EMEM_INV_ADR;    
}

/**
 * Function sets data into EEPROM record.
 * @param data Data to be set.
 * @param id Record id.
 * @return True if data was written, false otherwise.
 */
boolean emem_set_record(unsigned int id,unsigned char * data){
  if (id>EMEM_MAX_ADR || id<ENEM_MIN_ADR) return false;  
  EEPROM.write(id*EMEM_RECORD_SIZE,(unsigned char)(id&0x0ff));
  for (int ii=1;ii<EMEM_RECORD_SIZE;ii++) {
    EEPROM.write(id*EMEM_RECORD_SIZE+ii,*(data+ii));  
  }
  //Ok, now try to read and compare.
  for (int ii=1;ii<EMEM_RECORD_SIZE;ii++) {
    if (EEPROM.read(id*EMEM_RECORD_SIZE+ii) != *(data+ii)){
      return false;
    }
  }  
  return true;
}


/**
 * Prints all records in EEPROM memory.
 */
void emem_print(){  
  unsigned int id;  
  unsigned char data[EMEM_HASH_SIZE];
  for (int ii=0;ii<EMEM_HASH_SIZE;ii++){
    data[ii]=0;
  }
  for (int ii=ENEM_MIN_ADR;ii<=EMEM_MAX_ADR;ii++){
    if (emem_get_record(ii,&id,data)){
      Serial.print("REC,");
      Serial.print(ii,HEX);
      Serial.print(',');
      Serial.print(id,HEX);
      for (int jj=0;jj<EMEM_HASH_SIZE;jj++){
        Serial.print(',');
        Serial.print(*(data+jj),HEX);   
      }
      Serial.println();
    } else {
      Serial.println("Error reading EEPROM"); 
    }    
  }
}


/**
 * Function receives bytes from rfid reader and parses them into message
 * @return Returns true if message was received, false otherwise
 */
boolean rf_parse() {
  static int rf_idx = 0;
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
  }
  return false;
}

/**
 * Sends message to PC.
 * @param code Message code.
 * @param data Message data.
 * @param id Message id.
 */
void pc_print(char code,unsigned int id,unsigned char * data) {
  char buf[PC_MAX_BYTES+1+1];
  unsigned long token = 0;
  unsigned int pin = 0;
  snprintf(buf,PC_MAX_BYTES+1,"$%c,%04x,%02x%02x%02x%02x%02x%02x%02x%02x,%08lx\n",code,id,*(data),*(data+1),\
  *(data+2),*(data+3),*(data+4),*(data+5),*(data+6),*(data+7),token);  
  Serial.print(buf);
  Serial.println("Printing done");
}

/**
 * Handles communication with PC.
 */
void pc_comm(){  
  if (pc_parse()){
#ifdef DEBUG    
    Serial.print("I received: ");
    for (int ii=0;ii<PC_MAX_BYTES;ii++) {
      Serial.print(pc_bytes[ii],BYTE);
    }
#endif //DEBUG
    pc_bytes[PC_MAX_BYTES]=0;
    char code = 0;
    unsigned int id = 0;
    unsigned char data[EMEM_HASH_SIZE];
    for (int ii=0;ii<EMEM_HASH_SIZE;ii++){
      data[ii]=0;
    }
    unsigned long token = 0;
    int sres = sscanf(pc_bytes,"$%c,%4x,%02x%02x%02x%02x%02x%02x%02x%02x,%8lx\n",&code,&id,data+0,data+1,data+2,data+3,data+4,data+5,data+6,data+7,&token);    
#ifdef DEBUG    
    Serial.print("SCANNED: ");
    Serial.print(sres);
    Serial.print(", CODE: ");
    Serial.print(code,BYTE);
    Serial.print(", ID: ");
    Serial.print(id,HEX);
    Serial.print(", HASH: ");
    for (int ii=0;ii<EMEM_HASH_SIZE;ii++){
      Serial.print(data[ii],HEX);
      Serial.print(',');
    }  
    Serial.print(" TOKEN: ");    
    Serial.print(token>>24 & 0x0ff,HEX);    
    Serial.print(',');
    Serial.print(token>>16 & 0x0ff,HEX);    
    Serial.print(',');
    Serial.print(token>>8 & 0x0ff,HEX);
    Serial.print(',');
    Serial.print(token & 0x0ff,HEX);
    Serial.println();    
#endif //DEBUG
    if (token!=pc_token){
#ifdef DEBUG    
        Serial.println("WRONG TOKEN");
#endif //DEBUG
      pc_print('E',id,data);  
    } else if (pc_bytes[1]=='A' || pc_bytes[1]=='a'){
      //Add
      if (id==0) id=emem_find_free();
      if (emem_find_data(data)!=EMEM_INV_ADR){
        //Already have this data
        pc_print('E',id,data);
      } else if (emem_set_record(id,data)){
#ifdef DEBUG    
        Serial.println("ADD DONE");
#endif //DEBUG
        pc_print('C',id,data);  
      } else {
#ifdef DEBUG    
        Serial.println("ADD FAILED");
#endif //DEBUG
        pc_print('E',id,data);  
      }
    } else if (pc_bytes[1]=='R' || pc_bytes[1]=='r'){
//        //Revoke
      if (id==0) id=emem_find_data(data);
      if (emem_del_record(id)){
#ifdef DEBUG    
        Serial.println("REVOKE DONE");  
#endif //DEBUG    
        pc_print('K',id,data);
      } else {
#ifdef DEBUG    
        Serial.println("REVOKE FAILED");
#endif //DEBUG    
        pc_print('E',id,data);
      }
    } else if (pc_bytes[1]=='Z' || pc_bytes[1]=='z'){
      //Zero eeprom 
      Serial.println("ZERO");
      for (int ii=0;ii<1024;ii++){
        EEPROM.write(ii,0);

        if (ii%10==0) {
          Serial.print(".");
        }
      }
      Serial.println();
      Serial.print("DONE");
      Serial.println();      
    } else if (pc_bytes[1]=='P' || pc_bytes[1]=='p'){
      emem_print(); 
    } else if (pc_bytes[1]=='G' || pc_bytes[1]=='g'){
       pc_send_flag=true; 
    }
  }
}

/**
 * Function receives bytes from PC and parses them into message.
 * @return Returns true if message was received, false otherwise.
 */
boolean pc_parse(){
  static int pc_idx=0;
  char in = 0;	// for incoming serial data
  // send data only when you receive data:
  while (Serial.available() > 0) {
    //read the incoming byte:
    in = Serial.read();
    if (in=='$') {
      //Start
      pc_idx=0;
      pc_bytes[pc_idx]=in; 
      continue;
    }
    if (in=='\n'){
#ifdef DEBUG      
      Serial.print("Got newline ");
      Serial.println(pc_idx);
#endif //DEBUG      
      if (pc_idx==PC_MAX_BYTES-2) {
        pc_idx+=1;
        pc_bytes[pc_idx]=in;
        return true;
      } else {
        pc_idx=0;
        continue;
      }
    }
    if (pc_idx<PC_MAX_BYTES-2){
      pc_idx+=1;
      pc_bytes[pc_idx]=in;
      continue;
    } else{
      pc_idx=0;
      continue;
    }     
  }
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
//  (*p_id) = EMEM_INV_ADR;
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
//      unsigned int id = emem_find_data(data);            
//      if (pc_send_flag){
//        pc_send_flag=false;
//        pc_print('S',data,id);  
//        
//      }
//      if (id!=EMEM_INV_ADR){
        //User is authorized
//        (*p_id) = id;
        (*p_rfid) = data;
        return true;
//     }
    }
  } 
  return false;
}

/**
 * Sends hald command to rfid reader.
 */
void rf_halt() {
  //Halt tag
  rfid.print(255, BYTE);
  rfid.print(0, BYTE);
  rfid.print(1, BYTE);
  rfid.print(147, BYTE);
  rfid.print(148, BYTE);
}

/**
 * Sends seek command to rfid reader.
 */
void rf_seek(){
  //search for RFID tag
  rfid.print(255, BYTE);
  rfid.print(0, BYTE);
  rfid.print(1, BYTE);
  rfid.print(130, BYTE);
  rfid.print(131, BYTE); 
}

/**
 * Tries to read pin code from keyboard.
 * @param p_pin Pointer to space for pin.
 * @return True if code was red.
 */
boolean keypad_pin_get(unsigned long  * p_pin){
  
  static byte byte_cntr=0;
  byte k;
  switch (keypad_state){
    case SEND:
      //ask for pin
      keypad.print("#P");
      keypad_state=WAIT_RESP;
      break;
      
    case WAIT_RESP:
      //wait for response
      if( keypad.available() ){
        //store byte
        keypad_bytes[byte_cntr]=keypad.read();
        byte_cntr++;  
        
        if(byte_cntr==KEYPAD_RESP_SIZE){
          //resp. received - store pin and clear input buffer
          k=sscanf(keypad_bytes,"#%d",p_pin);
          byte_cntr=0;
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
 * Turnd off keyboard.
 */
void keypad_off(void)
{
  keypad.print("XX");
  keypad_state=SEND;
}
