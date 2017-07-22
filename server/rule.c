#include <stdio.h>
#include <string.h>
#include <ctype.h>
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include "rpn.h"
#include "rule.h"
#include "spam_filter.h"

struct rule_s
{
	int id;
	int weight;
	rule_type_t type;	
	pcre2_code* re;
	pcre2_match_data* match_data;	
	char* rpn;
};

rule_t* rule_init(char* line)
{
	rule_t* rule = malloc(sizeof(rule_t));

	char type[16];
	sscanf(line, "%d %s %d", &rule->id, type, &rule->weight);

	if (rule->id < 0)
	{
		fprintf(stderr, "Invalid rule id: %d\n", rule->id);
		goto free_rule;
	}

	if (strcmp(type, "regex") == 0)
		rule->type = RULE_REGEX;
	else if (strcmp(type, "bool") == 0)
		rule->type = RULE_BOOL;
	else
	{
		fprintf(stderr, "Invalid rule type: %s\n", type);
		goto free_rule;
	}
	
	if (rule->weight == 0)
	{
		fprintf(stderr, "Invalid rule weight: %d\n", rule->weight);
		goto free_rule;
	}
	
	/* Get pattern */
	
	while (*++line && (isdigit(*line) || isspace(*line))); /* skip first 3 args */
	while (*++line && (isalpha(*line) || isspace(*line)));
	while (*++line && (isdigit(*line)));
	while (*++line && (isspace(*line)));

	line[strlen(line) - 1] = '\0'; /* skip \n */
	
	if (rule->type == RULE_REGEX)
	{
		int errornumber;
		PCRE2_SIZE erroroffset;
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
	}
	else
	{
		rule->rpn = rpn_init(line);
		if (!rule->rpn)
			goto free_rule;
	}
	
	printf("Loaded rule: %d %s %d '%s'\n", rule->id, type, rule->weight, rule->type == RULE_BOOL ? rule->rpn : line);
	goto done;

free_rule:
	free(rule);
	return NULL;
done:
	return rule;
}

void rule_free(rule_t* rule)
{
	if (rule->type == RULE_REGEX)
	{
		pcre2_match_data_free(rule->match_data);
		pcre2_code_free(rule->re);
	}
	else
		rpn_free(rule->rpn);

	free(rule);
}

int rule_check(rule_t* rule, spam_filter_t* sf, const char* msg, msg_type_t* msg_type)
{		
	if (rule->type == RULE_REGEX)
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
	}
	else
	{
		int ret = 0;
		int* vals = NULL;
		int vals_size = 0;
		char* token;
		char* exp = rule->rpn;

		ret = get_token(exp, &token);

		while (ret > 0)
		{
			exp += ret;

			if (isdigit(*token)) /* if value */
			{
				int id = strtol(token, NULL, 10);
				*msg_type = MSG_TYPE_HAM;
				int ret = rule_check(spam_filter_get_rule(sf, id), sf, msg, msg_type); /* write value */
				if (ret < 0)
				{
					fprintf(stderr, "rule_check error %d\n", ret);
					ret = 1;
					goto done;
				}

				vals = realloc(vals, ++vals_size * sizeof(int));
				vals[vals_size - 1] = *msg_type;
			}
			else if (strcmp(token, "!") == 0)
			{
				if (vals_size < 1)
				{
					fprintf(stderr, "Not enough values\n");
					ret = 2;
					goto done;
				}

				vals[vals_size - 1] = !vals[vals_size - 1];
			}
			else if (strcmp(token, "&&") == 0)
			{
				if (vals_size < 2)
				{
					fprintf(stderr, "Not enough values\n");
					ret = 2;
					goto done;
				}

				--vals_size;
				vals[vals_size - 1] = vals[vals_size - 1] && vals[vals_size];
			}
			else if (strcmp(token, "||") == 0)
			{
				if (vals_size < 2)
				{
					fprintf(stderr, "Not enough values\n");
					ret = 2;
					goto done;
				}

				--vals_size;
				vals[vals_size - 1] = vals[vals_size - 1] || vals[vals_size];
			}
			else
			{
				fprintf(stderr, "Unknown operator: %s\n", token);
				ret = 3;
				goto done;
			}

			free(token);
			ret = get_token(exp, &token);
		}

		if (vals_size == 1)
		{
			*msg_type = vals[0];
			printf("Rule %d: %d\n", rule->id, *msg_type);
			ret = 0;
		}
		else
		{
			fprintf(stderr, "Invalid values\n");
			ret = 4;
		}

		done:
		free(token);
		free(vals);
		return ret;
	}

	return 0;
}

int rule_get_id(rule_t* rule)
{
	return rule->id;
}

int rule_get_weight(rule_t* rule)
{
	return rule->weight;
}