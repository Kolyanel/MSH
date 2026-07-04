#ifndef PARSER_RULES_H
#define PARSER_RULES_H

#include "parser.h"
#include "ast.h"

t_parse_res parse_simple_command(t_parser *p, t_cmd **out);


t_parse_res parse_pipeline(t_parser *p, t_pipeline **out);


t_parse_res parse_and_or(t_parser *p, t_and_or **out);


t_parse_res parse_line(t_parser *p);


#endif  // PARSER_RULES_H