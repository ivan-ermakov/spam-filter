#ifndef SPAM_FILTER_H
#define SPAM_FILTER_H

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include "lib/sf.h"

typedef enum
{
    SF_EOK = 0,
    SF_EFAIL
} sf_err_t;

typedef struct spam_filter_s
{
    int patterns_size;
    int patterns_capacity;
    int* patterns_weight;
    pcre2_code** patterns;
    pcre2_match_data** pattern_match_data;
} spam_filter_t;

sf_err_t spam_filter_init(spam_filter_t* sf, char* file_name);
void spam_filter_deinit(spam_filter_t* sf);
sf_err_t spam_filter_check_msg(spam_filter_t* sf, const char* msg, msg_type_t* msg_type);

#endif
