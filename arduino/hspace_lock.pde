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

//Defines
/** Record size. */
#define EMEM_RECORD_SIZE (7)
/** First record ID. */
#define ENEM_MIN_ADR (1)
/** Last record ID. */
#define EMEM_MAX_ADR (100)
/** Invalid record ID. */
#define EMEM_INV_ADR (0)
/** Flag that indicates (if defined) verbose mode (for diagnostic) */
//#define DEBUG 1
/** Max length of message from rfid reader. */
#define RF_MAX_BYTES (32)
/** Max length of message from or to PC. */
#define PC_MAX_BYTES (2+1+8+1+4+1+4+1+8+1)
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

//$g,00000000,0000,0000,386855c3
//$p,00000000,0000,0000,386855c3
//$a,00000000,0000,0000,386855c3

/** Setup function. */
void setup()  {
  Serial.begin(19200);
  rfid.begin(19200);
  delay(10);
  rf_halt();
#ifdef DEBUG  
  Serial.println("Ready");
#endif //DEBUG  
}

/** Main loop. */
void loop(){   
  //Communication with mifare reader.
  rf_comm();    
  //Communication with PC (optional).
  pc_comm();
}

/**
 * Function reads record with given id from EEPROM. 
 * @param adr Address of record to read.
 * @param data Pointer to space for data.
 * @param id Pointer to space for id.
 * @return Function returns true when data was red, false otherwise.
 */
boolean emem_get_record(unsigned int adr,unsigned long * data,unsigned int * id,unsigned int * pin){
  if (adr>EMEM_MAX_ADR || adr<ENEM_MIN_ADR) {
    //Address out of range.
    return false;  
  }
  byte rec[EMEM_RECORD_SIZE];
  for (int ii=0;ii<EMEM_RECORD_SIZE;ii++){
    rec[ii] = EEPROM.read(adr*EMEM_RECORD_SIZE+ii);
  }
  (*id)=rec[0];
  (*data)=(long(rec[1])<<24)|(long(rec[2])<<16)|(long(rec[3])<<8)|(long(rec[4]));
  (*pin)=(rec[5]<<8)|(rec[6]);
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
unsigned int emem_find_data(unsigned long data) {  
  for (int adr=ENEM_MIN_ADR;adr<=EMEM_MAX_ADR;adr++){    
    if (EEPROM.read(adr*EMEM_RECORD_SIZE+1)==(data>>24 & 0x0ff)){
      if (EEPROM.read(adr*EMEM_RECORD_SIZE+2)==(data>>16 & 0x0ff)){
        if (EEPROM.read(adr*EMEM_RECORD_SIZE+3)==(data>>8 & 0x0ff)){
          if (EEPROM.read(adr*EMEM_RECORD_SIZE+4)==(data & 0x0ff)){
            return adr;    
          } else {
            continue; 
          } 
        } else {
          continue; 
        }       
      } else {
        continue; 
      } 
    } else {
      continue; 
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
boolean emem_set_record(unsigned long data,unsigned int id,unsigned int pin){
  if (id>EMEM_MAX_ADR || id<ENEM_MIN_ADR) return false;  
  byte rec[EMEM_RECORD_SIZE];
  rec[0]=id;
  rec[1]=(data>>24) & 0x0ff;
  rec[2]=(data>>16) & 0x0ff;
  rec[3]=(data>>8) & 0x0ff;
  rec[4]=(data) & 0x0ff;  
  rec[5]=(pin>>8) & 0x0ff;
  rec[6]=pin & 0x0ff;
  for (int ii=0;ii<EMEM_RECORD_SIZE;ii++) {
    EEPROM.write(id*EMEM_RECORD_SIZE+ii,rec[ii]);  
  }
  //Ok, now try to read and compare.
  for (int ii=0;ii<EMEM_RECORD_SIZE;ii++) {
    if (EEPROM.read(id*EMEM_RECORD_SIZE+ii) != rec[ii]){
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
  unsigned long data;
  unsigned int pin;
  for (int ii=ENEM_MIN_ADR;ii<=EMEM_MAX_ADR;ii++){
    if (emem_get_record(ii,&data,&id,&pin)){
      Serial.print("REC,");
      Serial.print(ii,HEX);
      Serial.print(',');
      Serial.print(id,HEX);
      Serial.print(',');
      Serial.print(data,HEX);   
      Serial.print(',');
      Serial.println(pin,HEX);
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
void pc_print(char code,unsigned long data,unsigned int id) {
  char buf[PC_MAX_BYTES+1];
  unsigned long token = 0;
  unsigned int pin = 0;
  snprintf(buf,PC_MAX_BYTES+1,"$%c,%08lx,%04x,%04x,%08lx\n",code,data,id,pin,token);
  Serial.print(buf);
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
    unsigned long data = 0;
    unsigned int pin =0;
    unsigned long token = 0;
    sscanf(pc_bytes,"$%c,%8lx,%4lx,%4lx,%8lx\n",&code,&data,&id,&pin,&token);
#ifdef DEBUG    
    Serial.print("CODE: ");
    Serial.print(code,BYTE);
    Serial.print(" DATA: ");
    Serial.print(data>>24 & 0x0ff,HEX);    
    Serial.print(',');
    Serial.print(data>>16 & 0x0ff,HEX);    
    Serial.print(',');
    Serial.print(data>>8 & 0x0ff,HEX);
    Serial.print(',');
    Serial.print(data & 0x0ff,HEX);
    Serial.print(" ID: ");
    Serial.print(id,HEX);
    Serial.print(" PIN: ");
    Serial.print(pin,HEX);    
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
      pc_print('E',data,id);  
    } else if (pc_bytes[1]=='A' || pc_bytes[1]=='a'){
      //Add
      if (id==0) id=emem_find_free();
      if (emem_find_data(data)!=EMEM_INV_ADR){
        //Already have this data
        pc_print('E',data,id);
      } else if (emem_set_record(data,id,pin)){
#ifdef DEBUG    
        Serial.println("ADD DONE");
#endif //DEBUG
        pc_print('C',data,id);  
      } else {
#ifdef DEBUG    
        Serial.println("ADD FAILED");
#endif //DEBUG
        pc_print('E',data,id);  
      }
    } else if (pc_bytes[1]=='R' || pc_bytes[1]=='r'){
        //Revoke
      if (id==0) id=emem_find_data(data);
      if (emem_del_record(id)){
#ifdef DEBUG    
        Serial.println("REVOKE DONE");  
#endif //DEBUG    
        pc_print('K',data,id);
      } else {
#ifdef DEBUG    
        Serial.println("REVOKE FAILED");
#endif //DEBUG    
        pc_print('E',data,id);
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
 */
void rf_comm() {
  static unsigned long last_millis = 0;
  static unsigned long new_millis = 0;
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
      unsigned int id = emem_find_data(data);            
      if (pc_send_flag){
        pc_send_flag=false;
        pc_print('S',data,id);  
      
      }
    }
  } 
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




