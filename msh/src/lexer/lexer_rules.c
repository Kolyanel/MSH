#include "lexer_rules.h"


t_lex_rule g_rules[] = {
	lex_logic,
	lex_pipe,
	lex_background,
	lex_redir,
	lex_semi,
	lex_word
};

const size_t g_rules_cnt = sizeof(g_rules) / sizeof(*g_rules);