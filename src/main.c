#define _GNU_SOURCE // needed for CLOCK_MONOTONIC

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>

#include <libkoishi.h>
#include "sock/wsock.h"
#include "ts3/ts3.h"
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
    char *javpath;
    char prefix;
    char *tshost;
    char *tsname;
    char *tspass;
} config = {0};

ksh_model_t *markov;
ksh_model_t *jav;
wsock_t *conn;
buffer_t *ib, *ob;
char helptext[1024];
ts3_t *ts3;
struct tsn_cache *tsn;
struct user {
    struct user *next;
    int id;
    char *name;
};
struct user *users = NULL;

struct user *
user_get(int id) {
    for (struct user *u = users; u != NULL; u = u->next) {
        if (u->id == id)
            return u;
    }
    return NULL;
}

const char *
user_name(int id) {
    struct user *u;
    if ((u = user_get(id))) { // assignment intened
        return u->name;
    }
    static char oops[10];
    printf("[!] no name known for %d\n", id);
    snprintf(oops, 10, "[%d]", id);
    return oops;
}

void
user_add_or_update(int id, const char* name) {
    struct user *u = user_get(id);
    if (!u) {
        printf("[+] user add %d as '%s'\n", id, name);
        u = malloc(sizeof(struct user));
        u->next = users;
        users = u;
        u->id = id;
        u->name = strdup(name);
        return;
    } else {
        if (0 != strcmp(u->name, name)) {
            printf("[+] user update %d '%s' -> '%s'\n", id, u->name, name);
            free(u->name);
            u->name = strdup(name);
        }
    }
}

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
    char buf[20008]; // max flashii message length is 5000 characters, assuming worst case four bytes per char (utf-8) + eight more bytes because trust issues
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

double
timestamp(clockid_t clock)
{
    struct timespec ts;
    clock_gettime(clock, &ts); 
    double t = ts.tv_sec;
    t += ((double)ts.tv_nsec) / 1000000000.0;
    return t;
}
double ts3_ts(void) { return timestamp(CLOCK_MONOTONIC); }

/////////////////
/// COMMANDS
/////////////////

