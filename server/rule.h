#include "lib/sf.h"

typedef enum
{
	RULE_REGEX = 0,
	RULE_BOOL
} rule_type_t;

typedef struct rule_s rule_t;

rule_t* rule_init(char* line);
void rule_free(rule_t* rule);
int rule_check(rule_t* rule, const char* msg, msg_type_t* msg_type);
int rule_get_weight(rule_t* rule);