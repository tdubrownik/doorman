#ifndef __DOORMAN_EMEM_H__
#define __DOORMAN_EMEM_H__

boolean emem_get_record(unsigned int adr, unsigned int * id, unsigned char * data);
boolean emem_del_record(unsigned int adr);
unsigned int emem_find_id(unsigned int id);
unsigned int emem_find_data(unsigned char * data);
unsigned int emem_find_free();
boolean emem_set_record(unsigned int adr,unsigned char * data);
void emem_print();


#endif
