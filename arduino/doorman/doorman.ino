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
#define EMEM_HASH_SIZE (32)
/** Flag that indicates (if defined) verbose mode (for diagnostic) */
//#define DEBUG 1
/**
 * Flag that indicates (if defined) keyboard simulation mode (for diagnostic).
 * In this mode keyboard always return pin.
 */
//#define DEBUG_KEYPAD_SIM 1
/** Max length of message from rfid reader. */
#define RF_MAX_BYTES (16)
/** Max length of message from or to PC. */
#define PC_MAX_BYTES (2+1+4+1+EMEM_HASH_SIZE*2+1+8+1)
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
unsigned char global_hash[EMEM_HASH_SIZE];
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
/** Pin correct command. */ 
#define keypad_pin_ok() keypad.print("#R")
/** Pin wrong  command. */ 
#define keypad_pin_wrong() keypad.print("#W")
/** Door control pin. */
#define DOOR_CTRN (4)

//Old protocol
//$g,00000000,0000,0000,386855c3
//$p,00000000,0000,0000,386855c3
//$a,00000000,0000,0000,386855c3
//Protocol with hashes
//Read next scan
//$g,0000,dfdd69691a7af8c141cef1ce69b5196b327f71f40b54da3c4e673283dcebd9b7,386855c3
//Print eeprom
//$p,0000,dfdd69691a7af8c141cef1ce69b5196b327f71f40b54da3c4e673283dcebd9b7,386855c3
//Add user
//$a,0000,dfdd69691a7af8c141cef1ce69b5196b327f71f40b54da3c4e673283dcebd9b7,386855c3
//Revoke user
//$r,0000,dfdd69691a7af8c141cef1ce69b5196b327f71f40b54da3c4e673283dcebd9b7,386855c3

