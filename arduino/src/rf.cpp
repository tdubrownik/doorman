#include <Arduino.h>
#include <SM130.h>
#include <Wire.h>

#include "config.h"

/** Global variable that holds RFID reader object. */
SM130 rfid;

/**
 * Init NFC subsystem
 */
void rf_init(void) {
  Wire.begin();
  rfid.address = 0x30;
  rfid.reset();
#ifdef DEBUG
    Serial.print("SM130 firmware version: ");
    Serial.println(rfid.getFirmwareVersion());
#endif //DEBUG
}

/**
 * Resets the NFC controller
 */

void rf_reset() {
  rfid.reset();
}

/**
 * Sends seek command to rfid reader.
 */
void rf_seek() {
  rfid.seekTag();
}

/**
 * Handles communication with rfid reader.
 * @param p_rfid Pointer to space where mifare id will be written.
 * @return Returns true when authorized user rfid was rd.
 */
boolean rf_comm(unsigned long * p_rfid) {
  static unsigned long last_millis_seek = 0;
  static unsigned long last_millis_reset = 0;

  unsigned long new_millis = millis();

  (*p_rfid) = 0;
  if (new_millis < last_millis_seek) {
    last_millis_seek = new_millis; 
  } else if ((new_millis - last_millis_seek) > RF_PERIOD_SEEK_MS) {
    rf_seek();
    last_millis_seek = new_millis;
  }
  if (new_millis < last_millis_reset) {
    last_millis_reset = new_millis;
  } else if ((new_millis - last_millis_reset) > RF_PERIOD_RESET_MS) {
    rf_reset();
    last_millis_reset = new_millis;
  }

  if (rfid.available()){
    //Message received
    uint8_t *rf_bytes = rfid.getTagNumber();
#ifdef DEBUG
    Serial.print("RFID Tag: ");
    for (int i = 0; i < 8; i++) {
      Serial.print(rf_bytes[i], HEX);
      Serial.print(",");
    }
    Serial.println();
#endif //DEBUG
      unsigned long data=(long(rf_bytes[3])<<24)|(long(rf_bytes[2])<<16)|(long(rf_bytes[1])<<8)|(long(rf_bytes[0]));
      (*p_rfid) = data;
      return true;
  } 
  return false;
}

