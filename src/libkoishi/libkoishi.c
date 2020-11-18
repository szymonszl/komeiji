#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "libkoishi/libkoishi.h"

void _createrule(ksh_rule_t* target, const char* rulename) {
	target->next = NULL;
	target->suffix[0] = rulename[2];
	target->suffix[1] = rulename[3];
	target->probtotal = 0;
	target->cont = NULL;
}

ksh_rule_t* ksh_findorcreaterule(ksh_markovdict_t *dict, const char *rulename, bool create) {
	//printf("Rulename: %02x %02x\n", rulename[0], rulename[1]);
	uint16_t hash = ((rulename[0]&0x7F) << 8) | (rulename[1]&0x7F);
	ksh_rule_t *prefix = dict->hashtable[hash];
	if (!prefix) {
		if (create) {
			ksh_rule_t *rule = malloc(sizeof(ksh_rule_t));
			_createrule(rule, rulename);
			dict->hashtable[hash] = rule; // add as list head
			return rule;
		} else {
			return NULL;
		}
	} else {
		while (prefix) { // seek until null
			if (memcmp(rulename+2, prefix->suffix, 2) == 0) {
				return prefix;
			}
			prefix = prefix->next;
		} // if while ends, not found
		if (create) {
			ksh_rule_t *rule = malloc(sizeof(ksh_rule_t));
			_createrule(rule, rulename);
			rule->next = dict->hashtable[hash]; // set next to current head
			dict->hashtable[hash] = rule; // set as new head
			return rule;
		} else {
			return NULL;
		}
	}
}

