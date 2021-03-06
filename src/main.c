#define _GNU_SOURCE // needed for CLOCK_MONOTONIC

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>

#include <libkoishi.h>
#include "sock/wsock.h"
#include "utils/buffer.h"
#include "utils/string.h"
#include "utils/cmddisp.h"

volatile sig_atomic_t running = 1;
void sigint_handler(int sig) {
    if (sig == SIGINT)
        running = 0;
}
static struct {
    char *server;
    int uid;
    int ownerid;
    char *auth;
    char *markovpath;
    char prefix;
} config = {0};

ksh_model_t *markov;
wsock_t *conn;
buffer_t *ib, *ob;
char helptext[1024];

void sendchat(const char* msg) {
    char buf[64];
    snprintf(buf, 64, "2\t%d\t", config.uid);
    buffer_write_str(ob, buf);
    if (msg[0] == '/') {
        buffer_write_str(ob, "\u200b"); // U+200B ZERO WIDTH SPACE
    }
    buffer_write_str(ob, msg);
    wsock_send(conn, ob);
    buffer_truncate(ob);
}
void sendchatf(const char* msg, ...) {
    va_list args;
    va_start(args, msg);
    char buf[BUFSIZ];

    vsnprintf(buf, BUFSIZ, msg, args);
    sendchat(buf);

    va_end(args);
}
void trainmarkov(const char* msg) {
    char buf[5120]; // max flashii message length is 5000, this should be enough
    int cur = 0;
    if (strstr(msg, "[code]")) return;
    if (strstr(msg, "[sjis]")) return;
    if (str_prefix(msg, "!markov")) return;
    if (str_prefix(msg, "!np")) return;
    for (int i = 0; msg[i] != 0; i++) {
        if (str_prefix(&msg[i], " <br/> ")) {
            buf[cur++] = '\n';
            i += 6;
        } else if (str_prefix(&msg[i], "&lt;")) {
            buf[cur++] = '<';
            i += 3;
        } else if (str_prefix(&msg[i], "&gt;")) {
            buf[cur++] = '>';
            i += 3;
        } else if (msg[i] == '[') { // bbcode
            int ni = i; // new index
            int max = i + 8; // max length of bbcode
            if (str_prefix(&msg[i+1], "color=")) {
                max = i + 30; // cute exception fuck you
            }
            if (str_prefix(&msg[i+1], "url=")) {
                max = i + 250; // how fucking long can urls be christ
            }
            while ((msg[ni] != 0) && (ni < max)) {
                if (msg[ni] == ']') {
                    i = ni; // apply new index if bbcode ended
                    break;
                }
                ni++;
            }
        } else {
            buf[cur++] = msg[i];
        }
    }
    buf[cur] = 0;
    ksh_trainmarkov(markov, buf);
}
/////////////////
/// COMMANDS
/////////////////

void cmd_markov_h(const char* args) {
    char sentence[1024];
    ksh_createstring(markov, sentence, 1024);
    sendchat(sentence);
}
command_definition cmd_markov = {
    .keywords = "markov voodoo",
    .description = "send a randomly generated message",
    .handler = cmd_markov_h,
    .admin_only = 0,
    .has_arguments = 0
};

void cmd_continue_h(const char* args) {
    char sentence[512];
    if (args) {
        int len = strlen(args);
        char *ultrameme = strstr(args, ":ultreme:");
        if (ultrameme) {
            sendchat(":mewow:");
            return;
        }
        for (int i = 0; i < 5; i++) { // up to 5 rolls
            ksh_continuestring(markov, sentence, 512, args);
            // printf("Roll %i: '%s'[%d] -> '%s'[%d]\n", i, args, len, sentence, strlen(sentence));
            if (strlen(sentence) != len)
                break;
        }
        sendchat(sentence);
    } else {
        sendchat("[i]Please send a message to finish![/i]");
    }
}
command_definition cmd_continue = {
    .keywords = "continue c",
    .description = "continue the argument using a Markov chain",
    .handler = cmd_continue_h,
    .admin_only = 0,
    .has_arguments = 1
};

