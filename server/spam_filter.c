#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "rpn.h"
#include "rule.h"
#include "spam_filter.h"

enum { MIN_SPAM_SCORE = 90 };

struct spam_filter_s
{
    int rules_size;
    rule_t** rules;
};


static int spam_filter_rule_check(spam_filter_t* sf, rule_t* rule, const char* msg, msg_type_t* msg_type);

spam_filter_t* spam_filter_init(char* file_name)
{		
	FILE* f = fopen(file_name, "r");
	
	if (!f)
	{
		fprintf(stderr, "Unable to load rules\n");
		return NULL;
	}
	
	spam_filter_t* sf = (spam_filter_t*) malloc(sizeof(spam_filter_t));
	char* line = NULL;
	size_t line_len = 0;
	ssize_t ret;
	rule_t* rule;

	sf->rules_size = 0;
	sf->rules = NULL;
	
	while ((ret = getline(&line, &line_len, f)) > 0)
	{	
		rule = rule_init(line);

		if (!rule)
			continue;

		sf->rules = realloc(sf->rules, ++sf->rules_size * sizeof(rule_t*));		
		sf->rules[sf->rules_size - 1] = rule;
	}
	
	fclose(f);
	free(line);
	
	printf("Loaded %d rules\n", sf->rules_size);
	
	return sf;
}

void spam_filter_free(spam_filter_t* sf)
{
	for (int i = 0; i < sf->rules_size; ++i)
		rule_free(sf->rules[i]);
		
	free(sf->rules);
	free(sf);
}

static int spam_filter_rule_check_bool(spam_filter_t* sf, rule_t* rule, const char* msg, msg_type_t* msg_type)
{
	int ret = 0;
	int* vals = NULL;
	int vals_size = 0;
	char* token;
	char* exp = rule_get_rpn(rule);

	ret = get_token(exp, &token);

	while (ret > 0)
	{
		exp += ret;

		if (isdigit(*token)) /* if value */
		{
			int id = strtol(token, NULL, 10);
			if (id == rule_get_id(rule))
			{
				fprintf(stderr, "Rule %d circle reference\n", id);
				ret = -1;
				goto done;
			}

			*msg_type = MSG_TYPE_HAM;

			rule_t* r = spam_filter_get_rule(sf, id);
			if (!r)
			{
				fprintf(stderr, "Invalid rule %d referenced\n", id);
				ret = -2;
				goto done;
			}

			int ret = spam_filter_rule_check(sf, r, msg, msg_type); /* write value */
			if (ret < 0)
			{
				fprintf(stderr, "rule_check error %d\n", ret);
				ret = -3;
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
				ret = -4;
				goto done;
			}

			vals[vals_size - 1] = !vals[vals_size - 1];
		}
		else if (strcmp(token, "&&") == 0)
		{
			if (vals_size < 2)
			{
				fprintf(stderr, "Not enough values\n");
				ret = -4;
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
				ret = -4;
				goto done;
			}

			--vals_size;
			vals[vals_size - 1] = vals[vals_size - 1] || vals[vals_size];
		}
		else
		{
			fprintf(stderr, "Unknown operator: %s\n", token);
			ret = -5;
			goto done;
		}

		free(token);
		ret = get_token(exp, &token);
	}

	if (vals_size == 1)
	{
		*msg_type = vals[0];
		ret = 0;
	}
	else
	{
		fprintf(stderr, "Invalid values\n");
		ret = -6;
	}

done:
	free(token);
	free(vals);
	return ret;
}

static int spam_filter_rule_check(spam_filter_t* sf, rule_t* rule, const char* msg, msg_type_t* msg_type)
{
	if (rule_get_type(rule) == RULE_REGEX)
		return rule_check(rule, msg, msg_type);
	else
		return spam_filter_rule_check_bool(sf, rule, msg, msg_type);
}

sf_err_t spam_filter_check_msg(spam_filter_t* sf, const char* msg, msg_type_t* msg_type)
{
	int score = 0;
	
	for (int i = 0; i < sf->rules_size; ++i)
	{
		int ret = spam_filter_rule_check(sf, sf->rules[i], msg, msg_type);
		if (ret)
			continue;

		if (*msg_type == MSG_TYPE_SPAM)		
		{
			score += rule_get_weight(sf->rules[i]);
			printf("Rule %d triggered\n", rule_get_id(sf->rules[i]));
		}
	}
	
	*msg_type = score >= MIN_SPAM_SCORE ? MSG_TYPE_SPAM : MSG_TYPE_HAM;
	return SF_EOK;
}

rule_t* spam_filter_get_rule(spam_filter_t* sf, int id)
{
	for (int i = 0; i < sf->rules_size; ++i)
		if (rule_get_id(sf->rules[i]) == id)
			return sf->rules[i];

	return NULL;
}
