#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <confuse.h>
struct larg_s
{
    /* Account info */
    char username[30];                      // authusername as well
    char password[30];                      // account password
    char server[100];                       // registrar to register against, no REGISTER if this is not set
    char proxy[100];                        // where to send calls to by default
    char user_agent_string[100];            // user agent header
    int port;                               // port to bind to, default is 5060, set to 0 to bind to any available port
    char outputFile[100];                   // file to play instead of microphone
    char ringFile[100];                     // ring file to use
    char transport[10];                     // transport method to use, accept 'udp' or 'tcp', case insensitive
    char codecs[10][20];
};

typedef struct larg_s larg_t;
/* what to input into stdin when program is running */
void show_help();

int init_args(larg_t *arg);
