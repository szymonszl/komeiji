#include "cmddisp.h"
#include <levenshtein.h>

command_definition *
resolve_command(command_definition **commands, const char *keyword, int *dist)
{
    char buf[128];
    char *r;
    for (int i = 0; commands[i] != NULL; i++) {
        command_definition *cmd = commands[i];
        strcpy(buf, cmd->keywords);
        char *c = strtok_r(buf, " ", &r);
        do {
            size_t diff = levenshtein(keyword, c);
            if (diff == 0)
                return cmd;
            if (dist && *dist > diff) {
                *dist = diff;
            }
        } while (c = strtok_r(NULL, " ", &r));
    }
    return NULL;
}

int
generate_help(
    char *out,
    size_t len,
    command_definition **commands,
    const char *header,
    char prefix
)
{
    int i = 0;
    i += snprintf(out, len, "%s\n", header);
    for (int j = 0; commands[j] != NULL; j++) {
        command_definition *cmd = commands[j];
        char buf[128];
        char *c;
        if (cmd->admin_only)
            continue;

        strcpy(buf, cmd->keywords);
        c = strtok(buf, " ");
        while (1) {
            i += snprintf(&out[i], len-i, "%c%s", prefix, c);
            if (c = strtok(NULL, " "))
                i += snprintf(&out[i], len-i, ", ");
            else
                break;
        }
        if (cmd->has_arguments) {
            i += snprintf(&out[i], len-i, " [i]...[/i]");
        }
        i += snprintf(&out[i], len-i, " - %s\n", cmd->description);
    }
    return i;
}
