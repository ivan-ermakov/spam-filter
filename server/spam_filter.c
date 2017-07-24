#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rule.h"
#include "spam_filter.h"

enum { MIN_SPAM_SCORE = 90 };

struct spam_filter_s
{
    int rules_size;
    rule_t** rules;
};

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

sf_err_t spam_filter_check_msg(spam_filter_t* sf, const char* msg, msg_type_t* msg_type)
{
	int score = 0;
	
	for (int i = 0; i < sf->rules_size; ++i)
	{
		int ret = rule_check(sf->rules[i], sf, msg, msg_type);

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
