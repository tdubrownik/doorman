#include <Arduino.h>
#include <EEPROM.h>

#include "config.h"

// from doorman.ino
extern unsigned char g_Hash[];

/**
 * Function reads record with given address from EEPROM. 
 * @param adr Address of record to read, or NULL.
 * @param id Pointer to space reserved for id.
 * @param data Pointer to space reserved for hash.
 * @return Function returns true when data was red, false otherwise.
 */
boolean emem_get_record(unsigned int adr,unsigned int * id,unsigned char * data) {
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
boolean emem_del_record(unsigned int adr) {
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
unsigned int emem_find_free() {
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
boolean emem_set_record(unsigned int adr,unsigned char * data) {
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
void emem_print() {  
  unsigned int id;  
  for (int ii=0;ii<EMEM_HASH_SIZE;ii++){
    g_Hash[ii] = 0;
  }
  for (int ii=ENEM_MIN_ADR;ii<=EMEM_MAX_ADR;ii++){
    if (emem_get_record(ii,&id,g_Hash)){
      Serial.print("REC,");
      Serial.print(ii,HEX);
      Serial.print(',');
      Serial.print(id,HEX);
      for (int jj=0;jj<EMEM_HASH_SIZE;jj++){
        Serial.print(',');
        Serial.print(*(g_Hash+jj),HEX);   
      }
      Serial.println();
    } else {
      Serial.println("Error reading EEPROM"); 
    }    
  }
}


