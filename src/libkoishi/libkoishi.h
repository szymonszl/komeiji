#ifndef _LIBKOISHI_H
#define _LIBKOISHI_H
#include <stdio.h>
#include <stdint.h>

struct ksh_continuation_t {
	struct ksh_continuation_t *next;
	char character;
	uint32_t probability;
};
typedef struct ksh_continuation_t ksh_continuation_t;

struct ksh_rule_t {
	struct ksh_rule_t *next;
	char suffix[2];
	uint64_t probtotal;
	ksh_continuation_t *cont;
};
typedef struct ksh_rule_t ksh_rule_t;

struct ksh_markovdict_t {
	ksh_rule_t *hashtable[0x10000]; // 0x0 -> 0xFFFF
};
typedef struct ksh_markovdict_t ksh_markovdict_t;

ksh_markovdict_t* ksh_createdict(void);
void ksh_makeassociation(ksh_markovdict_t *dict, const char* rulename, char continuation);
void ksh_trainmarkov(ksh_markovdict_t *dict, const char* string);
char ksh_getcontinuation(ksh_markovdict_t *dict, const char* rulename);
void ksh_createstring(ksh_markovdict_t *dict, char* buf, size_t buflen);
void ksh_createprefixedstring(ksh_markovdict_t *dict, char* buf, size_t buflen, const char* initial_state, int skip);
void ksh_savedict(ksh_markovdict_t* dict, FILE* f);
ksh_markovdict_t* ksh_loaddict(FILE* f);


#endif
