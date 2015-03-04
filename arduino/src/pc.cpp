#include <Arduino.h>
#include <EEPROM.h>

#include "config.h"
#include "pc.h"
#include "emem.h"

/** Global vatiable that holds last message from PC. Lenght is plus one for zero termination. */
char pc_bytes[PC_MAX_BYTES+1];
/** Flag set when next scan should send mid. */
boolean pc_send_flag = false;

// from doorman.ino
extern unsigned char global_hash[];

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
      // what the fuck?
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
 * Prints hash given as blob of data.
 * Hash length is defined as EMEM_HASH_SIZE.
 * @param hash Given hash.
 */
void pc_print_hash(uint8_t* hash) {
  for (int ii=0; ii<EMEM_HASH_SIZE; ii++) {
    Serial.print("0123456789abcdef"[hash[ii]>>4]);
    Serial.print("0123456789abcdef"[hash[ii]&0xf]);
  }
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
 * Handles communication with PC.
 */
void pc_comm(){  
  if (pc_parse()){
#ifdef DEBUG    
    Serial.print("I received: ");
    for (int ii=0;ii<PC_MAX_BYTES;ii++) {
      Serial.print(pc_bytes[ii], HEX);
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
    Serial.print(code, HEX);
    Serial.print(", ID: ");
    Serial.print(id, HEX);
    Serial.print(", HASH: ");
    for (int ii=0; ii<EMEM_HASH_SIZE; ii++){
      Serial.print(global_hash[ii], HEX);
      Serial.print(',');
    }  
    Serial.print(" TOKEN: ");    
    Serial.print(token>>24 & 0x0ff, HEX);    
    Serial.print(',');
    Serial.print(token>>16 & 0x0ff, HEX);    
    Serial.print(',');
    Serial.print(token>>8 & 0x0ff, HEX);
    Serial.print(',');
    Serial.print(token & 0x0ff, HEX);
    Serial.println();    
    
          
#endif //DEBUG
    if (0){
#ifdef DEBUG    
      Serial.println("WRONG TOKEN");
#endif //DEBUG
      pc_print('E', id, global_hash);  
    } else if (pc_bytes[1]=='A' || pc_bytes[1]=='a'){
      //Add
      if (id == 0)
      {
        id = emem_find_free();
      }
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
