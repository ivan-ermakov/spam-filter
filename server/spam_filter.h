#ifndef SPAM_FILTER_H
#define SPAM_FILTER_H

#include "lib/sf.h"

int load_patterns();
void free_patterns();
msg_type_t check_msg_type(const char* msg);

#endif
