#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "rpn.h"

typedef enum
{
	OP_ASSOC_LEFT = 0,
	OP_ASSOC_RIGHT
} op_assoc_t;

static int op_get_precedence(char* op)
{
	if (strcmp(op, "!") == 0)
		return 3;
	else if (strcmp(op, "&&") == 0)
		return 2;
	else if (strcmp(op, "||") == 0)
		return 1;

	return 0;
}

static op_assoc_t op_get_associativity(char* op)
{
	if (strcmp(op, "!") == 0)
		return OP_ASSOC_RIGHT;

	return OP_ASSOC_LEFT;
}

int get_token(char* exp, char** token)
{
	char* start = exp;
	*token = NULL;

	while (isspace(*start)) ++start;

	if (!*start)
		return 0;

	char* end = start + 1;
	if (isdigit(*start))
	{
		while (isdigit(*end)) ++end;
	}
	else if (*start == '&' || *start == '|')
		++end;

	int len = end - start;
	*token = malloc(len + 1);
	strncpy(*token, start, len);
	(*token)[len] = '\0';

	return end - exp;
}

rpn_t* rpn_init(char* exp)
{
	char* rpn = malloc((strlen(exp) * 2 + 1) * sizeof(char));
	char* token;
	char* cur = rpn;
	char** op = NULL;
	int op_size = 0;
	int ret;

	ret = get_token(exp, &token);

	while (ret > 0)
	{
		exp += ret;
		/* printf("Token: '%s'\n", token); */

		if (isdigit(*token)) /* if value */
		{
			ret = sprintf(cur, "%s ", token); /* write value */
			if (ret < 0)
			{
				fprintf(stderr, "sprintf error %d\n", ret);
				goto free_rpn;
			}

			cur += ret;
		}
		else if (strcmp(token, ")") == 0) /* operator */
		{
			while (op_size && strcmp(op[op_size - 1], "(") != 0)
			{
				--op_size;
				ret = sprintf(cur, "%s ", op[op_size]); /* write value */
				free(op[op_size]);

				if (ret < 0)
				{
					fprintf(stderr, "sprintf error %d\n", ret);
					goto free_rpn;
				}

				cur += ret;
			}

			if (op_size == 0)
			{
				fprintf(stderr, "No pair for )\n");
				goto free_rpn;
			}

			/* check discard ( */
			--op_size;
			free(op[op_size]);
		}
		else if (strcmp(token, "(") == 0)
		{
			op = realloc(op, ++op_size * sizeof(char*));
			op[op_size - 1] = malloc(strlen(token) + 1);
			strcpy(op[op_size - 1], token);
		}
		else
		{
			while (op_size && (op_get_precedence(token) < op_get_precedence(op[op_size - 1])
			|| (op_get_precedence(token) == op_get_precedence(op[op_size - 1])  && op_get_associativity(token) == OP_ASSOC_LEFT)))
			{
				--op_size;
				ret = sprintf(cur, "%s ", op[op_size]); /* write value */
				free(op[op_size]);

				if (ret < 0)
				{
					fprintf(stderr, "sprintf error %d\n", ret);
					goto free_rpn;
				}

				cur += ret;
			}

			op = realloc(op, ++op_size * sizeof(char*));
			op[op_size - 1] = malloc(strlen(token) + 1);
			strcpy(op[op_size - 1], token);
		}	

		free(token);
		ret = get_token(exp, &token);
	}

	while (op_size--)
	{
		ret = sprintf(cur, "%s ", op[op_size]); /* write value */
		free(op[op_size]);

		if (ret < 0)
		{
			fprintf(stderr, "sprintf error %d\n", ret);
			goto free_rpn;
		}

		cur += ret;
	}

	/* realloc ret - cur + 1 */
	goto done;

free_rpn:
	free(rpn);
	rpn = NULL;

done:
	free(token);

	for (int i = 0; i < op_size; ++i)
		free(op[i]);
	free(op);

	return rpn;
}

void rpn_free(rpn_t* rpn)
{
	free(rpn);
}