void cmd_greentext_h(const char* args) {
    char sentence[256];
    ksh_continuestring(markov, sentence, 256, ">");
    for (int i = 0; i < 256; i++) { // end the sentence at the first newline
        if (sentence[i] == '\n') {
            sentence[i] = 0;
            break;
        }
    }
    sendchatf("[color=#789922]%s[/color]", sentence);
}
command_definition cmd_greentext = {
    .keywords = "&gt;",
    .description = "generate a greentext message",
    .handler = cmd_greentext_h,
    .admin_only = 0,
    .has_arguments = 0
};

const char *parseequotes[] = {
    "Oh, are you a human?",
    "You know about me? Who are you?",
    "What does a human want in the Former Capital?",
    "I don't have anything against you, but I can make up lots of reasons to attack you.",
    "Hmph, are you looking down on me?"
};

void cmd_help_h(const char* args) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    sendchatf(helptext, parseequotes[ts.tv_nsec%5]); // wdym
}
command_definition cmd_help = {
    .keywords = "help ?",
    .description = "post this",
    .handler = cmd_help_h,
    .admin_only = 0,
    .has_arguments = 0
};

void cmd_save_h(const char* args) {
    sendchat("Saving...");
    FILE* f = fopen(config.markovpath, "w");
    ksh_savemodel(markov, f);
    fclose(f);
    sendchat("Saved the markov database.");
}
command_definition cmd_save = {
    .keywords = "save",
    .description = NULL,
    .handler = cmd_save_h,
    .admin_only = 1,
    .has_arguments = 0
};

void cmd_exit_h(const char* args) {
    sendchat("Exiting...");
    running = 0;
}
command_definition cmd_exit = {
    .keywords = "exit",
    .description = NULL,
    .handler = cmd_exit_h,
    .admin_only = 1,
    .has_arguments = 0
};


command_definition *commands[] = {
    &cmd_markov, &cmd_continue, &cmd_greentext, &cmd_help,
    &cmd_save, &cmd_exit, 0
};

/// END COMMANDS
/////////////////


void dispatchcommand(char* msg, int author) {
    static struct timespec lastts = {0,0}, ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    if (author != config.ownerid) {
        if ((ts.tv_sec-lastts.tv_sec) < 2) {
            unsigned long diff =
                (ts.tv_sec-lastts.tv_sec) * 1000000000L +
                (ts.tv_nsec-lastts.tv_nsec);
            if (diff < 1000000000L) {
                // if not at least a second elapsed, then cancel
                printf("Command ignored (too soon, diff %ld)\n", diff);
                return;
            }
        }
    }
    char *part = strtok(msg, " ");
    command_definition *cmd = NULL;
    if (part != NULL) {
        cmd = resolve_command(commands, part);
    }
    if (cmd == NULL)
        return;
    if (cmd->admin_only && (author != config.ownerid))
        return;
    if (cmd->has_arguments) {
        part = strtok(NULL, "");
    } else {
        part = NULL;
    }
    cmd->handler(part);
    lastts = ts;
}

