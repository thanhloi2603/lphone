#include "lutils.h"
#define THIS_FILE "lutils.c"

void show_help() {
    printf("hangup/hup             -- hangup all calls\n");
    printf("quit                   -- quit program\n");
    printf("help                   -- show this help\n");
    printf("register               -- register against the registrar server\n");
    printf("unregister             -- flush the registration\n");
    printf("call <sip_uri>         -- call a sip uri\n");
    printf("hold                   -- hold the call\n");
    printf("unhold                 -- unhold the call\n");
    printf("dtmf digits            -- send dtmf digits, only avaialbe when call is active\n");
    printf("transfer <sip_uri>     -- transfer this call to a target uri\n");
    printf("codecs                 -- list all supported codecs\n");
    fflush(stdout);
}

int init_args(larg_t *arg) {

    cfg_opt_t opts[] =
    {
        CFG_STR("username", "_undef_", CFGF_NONE),
        CFG_STR("password", "_undef_", CFGF_NONE),
        CFG_STR("proxy", "_undef_", CFGF_NONE),
        CFG_STR("server", "_undef_", CFGF_NONE),
        CFG_STR("user_agent_string", "Loi-SIP", CFGF_NONE),
        CFG_STR("outputFile", "_undef_", CFGF_NONE),
        CFG_STR("ringFile", "_undef_", CFGF_NONE),
        CFG_INT("port", 5060, CFGF_NONE),
        CFG_STR("transport", "UDP", CFGF_NONE),
        CFG_STR_LIST("codecs", "{}", CFGF_NONE),
        CFG_END()
    };
    cfg_t *cfg;
    cfg = cfg_init(opts, CFGF_NONE);
    if(cfg_parse(cfg, "lphone.conf") == CFG_PARSE_ERROR)
        return -1;

    /* default value */
    strcpy(arg->username, cfg_getstr(cfg, "username"));
    strcpy(arg->proxy, cfg_getstr(cfg, "proxy"));
    strcpy(arg->password, cfg_getstr(cfg, "password"));
    strcpy(arg->server, cfg_getstr(cfg, "server"));
    strcpy(arg->outputFile, cfg_getstr(cfg, "outputFile"));
    strcpy(arg->ringFile, cfg_getstr(cfg, "ringFile"));
    arg->port = cfg_getint(cfg, "port");
    strcpy(arg->user_agent_string, cfg_getstr(cfg, "user_agent_string"));
    strcpy(arg->transport, cfg_getstr(cfg, "transport"));

    int codec_config_size = cfg_size(cfg, "codecs");
    if (codec_config_size > MAX_ALLOWED_CODEC)
        codec_config_size = MAX_ALLOWED_CODEC;

    for (int i = 0; i < codec_config_size; i++)
        strcpy(arg->codecs[i], cfg_getnstr(cfg, "codecs", i));

    cfg_free(cfg);
    return 0;
}