/** Setup function. */
void setup()  {
  Serial.begin(19200);
  //rfid.begin(19200);
  keypad.begin(19200);
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
            pc_send_flag=false
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

/**
 * Function reads record with given address from EEPROM. 
 * @param adr Address of record to read, or NULL.
 * @param id Pointer to space reserved for id.
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
  for (int ii=EMEM_RECORD_SIZE-1;ii<EMEM_HASH_SIZE;ii++){
    *(data+ii) = 0;
  }  
  return true;  
}

/** 
 * Function deletes record with given address.
 * @param adr Record address.
 * @return True if record was deleted, false otherwise.
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
 * Finds record containing given data (hash).
 * @param data Data to be found.
 * @return Address of found record or EMEM_INV_ADR.
 */
unsigned int emem_find_data(unsigned char * data) {  
  for (int adr=ENEM_MIN_ADR;adr<=EMEM_MAX_ADR;adr++){    
    for (int ii=1;ii<EMEM_RECORD_SIZE;ii++){
      unsigned char edata=EEPROM.read(adr*EMEM_RECORD_SIZE+ii);
      if (edata!=*(data+ii-1)){        
        break;
      } else {
        if (ii==EMEM_RECORD_SIZE-1) {
          return adr;
        }
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
 * @param adr Record address.
 * @return True if data was written, false otherwise (adress out of range or failed write).
 */
boolean emem_set_record(unsigned int adr,unsigned char * data){
  if (adr>EMEM_MAX_ADR || adr<ENEM_MIN_ADR) return false;  
  EEPROM.write(adr*EMEM_RECORD_SIZE,(unsigned char)(adr&0x0ff));
  for (int ii=1;ii<EMEM_RECORD_SIZE;ii++) {
    EEPROM.write(adr*EMEM_RECORD_SIZE+ii,*(data+ii-1));  
  }
  //Ok, now try to read and compare.
  for (int ii=1;ii<EMEM_RECORD_SIZE;ii++) {
    if (EEPROM.read(adr*EMEM_RECORD_SIZE+ii) != *(data+ii-1)){
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
  for (int ii=0;ii<EMEM_HASH_SIZE;ii++){
    global_hash[ii]=0;
  }
  for (int ii=ENEM_MIN_ADR;ii<=EMEM_MAX_ADR;ii++){
    if (emem_get_record(ii,&id,global_hash)){
      Serial.print("REC,");
      Serial.print(ii,HEX);
      Serial.print(',');
      Serial.print(id,HEX);
      for (int jj=0;jj<EMEM_HASH_SIZE;jj++){
        Serial.print(',');
        Serial.print(*(global_hash+jj),HEX);   
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
 * Sends message to PC.
 * @param code Message code.
 * @param data Message data.
 * @param id Message id.
 */
void pc_print(char code,unsigned int id,unsigned char * data) {
  unsigned long token = 0;
  unsigned int pin = 0;
  char buf[12];
  snprintf(buf,12,"$%c,%04x,",code,id);
  Serial.print(buf);
  pc_print_hash(data);
  snprintf(buf,12,",%08lx\n",token);
  Serial.print(buf); 
}

/**
 * Prints hash given as blob of data.
 * Hash lenght is defined as EMEM_HASH_SIZE.
 * @param hash Given hash.
 */
void pc_print_hash(uint8_t* hash) {
  for (int ii=0; ii<EMEM_HASH_SIZE; ii++) {
    Serial.print("0123456789abcdef"[hash[ii]>>4]);
    Serial.print("0123456789abcdef"[hash[ii]&0xf]);
  }
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

    for (int ii=0;ii<EMEM_HASH_SIZE;ii++){
      global_hash[ii]=0;
    }
    unsigned long token = 0;
 
    //$a,0000,dfdd69691a7af8c141cef1ce69b5196b327f71f40b54da3c4e673283dcebd9b7,386855c3
    code=pc_bytes[1];
    char ids[5];
    for (int ii=0;ii<4;ii++){
      ids[ii]=pc_bytes[ii+3];
    }
    ids[4]='\0';
    sscanf(ids,"%d",&id);
    for (int ii=0;ii<32;ii++){
      char hex[3];
      hex[0]= pc_bytes[ii*2+8];
      hex[1]= pc_bytes[ii*2+8+1];
      hex[2]= 0;
      sscanf(hex,"%02x",global_hash+ii);
    }
    sscanf(pc_bytes+32*2+8+1,"%8lx",&token);

      
#ifdef DEBUG    
    Serial.print("CODE: ");
    Serial.print(code,BYTE);
    Serial.print(", ID: ");
    Serial.print(id,HEX);
    Serial.print(", HASH: ");
    for (int ii=0;ii<EMEM_HASH_SIZE;ii++){
      Serial.print(global_hash[ii],HEX);
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
      pc_print('E',id,global_hash);  
    } else if (pc_bytes[1]=='A' || pc_bytes[1]=='a'){
      //Add
      if (id==0) id=emem_find_free();
      if (emem_find_data(global_hash)!=EMEM_INV_ADR){
        //Already have this data
#ifdef DEBUG
        Serial.println("ADD FAILED, USER EXISTS");
#endif 
        pc_print('E',id,global_hash);
      } else if (emem_set_record(id,global_hash)){
#ifdef DEBUG    
        Serial.println("ADD DONE");
#endif //DEBUG
        pc_print('C',id,global_hash);  
      } else {
#ifdef DEBUG    
        Serial.println("ADD FAILED");
#endif //DEBUG
        pc_print('E',id,global_hash);  
      }
    } else if (pc_bytes[1]=='R' || pc_bytes[1]=='r'){
//        //Revoke
      if (id==0) id=emem_find_data(global_hash);
      if (emem_del_record(id)){
#ifdef DEBUG    
        Serial.println("REVOKE DONE");  
#endif //DEBUG    
        pc_print('K',id,global_hash);
      } else {
#ifdef DEBUG    
        Serial.println("REVOKE FAILED");
#endif //DEBUG    
        pc_print('E',id,global_hash);
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
    } else {
#ifdef DEBUG      
      Serial.println("Unknown command");
#endif //DEBUG      
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
      if (pc_idx==PC_MAX_BYTES-2) {;
        pc_bytes[pc_idx+1]=in;
        pc_idx=0;
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
void rf_seek(){
  //search for RFID tag
  /*rfid.print(255, BYTE);
  rfid.print(0, BYTE);
  rfid.print(1, BYTE);
  rfid.print(130, BYTE);
  rfid.print(131, BYTE); */
}

/**
 * Tries to read pin code from keyboard.
 * @param p_pin Pointer to space for pin.
 * @return True if code was red.
 */
boolean keypad_pin_get(unsigned long  * p_pin){  
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
 * Turnd off keyboard.
 */
void keypad_off(void) {
  keypad.print("XX");
  keypad_state=SEND;
}