void cmd_markov_h(int author, const char* args) {
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

void cmd_continue_h(int author, const char* args) {
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

void cmd_greentext_h(int author, const char* args) {
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

#define BEGIN_LEN 11
const static char *beginnings[BEGIN_LEN] = {
    "tomorrow you will ",
    "tomorrow fate ",
    "# fate:",
    "# will ",
    "# soon will ",
    "soon you will ",
    "# fortune:",
    "fortune for #:",
    "tomorrow fortune:",
    "i am certain # ",
    "In @ $ Will ",
};
void cmd_fortune_h(int author, const char* args) {
    char sentence[1024];
    char beginning[1024];
    memset(beginning, 0, sizeof(beginning));
    int n = rand() % BEGIN_LEN;
    int c = 0;
    int caps = 0;
    for (int i = 0; beginnings[n][i]; i++) {
        if (beginnings[n][i] == '#') {
            strcat(beginning, user_name(author));
            c += strlen(user_name(author));
        } else if (beginnings[n][i] == '$') {
            char *name = strdup(user_name(author));
            *name = (char)toupper(*name);
            str_lower(name+1);
            strcat(beginning, name);
            c += strlen(name);
            free(name);
            caps = 1;
        } else if (beginnings[n][i] == '@') {
            char buf[6];
            time_t t = time(NULL);
            struct tm *ts = gmtime(&t);
            if ((rand() & 0x30) == 0x10) { // 1 in 4 chance
                ts->tm_year += rand() % 5;
            }
            strftime(buf, 5, "%Y", ts);
            strcat(beginning, buf);
            c += strlen(buf);
        } else {
            beginning[c++] = beginnings[n][i];
        }
    }
    char *lbegin = strdup(beginning); // use a lowercase copy to generate
    str_lower(lbegin);
    for (int i = 0; i < 5; i++) { // up to 5 rolls
        ksh_continuestring((rand()%1234)?markov:jav, sentence, 1024, lbegin);
        if (strlen(sentence) != strlen(beginning))
            break;
    }
    free(lbegin);
    memcpy(sentence, beginning, strlen(beginning)); // then copy the Cased version back over
    if (caps) {
        char *cur = sentence;
        while ((cur = strchr(cur, ' '))) {
            cur++;
            *cur = toupper(*cur);
        }
    }
    sendchatf("%s", sentence);
}
command_definition cmd_fortune = {
    .keywords = "fortune fate",
    .description = "generate a random fortune",
    .handler = cmd_fortune_h,
    .admin_only = 0,
    .has_arguments = 0
};

void cmd_jav_h(int author, const char* args) {
    if (jav) {
        char title[1024];
        ksh_createstring(jav, title, 1024);
        sendchat(title);
    } else {
        sendchat("[i]error: no JAV title model[/i]");
    }
}
command_definition cmd_jav = {
    .keywords = "jav",
    .description = "create a Japanese Adult Video title",
    .handler = cmd_jav_h,
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

void cmd_help_h(int author, const char* args) {
    sendchatf(helptext, parseequotes[rand()%5]);
}
command_definition cmd_help = {
    .keywords = "help ?",
    .description = "post this",
    .handler = cmd_help_h,
    .admin_only = 0,
    .has_arguments = 0
};

void cmd_ts_h(int author, const char* args) {
    static buffer_t *buf = NULL;
    if (!buf) buf = buffer_create();
    if (!ts3) {
        sendchat("[i]ts3 functionality disabled[/i]");
        return;
    }
    ts3_resp *cl = ts3_query(ts3, "clientlist");
    if (!cl) {
        sendchat("[i]Error: couldn't connect to ts[/i]");
        return;
    }
    if (!ts3_issuccess(cl)) {
        sendchatf("[i]Error: request failed - %s (%d)[/i]", cl->desc, cl->errid);
        return;
    }
    int total = 0;
    for (ts3_record *user = cl->records; user; user = user->next) {
        const char *iscl = ts3_getval(user, "client_type");
        if (iscl[0] == '0') {
            char *name = ts3_getval(user, "client_nickname");
            char *meme;
            while ((meme = strstr(name, ":ultreme:"))) {
                memcpy(meme, ":extreme:", 10);
            }
            char tmp[64];
            snprintf(tmp, 64, "%s[b]%s[/b]", total?", ":"", name);
            buffer_write_str(buf, tmp);
            total++;
        }
    }
    ts3_freeresp(cl);
    char *out = buffer_read_str(buf);
    sendchatf("%d users online: %s", total, out);
    free(out);
    buffer_truncate(buf);
}
command_definition cmd_ts = {
    .keywords = "ts ts3 teamspeak",
    .description = "show list of online users",
    .handler = cmd_ts_h,
    .admin_only = 0,
    .has_arguments = 0
};

void cmd_save_h(int author, const char* args) {
    if (!args || !args[0]) {
        args = config.markovpath;
    }
    sendchatf("Saving to [code]%s[/code]...", args);
    FILE* f = fopen(args, "w");
    ksh_savemodel(markov, f);
    fclose(f);
    sendchat("Saved the markov database.");
}
command_definition cmd_save = {
    .keywords = "save",
    .description = NULL,
    .handler = cmd_save_h,
    .admin_only = 1,
    .has_arguments = 1
};

void cmd_exit_h(int author, const char* args) {
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
    &cmd_markov, &cmd_continue, &cmd_greentext, &cmd_fortune,
    &cmd_jav, &cmd_ts, &cmd_help,
    &cmd_save, &cmd_exit, 0
};

/// END COMMANDS
/////////////////


void dispatchcommand(char* msg, int author) {
    static double lastts = 0, lastfunny = 0, ts;
    static float funny_score = 0;
    ts = timestamp(CLOCK_MONOTONIC);
    if (author != config.ownerid) {
        if (ts-lastts < 1.0) {
            // if not at least a second elapsed, then cancel
            printf("Command ignored (too soon, diff %fs)\n", ts-lastts);
            return;
        }
    }
    char *part = strtok(msg, " ");
    command_definition *cmd = NULL;
    int diff = INT_MAX;
    if (part != NULL) {
        cmd = resolve_command(commands, part, &diff);
    }
    if (cmd == NULL) {
        float pfs = funny_score;
        float dt = ts - lastfunny;
        if (dt < 0.7) dt = 0.7;
        funny_score /= dt;
        lastfunny = ts;
        funny_score += 2/diff;
        printf("[f] fs%f <- d%d, dt%f, pfs%f\n", funny_score, diff, dt, pfs);
        if (funny_score > 5 && (rand()%5) == 0) {
            sendchat("Take it easy...");
            lastts = ts;
            funny_score = 0;
        }
        return;
    }
    if (cmd->admin_only && (author != config.ownerid))
        return;
    if (cmd->has_arguments) {
        part = strtok(NULL, "");
    } else {
        part = NULL;
    }
    cmd->handler(author, part);
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
        else if (0 == strcmp(key, "javpath")) config.javpath = strdup(val);
        else if (0 == strcmp(key, "prefix")) config.prefix = val[0];
        else if (0 == strcmp(key, "tshost")) config.tshost = strdup(val);
        else if (0 == strcmp(key, "tsname")) config.tsname = strdup(val);
        else if (0 == strcmp(key, "tspass")) config.tspass = strdup(val);
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

    jav = ksh_createmodel(10, NULL, time(NULL));
    if (!markov) {
        fprintf(stderr, "[!] Couldn't allocate JAV markov model, exiting!");
        return 1;
    }
    FILE *f2 = fopen(config.javpath, "r");
    if (f2) {
        if (ksh_loadmodel(jav, f2) < 0) {
            fprintf(stderr, "[!] Invalid markov file, exiting!");
            return 1;
        }
        fclose(f);
    } else {
        perror("[!] Warning: JAV markov file could not be opened");
        fprintf(stderr, "[!] Starting with disabled JAV functionality");
        ksh_freemodel(jav);
        jav = NULL;
    }

    if (config.tshost && config.tsname && config.tspass) {
        ts3 = ts3_open(config.tshost, config.tsname, config.tspass);
        tsn = calloc(1, sizeof(struct tsn_cache));
    } else {
        fprintf(stderr, "[!] Disabling TS3 due to missing configs\n");
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
    double lastsave = 0, lastping = 0;
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
                part = strtok(NULL, "\t"); // y/n/timestamp
                if (part[0] == 'y') {
                    part = strtok(NULL, "\t"); // session uid
                    part = strtok(NULL, "\t"); // username
                    user_add_or_update(config.uid, part);
                    printf("[+] Joined chat!\n");
                } else if (part[0] == 'n') {
                    part = strtok(NULL, "\t"); // failure reason
                    printf("[!] Could not connect to chat. Reason: '%s'.\n", part);
                    break;
                } else { // someone joined
                    part = strtok(NULL, "\t"); // uid
                    int id = strtol(part, NULL, 10);
                    part = strtok(NULL, "\t"); // username
                    user_add_or_update(id, part);
                }
            } else if (0 == strcmp(part, "2")) { // is a message
                int author = 0;
                for (int i = 1; (part = strtok(NULL, "\t")) != NULL; i++) {
                    // ^ for every tab-separated token, with the token index as i
                    if (i == 2) { // message author
                        author = strtol(part, NULL, 10);
                    }
                    if (i == 3) { // message text
                        printf("[ ] %s: \'%s\'\n", user_name(author), part);
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
            } else if (0 == strcmp(part, "7")) { // contexts
                part = strtok(NULL, "\t"); // subtype
                if (0 == strcmp(part, "0")) { // user
                    part = strtok(NULL, "\t");
                    int usercount = strtol(part, NULL, 10);
                    for (int i = 0; i < usercount; i++) {
                        part = strtok(NULL, "\t"); // user id
                        int id = strtol(part, NULL, 10);
                        part = strtok(NULL, "\t"); // username
                        user_add_or_update(id, part);
                        part = strtok(NULL, "\t"); // css color
                        part = strtok(NULL, "\t"); // perms
                        part = strtok(NULL, "\t"); // visible
                    }
                }
            } else if (0 == strcmp(part, "10")) { // user update
                part = strtok(NULL, "\t"); // uid
                int id = strtol(part, NULL, 10);
                part = strtok(NULL, "\t"); // username
                user_add_or_update(id, part);
            }
            free(recvd);
        } else if (status < 0) {
            printf("[!] An error occured during recv (-1), connection closed.\n");
            conn = NULL;
            break;
        }
        double ts = timestamp(CLOCK_MONOTONIC);
        if (ts - lastping > 30) {
            sprintf(buf, "0\t%d", config.uid);
            buffer_write_str(ob, buf);
            wsock_send(conn, ob);
            buffer_truncate(ob);
            lastping = ts;
        }
        if (ts - lastsave > 10*60) {
            f = fopen(config.markovpath, "w");
            ksh_savemodel(markov, f);
            fclose(f);
            printf("[+] Saved markov.\n");
            lastsave = ts;
        }
        if (ts3) {
            ts3_resp *notif = ts3_idlepoll(ts3);
            if (notif) {
                if (0 == strcmp(notif->desc, "cliententerview")) {
                    const char *iscl = ts3_getval(notif->records, "client_type");
                    if (iscl[0] == '0') {
                        char *nick = ts3_getval(notif->records, "client_nickname");
                        char *meme;
                        while ((meme = strstr(nick, ":ultreme:"))) {
                            memcpy(meme, ":extreme:", 10);
                        }
                        char *id = ts3_getval(notif->records, "clid");
                        tsn_push(tsn, id, nick);
                        sendchatf("[b]%s[/b] joined TeamSpeak", nick);
                    }
                } else if (0 == strcmp(notif->desc, "clientleftview")) {
                    char *id = ts3_getval(notif->records, "clid");
                    char *nick = tsn_pull(tsn, id);
                    if (nick) {
                        sendchatf("[b]%s[/b] left TeamSpeak", nick);
                        free(nick);
                    }
                }
                ts3_freeresp(notif);
            }
        }
        usleep(10000);
    }
    printf("[!] Loop exited!\n");
    f = fopen(config.markovpath, "w");
    ksh_savemodel(markov, f);
    fclose(f);
    if (ts3) {
        ts3_close(ts3);
    }
    printf("[+] Markov saved. Trying to close the socket...\n");
    if (conn) {
        wsock_free(conn);
    }
    buffer_free(ib);
    buffer_free(ob);
    return 0;
}
