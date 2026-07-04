#ifndef LEX_STRING_H
#define LEX_STRING_H



/* --- lex_escape --- */

int read_escape(const char **s, char *out);

int read_escape_dq(const char **s, char *out);


/* --- variable --- */

char *read_variable(const char **s);


#endif  // LEX_STRING_H