int main(int argc, char** argv) {

    // cute banner
    puts("===      PARSEE       ===\n"
         "==   based on KOMEIJI  ==\n"
         "=  because markov hard  =\n"
         "====                 ====\n");

    signal(SIGINT, sigint_handler);

    // config loading
    FILE* f = fopen("kmj.cfg", "r");
    char key[64];
    char val[256];
    while (fscanf(f, "%64[^ \n] %256[^\n]%*c", key, val) == 2) {
        if (key[0] == '#') continue; // ignore comments
        else if (0 == strcmp(key, "server")) config.server = strdup(val);
        else if (0 == strcmp(key, "uid")) config.uid = strtol(val, NULL, 10);
        else if (0 == strcmp(key, "ownerid")) config.ownerid = strtol(val, NULL, 10);
        else if (0 == strcmp(key, "auth")) config.auth = strdup(val);
        else if (0 == strcmp(key, "markovpath")) config.markovpath = strdup(val);
        else if (0 == strcmp(key, "prefix")) config.prefix = val[0];
        else fprintf(stderr, "Warning: unrecognized key \"%s\" in config\n", key);
    }
    fclose(f);

    generate_help(helptext, 1024, commands, "[color=#ebc18f][b]Parsee[/b] - %s[/color]", config.prefix);

    // markov loading
    markov = ksh_createmodel(20, NULL, time(NULL));
    if (!markov) {
        fprintf(stderr, "[!] Couldn't allocate markov model, exiting!");
        return 1;
    }
    f = fopen(config.markovpath, "r");
    if (f) {
        if (ksh_loadmodel(markov, f) < 0) {
            fprintf(stderr, "[!] Invalid markov file, exiting!");
            return 1;
        }
        fclose(f);
    } else {
        perror("[!] Warning: Markov file could not be opened");
        fprintf(stderr, "[!] Starting with empty model\n");
    }

    printf("[+] Connecting to \"%s\"...\n", config.server);
    conn = wsock_open(config.server);
    if (!conn) {
        printf("wsock_open() returned NULL, exiting!\n");
        return 1;
    }
    printf("[+] Connected!\n");
    ib = buffer_create();
    ob = buffer_create();

    char buf[256];
    sprintf(buf, "1\t%d\t%s", config.uid, config.auth);
    buffer_write_str(ob, buf);
    wsock_send(conn, ob);
    buffer_truncate(ob);
    printf("[+] Login packet sent!\n");
    struct timespec lastsave, lastping, ts;
    while (running) {
        int status = wsock_recv(conn, ib, 0);
        if (status > 0) {
            char *recvd = buffer_read_str(ib);
            //printf("Received chars: \"%s\"\n", recvd);
            buffer_truncate(ib);
            // parse the received packet
            char *part = strtok(recvd, "\t");
            if (!part) {
                printf("[!] Warning: empty string received\n");
            } else if (0 == strcmp(part, "1")) { // is a login confirmation
                part = strtok(NULL, "\t"); // y/n
                if (part[0] == 'y') {
                    printf("[+] Joined chat!\n");
                } else if (part[0] == 'n') {
                    part = strtok(NULL, "\t"); // failure reason
                    printf("[!] Could not connect to chat. Reason: '%s'.\n", part);
                    break;
                } // else it's a join
            } else if (0 == strcmp(part, "2")) { // is a message
                int author = 0;
                for (int i = 1; (part = strtok(NULL, "\t")) != NULL; i++) {
                    // ^ for every tab-separated token, with the token index as i
                    if (i == 2) { // message author
                        author = strtol(part, NULL, 10);
                    }
                    if (i == 3) { // message text
                        printf("[ ] %4d: \'%s\'\n", author, part);
                        str_lower(part);
                        if (author != config.uid) {
                            if (part[0] == config.prefix) {
                                dispatchcommand(part+1, author);
                            } else {
                                trainmarkov(part);
                            }
                        }
                        break;
                    }
                }
            }
            free(recvd);
        } else if (status < 0) {
            printf("[!] An error occured during recv (-1), connection closed.\n");
            conn = NULL;
            break;
        }
        clock_gettime(CLOCK_MONOTONIC, &ts);
        if ((ts.tv_sec-lastping.tv_sec) > 10) {
            sprintf(buf, "0\t%d", config.uid);
            buffer_write_str(ob, buf);
            wsock_send(conn, ob);
            buffer_truncate(ob);
            lastping = ts;
        }
        if ((ts.tv_sec-lastsave.tv_sec) > 10*60) {
            f = fopen(config.markovpath, "w");
            ksh_savemodel(markov, f);
            fclose(f);
            printf("[+] Saved markov.\n");
            lastsave = ts;
        }
        usleep(10000);
    }
    printf("[!] Loop exited!\n");
    f = fopen(config.markovpath, "w");
    ksh_savemodel(markov, f);
    fclose(f);
    printf("[+] Markov saved. Trying to close the socket...\n");
    if (conn) {
        wsock_close(conn);
    }
    buffer_free(ib);
    buffer_free(ob);
    return 0;
}
