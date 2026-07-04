#ifndef ENV_UTILS_H
#define ENV_UTILS_H



enum{
	ASSIGN,
	APPEND
};


// Разделяет строку вида "KEY=VALUE" на key и val
// - если '=' нет, val = NULL
// - выделяет память для key и val через strdup/strndup
// - возвращает 0 при успехе, -1 при ошибке (например, ошибка аллокации)

int parse_assignment(const char *arg, char **key, char **val, int *mode);



// Проверяет, что строка s является допустимым идентификатором переменной среды
// - начинается с буквы или '_'
// - далее могут быть буквы, цифры или '_'
// Возвращает 1 если корректно, 0 если нет

int is_valid_identifier(const char *s);


#endif  //  ENV_UTILS_H