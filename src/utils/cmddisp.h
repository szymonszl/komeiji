#ifndef KMJ_CMDDISP_H
#define KMJ_CMDDISP_H

#include <stdio.h>
#include <string.h>

typedef void (*handler_t)(int, const char*);

typedef struct {
    char *keywords;
    char *description;
    handler_t handler;
    int admin_only;
    int has_arguments;
} command_definition;

command_definition *resolve_command(command_definition **commands, const char *keyword);
int generate_help(char *out, size_t len, command_definition **commands, const char *header, char prefix);

#endif