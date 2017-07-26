#ifndef RPN_H
#define RPN_H

typedef char rpn_t;

rpn_t* rpn_init(char* exp);
void rpn_free(rpn_t* rpn);
int get_token(char* exp, char** token);

#endif