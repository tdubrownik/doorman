#include <Arduino.h>
#include <Adafruit_PN532.h>

#include "config.h"

/** Global variable that holds RFID reader object. */
Adafruit_PN532 rfid(3, 4);

/**
 * Init NFC subsystem
 */
void rf_init(void) {
  rfid.begin();
#ifdef DEBUG
    uint32_t versiondata = rfid.getFirmwareVersion();
    Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
    Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
    Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
#endif //DEBUG
  rfid.setPassiveActivationRetries(0xFF);
  rfid.SAMConfig();
}

uint8_t mifareBytes[7];
/**
 * Handles communication with rfid reader.
 * @param p_rfid Pointer to space where mifare id will be written.
 * @return Returns true when authorized user rfid was rd.
 */
boolean rf_comm(unsigned long * p_rfid) {
  bool success;
  uint8_t mifareLength;

  success = rfid.readPassiveTargetID(PN532_MIFARE_ISO14443A,
                                    &mifareBytes[0], &mifareLength);
  if (success){
#ifdef DEBUG
    Serial.print("RFID Tag: ");
    for (int i = 0; i < mifareLength; i++) {
      Serial.print(mifareBytes[i], HEX);
      Serial.print(",");
    }
    Serial.println();
#endif //DEBUG
      unsigned long data=(long(mifareBytes[3])<<24)|(long(mifareBytes[2])<<16)|(long(mifareBytes[1])<<8)|(long(mifareBytes[0]));
      (*p_rfid) = data;
      return true;
  } 
  return false;
}