ksh_markovdict_t* ksh_createdict() {
	ksh_markovdict_t *dict = malloc(sizeof(ksh_markovdict_t));
	memset(dict, 0, sizeof(ksh_markovdict_t));
	return dict;
}
ksh_continuation_t* ksh_findcontinuation(ksh_rule_t *rule, char continuation) {
	ksh_continuation_t *cont = rule->cont;
	while (cont) {
		if (cont->character == continuation) {
			return cont;
		}
		cont = cont->next;
	}
	return NULL;
}
void ksh_createcontinuation(ksh_rule_t *rule, char continuation, uint16_t probability) {
	rule->probtotal += probability;
	ksh_continuation_t *new = malloc(sizeof(ksh_continuation_t));
	new->next = rule->cont; // push to front
	rule->cont = new;
	new->character = continuation;
	new->probability = probability;
}
void ksh_makeassociation(ksh_markovdict_t *dict, const char* rulename, char continuation) {
	//printf("Associating '%02x%02x%02x%02x'->'%02x'\n", rulename[0], rulename[1], rulename[2], rulename[3], continuation);
	ksh_rule_t *rule = ksh_findorcreaterule(dict, rulename, true);
	ksh_continuation_t *cont = ksh_findcontinuation(rule, continuation);
	if (cont) {
		cont->probability++;
	} else {
		ksh_createcontinuation(rule, continuation, 1);
	}
}
void ksh_trainmarkov(ksh_markovdict_t *dict, const char* string) {
	if (strlen(string) < 4) {
		//printf("Line too short!\n\"); // todo: return values ig
		return;
	}
	char rulename[4];
	for (int i = 1; i <= 4; i++) {
		memset(rulename, 0, i+1);
		memcpy(rulename+i, string, 4-i);
		ksh_makeassociation(dict, rulename, string[4-i]);
	}
	for (int i = 0; string[i+3]; i++) {
		memcpy(rulename, &string[i], 4);
		ksh_makeassociation(dict, rulename, string[i+4]);
	}

}
char ksh_getcontinuation(ksh_markovdict_t *dict, const char* rulename) {
	//printf("Finding continuation to %c%c%c%c (%02x%02x%02x%02x)\n", rulename[0], rulename[1], rulename[2], rulename[3], rulename[0], rulename[1], rulename[2], rulename[3]);
	ksh_rule_t *rule = ksh_findorcreaterule(dict, rulename, false);
	if (rule == NULL) {
		//printf("No rule!\n");
		return 0;
	}
	int random = rand() % rule->probtotal+1;
	//printf("Got random %i\n", random);
	ksh_continuation_t *cont = rule->cont;
	while (cont) {
		//printf("current continuation: %c, p%i, r%i\n", cont->character, cont->probability, random);
		random -= cont->probability;
		if (random <= 0) {
			return cont->character;
		}
		cont = cont->next;
	}
	return 0;
}
void ksh_createprefixedstring(ksh_markovdict_t *dict, char* buf, size_t buflen, const char* initial_state, int skip) {
	memcpy(buf, initial_state, 4);
	for (int i = 0; i < 4; i++) {
		buf[4+i] = ksh_getcontinuation(dict, &buf[i]);
	}
	memmove(buf, buf+skip, 8-skip); // lmfao
	for (int i = 8-skip; i < buflen-1; i++) {
		buf[i] = ksh_getcontinuation(dict, &buf[i-4]);
		if (buf[i] == 0) {
			//printf("NULL found, ending chain\n");
			return;
		}
	}
	buf[buflen] = 0;
}
void ksh_createstring(ksh_markovdict_t *dict, char* buf, size_t buflen) {
	ksh_createprefixedstring(dict, buf, buflen, "\0\0\0\0", 4);
	return;
}
/* File structure

HEADER (l=lib 5=ko 1=i 4=shi)
for each hash with a value {

	HASH (2 bytes)
	for each rule {
		SUFHEAD (+)
		SUFFIX (2 bytes)
		for each continuation {
			CONTHEAD (=)
			CHARACTER (1 byte)
			PROPABILITY (4 bytes)
		}
	}
	ENDHASH (;)
}

*/
void ksh_savedict(ksh_markovdict_t* dict, FILE* f) {
	fwrite("l\x05\x01\x04", sizeof(char), 4, f);
	printf("Written header!\n");
	for (int i = 0; i < 0x10000; i++) {
		if (dict->hashtable[i]) {
			fputc(i >> 8, f);
			fputc(i & 0xFF, f);
			for (ksh_rule_t *rule = dict->hashtable[i]; rule; rule = rule->next) {
				//printf("  Dumping rule with suffix %02x %02x\n", rule->suffix[0], rule->suffix[1]);
				fputc('+', f);
				fwrite(rule->suffix, sizeof(char), 2, f);
				for (ksh_continuation_t *cont = rule->cont; cont; cont = cont->next) {
					//printf("    Continuation %02x probability %i\n", cont->character, cont->probability);
					fputc('=', f);
					fputc(cont->character, f);
					fputc((cont->probability >> 24) & 0xFF, f);
					fputc((cont->probability >> 16) & 0xFF, f);
					fputc((cont->probability >> 8 ) & 0xFF, f);
					fputc((cont->probability      ) & 0xFF, f);
				}
			}
			fputc(';', f);
		}/* else {
			printf("Ignoring empty hash %04x\n", i);
		}*/
	}
}
ksh_markovdict_t* ksh_loaddict(FILE* f) {
	// check header
	char header[4];
	fread(header, sizeof(char), 4, f);
	if (memcmp(header, "l\x05\x01\x04", 4) != 0) {
		printf("Invalid header!");
		return NULL;
	}
	printf("Found valid header!");
	// create dict
	ksh_markovdict_t *dict = ksh_createdict();
	// load hash
	while (!feof(f)) {
		char rulename[4];
		fread(rulename, sizeof(char), 2, f);
		//printf("\033[GRead rule %02x%02x\n", rulename[0], rulename[1]);
		//uint16_t hash = (rulename[0] << 8) | rulename[1];
		while (1) {
			int sufhead = fgetc(f);
			if (sufhead == '+') { // new suffix
				fread(&rulename[2], sizeof(char), 2, f);
				ksh_rule_t *rule = ksh_findorcreaterule(dict, rulename, true);
				while (1) {
					char conthead = fgetc(f);
					if (conthead == '=') {
						char character = fgetc(f);
						char rawprop[4];
						fread(rawprop, sizeof(char), 4, f);
						uint32_t probability =
							(rawprop[0] << 24) |
							(rawprop[1] << 16) |
							(rawprop[2] << 8) |
							(rawprop[3]);
						ksh_createcontinuation(rule, character, probability);
					} else if (conthead == '+') { // new suffix started
						ungetc('+', f);
						break;
					} else if (conthead == ';') { // new hash started
						ungetc(';', f);
						break;
					} else {
						printf("Invalid character - expected '+=;' found %02x", conthead);
						return NULL;
						// FIXME: deallocate dict
					}
				}
			} else if (sufhead == ';') { // new hash
				break;
			} else if (sufhead == EOF) {
				printf("File ended!");
				// return filled dict
				return dict;
			} else {
				printf("Invalid character - expected '+;' found %02x", sufhead);
				return NULL;
				// FIXME: deallocate dict
			}
		}
	}
}
