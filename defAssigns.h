#ifndef LAB7
#define LAB7

#define EMPTY NULL
#define STATUS_FAILURE (-1)
#define STATUS_SUCCESS 0

#define IS_STR_EMPTY(STR) ((STR) == EMPTY | (STR)[0] == '\0')
#define IS_PTR_EMPTY(PTR) ((PTR) == EMPTY)
#define IS_STRS_EQUAL(STR1, STR2) (strcmp(STR1, STR2) == 0)

#endif 