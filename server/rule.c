#include <stdio.h>
#include <string.h>
#include <ctype.h>
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include "rule.h"

struct rule_s
{
	pcre2_code* re;
	pcre2_match_data* match_data;
	rule_type_t type;
	int weight;
};

rule_t* rule_init(char* line)
{
	rule_t* rule = malloc(sizeof(rule_t));
	int errornumber;
	PCRE2_SIZE erroroffset;

	/* Get weight */	
				
	char* wc = line + strlen(line) - 2;
	while (wc > line && (isdigit(*wc) || isspace(*wc)))
		--wc;
		
	if (wc == line)
	{
		fprintf(stderr, "Corrupt pattern: %s\n", line);
		goto free_rule;
	}
	
	if (*wc == '-')
		--wc;
	++wc;

	sscanf(wc, "%d", &rule->weight);
	
	if (rule->weight == 0)
	{
		fprintf(stderr, "Zero weight pattern: %s\n", line);
		goto free_rule;
	}
	
	/* Get pattern */
	
	while (--wc > line && isspace(*wc));
	*(++wc) = '\0';
		
	PCRE2_SPTR pattern = (PCRE2_SPTR) line;		
	
	rule->re = pcre2_compile(
	pattern,               /* the pattern */
	PCRE2_ZERO_TERMINATED, /* indicates pattern is zero-terminated */
	0,                     /* default options */
	&errornumber,          /* for error number */
	&erroroffset,          /* for error offset */
	NULL);                 /* use default compile context */
	
	if (rule->re == NULL)
	{
		PCRE2_UCHAR buffer[256];
		pcre2_get_error_message(errornumber, buffer, sizeof(buffer));
		printf("PCRE2 compilation failed at offset %d: %s\n", (int) erroroffset, buffer);
		goto free_rule;
	}
	
	rule->match_data = pcre2_match_data_create_from_pattern(rule->re, NULL);
	
	printf("Loaded pattern: '%s' %d\n", line, rule->weight);
	goto done;

free_rule:
	free(rule);
	return NULL;
done:
	return rule;
}

void rule_free(rule_t* rule)
{
	pcre2_match_data_free(rule->match_data);
	pcre2_code_free(rule->re);
	free(rule);
}

int rule_check(rule_t* rule, const char* msg, msg_type_t* msg_type)
{	
	PCRE2_SPTR subject = (PCRE2_SPTR) msg;
	size_t subject_length = strlen(msg);
	
	int rc = pcre2_match(
		rule->re,             /* the compiled pattern */
		subject,              /* the subject string */
		subject_length,       /* the length of the subject */
		0,                    /* start at offset 0 in the subject */
		0,                    /* default options */
		rule->match_data,     /* block for storing the result */
		NULL);                /* use default match context */

	/* Matching failed: handle error cases */

	if (rc < 0)
	{
		switch(rc)
		{
			case PCRE2_ERROR_NOMATCH:
				*msg_type = MSG_TYPE_HAM;
				return 0;
			default:
				printf("Matching error %d\n", rc);
				*msg_type = MSG_TYPE_HAM;
				return rc;
		}
	}
	
	*msg_type = MSG_TYPE_SPAM;
	return 0;
}

int rule_get_weight(rule_t* rule)
{
	return rule->weight;
}