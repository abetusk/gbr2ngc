#ifndef STRING_LL
#define STRING_LL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct string_ll_node_type {
  char *s;
  struct string_ll_node_type *next;
} string_ll_node_t;

typedef struct string_ll_type {
  string_ll_node_t *head, *tail;
  int n;
} string_ll_t;

void string_ll_init(string_ll_t *slt);
char *string_ll_dup_str(string_ll_t *slt);
void string_ll_add(string_ll_t *slt, char *s);
void string_ll_free(string_ll_t *slt);

#ifdef __cplusplus
}
#endif


#endif
