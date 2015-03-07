#include <Arduino.h>
#include <EEPROM.h>

#include "sha256.h"

#include "config.h"
#include "pc.h"
#include "emem.h"

/** Global variable that holds last message from PC. Length is plus one for zero termination. */
char pc_bytes[PC_MAX_BYTES+1];
/** Flag set when next scan should send mid. */
boolean pc_send_flag = false;
/** Global varuiable that holds last MAC from PC. Length is 32 bytes, no termination. */
unsigned char pc_mac[PC_MAC_SIZE];

// from doorman.ino
extern unsigned char g_Hash[];

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
      //Start condition
      pc_idx=0;
      pc_bytes[pc_idx]=in; 
      continue;
    } 
    if (in=='\n'){
#ifdef DEBUG      
      Serial.print("Got newline ");
      Serial.println(pc_idx);
#endif //DEBUG
      // Did we match the required length?
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
 * Message structure is:
 *  $<code>,<id>,<hash/data>,<token>,newline
 * eg.:
 *  $a,0000,dfdd69691a7af8c141cef1ce69b5196b327f71f40b54da3c4e673283dcebd9b7,\
 *  e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
 * where code is a command code, hash/data is a 32-byte piece of data in
 * lowercase hexdigest, and token is a 32-byte MAC in lowecase hexdigest
 *   
 */
void pc_comm(){  
  if (pc_parse()){
#ifdef DEBUG    
    Serial.print("I received: ");
    for (int ii=0;ii<PC_MAX_BYTES;ii++) {
      Serial.print(pc_bytes[ii]);
    }
#endif //DEBUG
    pc_bytes[PC_MAX_BYTES]=0;

    // Zero global hash
    for (int i = 0; i < EMEM_HASH_SIZE; i++) {
      g_Hash[i] = 0;
    }
    // Zero given MAC
    for (int i = 0; i < PC_MAC_SIZE; i++) {
      pc_mac[i] = 0;
    }
 
    // Get code
    char code = pc_bytes[1];
    // Get ID
    char id_string[5];
    for (int i = 0; i < 4; i++) {
      id_string[i] = pc_bytes[3 + i];
    }
    id_string[4] = '\0';
    unsigned short id;
    sscanf(id_string, "%d", &id);

    // Get hash
    for (int i = 0; i < EMEM_HASH_SIZE; i++) {
      char hex[3];
      hex[0] = pc_bytes[8+i*2];
      hex[1] = pc_bytes[8+i*2+1];
      hex[2] = 0;
      sscanf(hex, "%02x", g_Hash + i);
    }

    // Get MAC
    for (int i = 0; i < PC_MAC_SIZE; i++) {
      char hex[3];
      hex[0] = pc_bytes[73+i*2];
      hex[1] = pc_bytes[73+i*2+1];
      hex[2] = 0;
      sscanf(hex, "%02x", pc_mac + i);
    }
      
#ifdef DEBUG    
    Serial.print("CODE: ");
    Serial.print(code, HEX);
    Serial.print(", ID: ");
    Serial.print(id, HEX);
    Serial.print(", HASH: ");
    for (int i = 0; i < EMEM_HASH_SIZE; i++) {
      Serial.print(g_Hash[i], HEX);
      Serial.print(',');
    }  
    Serial.print(" TOKEN: ");    
    for (int i = 0; i < PC_MAC_SIZE; i++) {
      Serial.print(pc_mac[i], HEX);
      if (i == PC_MAC_SIZE - 1)
        Serial.println("");
      else
        Serial.print(',');
    }  
#endif

    // Check MAC
    pc_bytes[1+1+1+4+1+EMEM_HASH_SIZE*2] = '\0';
#ifdef DEBUG
    Serial.print("MACing ");
    Serial.println(pc_bytes);
#endif //DEBUG
    Sha256.initHmac((const uint8_t *)PC_MAC_SECRET, strlen(PC_MAC_SECRET));
    Sha256.print(pc_bytes);
    uint8_t *expected_mac = Sha256.resultHmac();
    // anti-timing attack - xor all MAC bytes together, or-fold, check if null
    uint8_t mac_result = 0;
    for (int i = 0; i < PC_MAC_SIZE; i++) {
      mac_result |= (pc_mac[i] ^ expected_mac[i]);
    }
#ifdef DEBUG
    Serial.print("Expected MAC: ");
    for (int i = 0; i < PC_MAC_SIZE; i++) {
      Serial.print(expected_mac[i], HEX);
      Serial.print(',');
    }
    Serial.println("");
#endif //DEBUG
          
    if (mac_result != 0){
#ifdef DEBUG    
      Serial.println("WRONG TOKEN");
#endif //DEBUG
      pc_print('E', id, g_Hash);  
      return;
    }
   
    if (code == 'A' || code == 'a') {
      //Add
      if (id == 0) {
        id = emem_find_free();
      }
      if (emem_find_data(g_Hash) != EMEM_INV_ADR) {
        //Already have this data
#ifdef DEBUG
        Serial.println("ADD FAILED, USER EXISTS");
#endif 
        pc_print('E', id, g_Hash);
        return;
      }
      if (emem_set_record(id, g_Hash)) {
#ifdef DEBUG    
        Serial.println("ADD DONE");
#endif //DEBUG
        pc_print('C', id, g_Hash);  
        return;
      }

#ifdef DEBUG    
      Serial.println("ADD FAILED");
#endif //DEBUG
      pc_print('E', id, g_Hash);  
      return;
    }
    
    if (pc_bytes[1]=='R' || pc_bytes[1]=='r') {
      if (id == 0)
        id=emem_find_data(g_Hash);
      if (emem_del_record(id)){
#ifdef DEBUG    
        Serial.println("REVOKE DONE");  
#endif //DEBUG    
        pc_print('K',id,g_Hash);
      } else {
#ifdef DEBUG    
        Serial.println("REVOKE FAILED");
#endif //DEBUG    
        pc_print('E',id,g_Hash);
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
