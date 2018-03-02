#include "string_ll.h"

void string_ll_init(string_ll_t *slt) {
  slt->n=0;
  slt->head = NULL;
  slt->tail = NULL;
}

void string_ll_add(string_ll_t *slt, char *s) {
  string_ll_node_t *nod;

  nod = (string_ll_node_t *)malloc(sizeof(string_ll_node_t));
  nod->s = strdup(s);
  nod->next = NULL;

  if (!slt->head) { slt->head = nod; }
  else            { slt->tail->next = nod; }

  slt->tail = nod;
  slt->n++;
}

char *string_ll_dup_str(string_ll_t *slt) {
  char *s;
  int n_tot=0, pos=0, i;
  string_ll_node_t *nod;

  for (nod = slt->head; nod ; nod = nod->next) {
    n_tot += strlen(nod->s);
  }

  s = (char *)malloc(sizeof(char)*(n_tot+1));
  s[n_tot] = '\0';

  pos=0;
  for (nod = slt->head; nod ; nod = nod->next) {
    for (i=0; nod->s[i]; i++, pos++) {
      s[pos] = nod->s[i];
    }
  }

  return s;
}

void string_ll_free(string_ll_t *slt) {
  string_ll_node_t *nod, *tnod;

  nod = slt->head;
  while (nod) {
    tnod = nod;
    nod = nod->next;
    free(tnod->s);
    free(tnod);
  }

  slt->head = NULL;
  slt->tail = NULL;
  slt->n = 0;
}
