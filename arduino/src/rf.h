#ifndef __DOORMAN_RF_H__
#define __DOORMAN_RF_H__

void rf_halt();
void rf_seek();
boolean rf_parse();
boolean rf_comm(unsigned long * p_rfid);

#endif
