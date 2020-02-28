#ifndef CONNMGR_H
#define CONNMGR_H

#include "sbuffer.h"

#ifndef TIMEOUT
    #error TIMEOUT not set
#endif

void connmgr_listen(int port_number, sbuffer_t** buffer);
void connmgr_free();


#endif /* CONNMGR_H */
