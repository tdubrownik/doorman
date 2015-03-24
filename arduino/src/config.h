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
/** Length of MAC **/
#define PC_MAC_SIZE (32)
/** Max length of message from or to PC. */
#define PC_MAX_BYTES (2+1+4+1+EMEM_HASH_SIZE*2+1+PC_MAC_SIZE*2+1)
/** Period for quering rfid reader. */
#define RF_PERIOD_SEEK_MS (500)
#define RF_PERIOD_RESET_MS (30 * 60 * 1000)

/** PC Communications MAC secret **/
#define PC_MAC_SECRET ("hackme")
