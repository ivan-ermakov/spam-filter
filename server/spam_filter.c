#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "spam_filter.h"

sf_err_t spam_filter_init(spam_filter_t* sf, char* file_name)
{	
	char* line = NULL;
	size_t line_len = 0;
	ssize_t ret;
	int errornumber;
	PCRE2_SIZE erroroffset;

	sf->patterns_size = 0;
	sf->patterns_capacity = 0;
	sf->patterns_weight = NULL;
	sf->patterns = NULL;
   	sf->pattern_match_data = NULL;
	
	FILE* f = fopen(file_name, "r");
	
	if (!f)
	{
		fprintf(stderr, "Unable to load patterns\n");
		return SF_EFAIL;
	}
	
	while ((ret = getline(&line, &line_len, f)) > 0)
	{		
		/* Get weight */	
				
		char* wc = line + strlen(line) - 2;
		while (wc > line && (isdigit(*wc) || isspace(*wc)))
			--wc;
			
		if (wc == line)
		{
			fprintf(stderr, "Corrupt pattern: %s\n", line);
			continue;
		}
		
		if (*wc == '-')
			--wc;
		++wc;
		
		int weight = 0;
		sscanf(wc, "%d", &weight);
		
		if (weight == 0)
		{
			fprintf(stderr, "Zero weight pattern: %s\n", line);
			continue;
		}
		
		/* Get pattern */
		
		while (--wc > line && isspace(*wc));
		*(++wc) = '\0';
			
		PCRE2_SPTR pattern = (PCRE2_SPTR) line;		
		
		pcre2_code* re = pcre2_compile(
		pattern,               /* the pattern */
		PCRE2_ZERO_TERMINATED, /* indicates pattern is zero-terminated */
		0,                     /* default options */
		&errornumber,          /* for error number */
		&erroroffset,          /* for error offset */
		NULL);                 /* use default compile context */
		
		if (re == NULL)
		{
			PCRE2_UCHAR buffer[256];
			pcre2_get_error_message(errornumber, buffer, sizeof(buffer));
			printf("PCRE2 compilation failed at offset %d: %s\n", (int) erroroffset, buffer);
			continue;
		}
		
		if (sf->patterns_size == sf->patterns_capacity)
		{
			sf->patterns_capacity = sf->patterns_capacity == 0 ? 16 : sf->patterns_capacity * 2;
			sf->patterns_weight = realloc(sf->patterns_weight, sf->patterns_capacity * sizeof(int));
			sf->patterns = realloc(sf->patterns, sf->patterns_capacity * sizeof(pcre2_code*));
			sf->pattern_match_data = realloc(sf->pattern_match_data, sf->patterns_capacity * sizeof(pcre2_match_data*));
		}
		
		sf->patterns[sf->patterns_size] = re;
		sf->patterns_weight[sf->patterns_size] = weight;
		sf->pattern_match_data[sf->patterns_size] = pcre2_match_data_create_from_pattern(re, NULL);
		++sf->patterns_size;
		
		printf("Loaded pattern: '%s' %d\n", line, weight);
	}
	
	fclose(f);
	free(line);
	
	printf("Loaded %d patterns\n", sf->patterns_size);
	
	return sf->patterns_size == 0;
}

void spam_filter_deinit(spam_filter_t* sf)
{
	for (int i = 0; i < sf->patterns_size; ++i)
	{
		pcre2_match_data_free(sf->pattern_match_data[i]);
		pcre2_code_free(sf->patterns[i]);
	}
		
	free(sf->patterns);
	free(sf->patterns_weight);
	free(sf->pattern_match_data);
	sf->patterns_size = 0;
}

sf_err_t spam_filter_check_msg(spam_filter_t* sf, const char* msg, msg_type_t* msg_type)
{
	int score = 0;
	const int MIN_SPAM_SCORE = 90;
	
	PCRE2_SPTR subject = (PCRE2_SPTR) msg;
	size_t subject_length = strlen(msg);
	
	for (int i = 0; i < sf->patterns_size; ++i)
	{
		int rc = pcre2_match(
		  sf->patterns[i],                   /* the compiled pattern */
		  subject,              /* the subject string */
		  subject_length,       /* the length of the subject */
		  0,                    /* start at offset 0 in the subject */
		  0,                    /* default options */
		  sf->pattern_match_data[i],           /* block for storing the result */
		  NULL);                /* use default match context */

		/* Matching failed: handle error cases */

		if (rc < 0)
		{
			switch(rc)
			{
				case PCRE2_ERROR_NOMATCH:
					continue;
				default:
					printf("Matching error %d\n", rc);
					*msg_type = MSG_TYPE_HAM;
					return rc;
			}
		}
		
		score += sf->patterns_weight[i];
	}
	
	*msg_type = score >= MIN_SPAM_SCORE ? MSG_TYPE_SPAM : MSG_TYPE_HAM;
	return SF_EOK;
}
