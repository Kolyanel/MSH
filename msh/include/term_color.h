#ifndef TERM_COLOR_H
#define TERM_COLOR_H


#define C_RESET "\033[0m"  // сброс

#define C_BOLD "\033[1m"  // жирный текст, повышенная яркость


#define C_RED "\033[31m"  // красный

#define C_GREEN "\033[32m"  // зеленый

#define C_YELLOW "\033[33m"  // желтый

#define C_BLUE "\033[34m"  // синий


#define C_CYAN "\033[36m"  // голубой / бирюзовый

#define C_GRAY "\033[90m"  // серый / тусклый


#define C_ERR    C_RED

#define C_OK       C_GREEN

#define C_HINT   C_GRAY


#endif  // TERM_COLOR_H