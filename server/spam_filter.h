#ifndef SPAM_FILTER_H
#define SPAM_FILTER_H

#include "lib/sf.h"

typedef enum
{
    SF_EOK = 0,
    SF_EFAIL
} sf_err_t;

typedef struct spam_filter_s spam_filter_t;

spam_filter_t* spam_filter_init(char* file_name);
void spam_filter_free(spam_filter_t* sf);
sf_err_t spam_filter_check_msg(spam_filter_t* sf, const char* msg, msg_type_t* msg_type);

#endif
