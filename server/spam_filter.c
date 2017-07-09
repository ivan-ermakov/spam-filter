#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include "spam_filter.h"

static int patterns_size = 0;
static int patterns_capacity = 0;
static int* patterns_weight = NULL;
static pcre2_code** patterns = NULL;
static pcre2_match_data** pattern_match_data = NULL;

int load_patterns()
{	
	char* line = NULL;
	size_t line_len = 0;
	ssize_t ret;
	int errornumber;
	PCRE2_SIZE erroroffset;
	
	FILE* f = fopen("patterns.txt", "r");
	
	if (!f)
	{
		fprintf(stderr, "Unable to load patterns\n");
		return 0;
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
		
		if (patterns_size == patterns_capacity)
		{
			patterns_capacity = patterns_capacity == 0 ? 16 : patterns_capacity * 2;
			patterns_weight = realloc(patterns_weight, patterns_capacity * sizeof(int));
			patterns = realloc(patterns, patterns_capacity * sizeof(pcre2_code*));
			pattern_match_data = realloc(pattern_match_data, patterns_capacity * sizeof(pcre2_match_data*));
		}
		
		patterns[patterns_size] = re;
		patterns_weight[patterns_size] = weight;
		pattern_match_data[patterns_size] = pcre2_match_data_create_from_pattern(re, NULL);
		++patterns_size;
		
		printf("Loaded pattern: '%s' %d\n", line, weight);
	}
	
	fclose(f);
	free(line);
	
	printf("Loaded %d patterns\n", patterns_size);
	
	return patterns_size;
}

void free_patterns()
{
	for (int i = 0; i < patterns_size; ++i)
	{
		pcre2_match_data_free(pattern_match_data[i]);
		pcre2_code_free(patterns[i]);
	}
		
	free(patterns);
	free(patterns_weight);
}

msg_type_t check_msg_type(const char* msg)
{
	int score = 0;
	const int MIN_SPAM_SCORE = 90;
	
	PCRE2_SPTR subject = (PCRE2_SPTR) msg;
	size_t subject_length = strlen(msg);
	
	for (int i = 0; i < patterns_size; ++i)
	{
		int rc = pcre2_match(
		  patterns[i],                   /* the compiled pattern */
		  subject,              /* the subject string */
		  subject_length,       /* the length of the subject */
		  0,                    /* start at offset 0 in the subject */
		  0,                    /* default options */
		  pattern_match_data[i],           /* block for storing the result */
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
					continue;
			}
		}
		
		score += patterns_weight[i];
	}
	
	return score >= MIN_SPAM_SCORE ? MSG_TYPE_SPAM : MSG_TYPE_HAM;
}
