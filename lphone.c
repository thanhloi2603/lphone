#include <pjsua-lib/pjsua.h>
#include "call_func.h"
#include "lutils.h"

#define THIS_FILE "lphone.c"

/* only one account during runtime, process 1 call at a time as well */
pjsua_call_id call_id;
pjsua_acc_id acc_id;
pjsua_acc_config acc_cfg;
laudio_config_t laudio_config;

int main(int argc, char *argv[])
{
    /* parse program's parameter */
    larg_t *mParams;
    mParams = malloc(sizeof(larg_t));
    if (init_args(mParams) == -1) {
        /* Failed to parse parameter, end here */
        printf("Error parsing config file, please correct it first\n");
        abort();
    }

    if (!strcmp(mParams->username, "_undef_") 
        || !strcmp(mParams->password, "_undef_") 
        || !strcmp(mParams->proxy, "_undef_")) {
        printf("Param is missing, please check!\n");
        abort();
    }

    /* create pjsua first */
    pj_status_t status;
    status = pjsua_create();
    if (status != PJ_SUCCESS)
        error_exit("Unable to create pjsua!\n", status);

    /* user agent configuration */
    pjsua_config ua_cfg;
    pjsua_config_default(&ua_cfg);
    ua_cfg.max_calls = 1;
    ua_cfg.use_srtp = PJMEDIA_SRTP_DISABLED;
    ua_cfg.use_timer = PJSUA_SIP_TIMER_INACTIVE;
    ua_cfg.user_agent = pj_str(mParams->user_agent_string);
    ua_cfg.stun_srv[0] = pj_str("stun.hoiio.com");
    
    /* event to handle */
    ua_cfg.cb.on_call_media_state = &on_call_media_state;
    ua_cfg.cb.on_incoming_call = &on_incoming_call;
    ua_cfg.cb.on_call_state = &on_call_state;

    /* logging configuration */
    pjsua_logging_config log_cfg;
    pjsua_logging_config_default(&log_cfg);
    log_cfg.msg_logging = PJ_TRUE;
    log_cfg.console_level = 4;  

    status = pjsua_init(&ua_cfg, &log_cfg, NULL);
    if (status != PJ_SUCCESS)
        error_exit("Unable to init pjsua!\n", status);
    
    /* binding an transport protocol*/

    pjsua_transport_config transport_cfg;
    pjsua_transport_id transport_id;
    pjsua_transport_config_default(&transport_cfg);
    transport_cfg.port = mParams->port;

    /* transport type to use */
    enum pjsip_transport_type_e transport = PJSIP_TRANSPORT_UDP;
    if (!strcasecmp(mParams->transport, "tcp"))
        transport = PJSIP_TRANSPORT_TCP;
    status = pjsua_transport_create(transport, &transport_cfg, &transport_id);

    if (status != PJ_SUCCESS)
        error_exit("Unable to create transport!\n", status);

    /* start pjsua */
    status = pjsua_start();
    if (status != PJ_SUCCESS)
        error_exit("Unable to start pjsua!\n", status);

    {
        pjsua_acc_config_default(&acc_cfg);

        char acct_uri[MAX_URI_SIZE], registrar[MAX_URI_SIZE];
        sprintf(acct_uri, "sip:%s@%s", mParams->username, mParams->proxy);
        /* uri of the account */
        acc_cfg.id = pj_str(acct_uri);

        /* disable sending re-INVITE to lock the session using a single codec */
        acc_cfg.lock_codec = 0;

        acc_cfg.use_rfc5626 = PJ_FALSE;
        acc_cfg.register_on_acc_add = PJ_TRUE;
        acc_cfg.reg_retry_interval = 60;
        acc_cfg.allow_contact_rewrite = PJ_FALSE;

        /* no REGISTER stuff when registrar is not specified */
        if (strcmp(mParams->server, "_undef_")) {
            sprintf(registrar, "sip:%s;transport=%s", mParams->server, mParams->transport);
            acc_cfg.reg_timeout = 3600;
            acc_cfg.reg_uri = pj_str(registrar);
        }
        acc_cfg.proxy[0] = pj_str(mParams->proxy);

        /* account authentication */
        acc_cfg.cred_count = 1;
        acc_cfg.cred_info[0].realm = pj_str("*");
        acc_cfg.cred_info[0].scheme = pj_str("digest");
        acc_cfg.cred_info[0].username = pj_str(mParams->username);
        acc_cfg.cred_info[0].data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
        acc_cfg.cred_info[0].data = pj_str(mParams->password);

        /* add account to pjsua */
        status = pjsua_acc_add(&acc_cfg, PJ_TRUE, &acc_id);
        if (status != PJ_SUCCESS)
            error_exit("Unable to add account!\n", status);

        /* set transport MUST be done after adding the account */
        pjsua_acc_set_transport(acc_id, transport_id);
        
        /* create audio player and slot */
        init_laudio_config(mParams->ringFile, mParams->outputFile);
    }

    /* codec setting */
    pjsua_codec_info supported_codecs[32];
    int cdcount = PJ_ARRAY_SIZE(supported_codecs);
    pjsua_enum_codecs(supported_codecs, &cdcount);

    codec_setting(cdcount, supported_codecs, mParams->codecs);

    /* wait for commands */
    char *input, *params[4], *str;
    for (;;) {
        input = (char *)malloc(MAX_INPUT_SIZE * sizeof(char));

        fflush(stdin);
        if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
            puts("EOF while reading stdin, will quit now..");
            break;
        }

        if (input[0] == '\n')
            // user press enter for nothing
            continue;

        if (input[strlen(input) -1] == '\n')
            input[strlen(input) -1] = '\0';

        int i = 0;
        while ((str = strsep(&input, " ")) != NULL) {
            params[i++] = str;
        }

        // params[0] should be the command
        // incase of transfering and calling, params[1] should be the uri of the transfer target
        if (!strcmp(params[0], "quit") || !strcmp(params[0], "shutdown")) {
            break;
        } else if (!strcmp(params[0], "hangup") || !strcmp(params[0], "hup")) {
            pjsua_call_hangup_all();
        } else if (!strcmp(params[0], "help")) {
            show_help();
        } else if (!strcmp(params[0], "codecs")) {
            for (int i = 0; i < cdcount; i++) {
                PJ_LOG(3, (THIS_FILE, "Codec %s suppoted", supported_codecs[i].codec_id));
            }
        } else if (!strcmp(params[0], "call")) {
            if (strcmp(mParams->outputFile, "_undef_")) {
                PJ_LOG(3, (THIS_FILE, "Will play sound file instead of voice from user's micro"));
                // pjsua_set_null_snd_dev();
            }
            status = do_call(acc_id, params[1], mParams->proxy, NULL, NULL, NULL, &call_id);
        } else if (!strcmp(params[0], "callr")) {
            // call with Replaces header for replacing an existing call.
            if (strcmp(mParams->outputFile, "_undef_")) {
                PJ_LOG(3, (THIS_FILE, "Will play sound file instead of voice from user's micro"));
                // pjsua_set_null_snd_dev();
            }
            pjsua_msg_data msg_data;
            pjsua_msg_data_init(&msg_data);
            pjsip_generic_string_hdr x_replaces;

            pj_str_t hname = pj_str("Replaces");
            char *cvalue = (char*)malloc(sizeof(char)*100);
            sprintf(cvalue, "%s;from-tag=%s;to-tag=%s", params[2], params[3], params[4]);
            pj_str_t hvalue = pj_str(cvalue);
            pjsip_generic_string_hdr_init2(&x_replaces, &hname, &hvalue);
            pj_list_push_back(&msg_data.hdr_list, &x_replaces);
            status = do_call(acc_id, params[1], mParams->proxy, NULL, NULL, &msg_data, &call_id);
            pjsua_call_set_hold(call_id, NULL);
            pjsua_call_reinvite(call_id, PJSUA_CALL_UNHOLD, NULL);

        } else if (!strcmp(params[0], "transfer")) {
            if (!pjsua_call_is_active(call_id)) {
                PJ_LOG(3, (THIS_FILE, "There's no active call!"));
            }else{
                do_transfer(call_id, params[1], mParams->proxy, NULL);
            }
        } else if (!strcmp(params[0], "register")) {
            pjsua_acc_set_registration(acc_id, PJ_TRUE);
        } else if (!strcmp(params[0], "unregister")) {
            pjsua_acc_set_registration(acc_id, PJ_FALSE);
        } else if (!strcmp(params[0], "dtmf")) {
            send_dtmf(call_id, params[1]);
        } else if (!strcmp(params[0], "hold")) {
            pjsua_call_set_hold(call_id, NULL);
        } else if (!strcmp(params[0], "unhold")) {
            pjsua_call_reinvite(call_id, PJSUA_CALL_UNHOLD, NULL);
        } else if (!strcmp(params[0], "message")) {
            send_message(call_id, params[1]);
        } else if (!strcmp(params[0], "answer")) {
            pjsua_call_answer(call_id, 200, NULL, NULL);
        } else if (!strncmp(params[0], "reject-", 7)) {
            /* reject-486 command reject the call with 486 response code */
            char *code = (char*)malloc(sizeof(char) * 3);
            code = params[0] + 7;
            int int_code = atoi(code);
            pjsua_call_answer(call_id, int_code, NULL, NULL);
        } else if (!strncmp(params[0], "redirect-", 9)){
            /* redirect-65123456789 command will reject the call with 302 and fill the contact with 65123456789 as the redirected destination */
            char *target = params[0] + 9;
            printf("target: %s", target);
            pjsua_msg_data msg_data;
            pjsua_msg_data_init(&msg_data);
            pjsip_generic_string_hdr x_hoiio_txnid;

            pj_str_t hname = pj_str("Contact");
            char *cvalue = (char*)malloc(sizeof(char)*30);
            sprintf(cvalue, "sip:%s@192.168.1.44", target);
            pj_str_t hvalue = pj_str(cvalue);
            pjsip_generic_string_hdr_init2(&x_hoiio_txnid, &hname, &hvalue);
            pj_list_push_back(&msg_data.hdr_list, &x_hoiio_txnid);
            pjsua_call_answer(call_id, 302, NULL, &msg_data);
        } else {
            PJ_LOG(3, (THIS_FILE, "Invalid command. Enter <help> for instruction"));
        }
    }

    /* destroy pjsua afterall */
    PJ_LOG(3, (THIS_FILE, "pjsua destroyed"));
    pjsua_destroy();

    return 0;
}
