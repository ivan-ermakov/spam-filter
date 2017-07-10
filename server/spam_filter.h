#ifndef SPAM_FILTER_H
#define SPAM_FILTER_H

#include "lib/sf.h"

int load_patterns();
void free_patterns();
int check_msg_type(const char* msg, msg_type_t* msg_type);

#endif
