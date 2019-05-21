#include "call_func.h"
#include "lutils.h"
#define THIS_FILE "call_func.c"

/* ------------------------------------------ Call functions implementation--------------------------------------------*/

/* initiate INVITE for calling a destination */
pj_status_t do_call(pjsua_acc_id acc_id, char *dest, char *proxy, const pjsua_call_setting *opt, 
    void *user_data, const pjsua_msg_data *msg_data, pjsua_call_id *p_call_id) {
    if (dest == NULL) {
        PJ_LOG(3, (THIS_FILE, "Need to specify the call destination"));
        return PJ_FALSE;
    }

    if (proxy == NULL){
        /* This should never happens tho */
        return PJ_FALSE;
    }

    pj_str_t callee;
    char dst_sip_uri[100];

    if (pjsua_verify_sip_url(dest) != PJ_SUCCESS) {
        /* proxy call */
        sprintf(dst_sip_uri, "sip:%s@%s", dest, proxy);
    } else {
        /* direct ip call */
        strcpy(dst_sip_uri, dest);
    }

    callee = pj_str(dst_sip_uri);
    PJ_LOG(3, (THIS_FILE, "Calling %s", dst_sip_uri));
    return pjsua_call_make_call(acc_id, &callee, opt, user_data, msg_data, p_call_id);
}

/* initiate in-dialog REFER */
pj_status_t do_transfer(pjsua_call_id call_id, char *target, char *proxy, const pjsua_msg_data *msg_data) {
    if (target == NULL) {
        PJ_LOG(3, (THIS_FILE, "Need to specify the transfer target"));
        return PJ_FALSE;
    }

    if (proxy == NULL){
        /* This should never happens tho */
        return PJ_FALSE;
    }

    pj_str_t pj_target;
    char target_sip_uri[30];

    if (pjsua_verify_sip_url(target) != PJ_SUCCESS) {
        /* Transfer to proxy */
        sprintf(target_sip_uri, "sip:%s@%s", target, proxy);
    } else {
        /* direct transfer to sip_uri specified */
        strcpy(target_sip_uri, target);
    }

    PJ_LOG(3, (THIS_FILE, "Transfering this call to %s", target_sip_uri));
    pj_target = pj_str(target_sip_uri);
    return pjsua_call_xfer(call_id, &pj_target, msg_data);
}

/* send dtmf digit(s) follow rfc2833 */
pj_status_t send_dtmf(pjsua_call_id call_id, char *digits) {
    if (digits == NULL) {
        PJ_LOG(3, (THIS_FILE, "Need to specify the dtmf digit"));
        return PJ_FALSE;
    }

    if (pjsua_call_is_active(call_id) == PJ_FALSE) {
        PJ_LOG(3, (THIS_FILE, "Call is not active"));
        return PJ_FALSE;
    }

    pj_str_t pj_digits = pj_str(digits);

    return pjsua_call_dial_dtmf(call_id, &pj_digits);
}

/* initiate SIP MESSAGE */
pj_status_t send_message(pjsua_call_id call_id, char *content) {
    if (content == NULL) {
        PJ_LOG(3, (THIS_FILE, "Need to specify the content in the message"));
        return PJ_FALSE;
    }

    if (pjsua_call_is_active(call_id) == PJ_FALSE) {
        PJ_LOG(3, (THIS_FILE, "Call is not active"));
        return PJ_FALSE;
    }

    pj_str_t pj_content = pj_str(content);

    return pjsua_call_send_im(call_id, NULL, &pj_content, NULL, NULL);
}

/* throw error and exit */
void error_exit(const char *title, pj_status_t status) {
    pjsua_perror(THIS_FILE, title, status);
    pjsua_destroy();
    exit(1);
}

void init_laudio_config(char *ring_file, char *media_file) {
    pj_str_t pj_ring_file = pj_str(ring_file);
    // 0 means play file repeatedly
    pjsua_player_create(&pj_ring_file, 0, &laudio_config.ring_player);
    laudio_config.ringback_slot = pjsua_player_get_conf_port(laudio_config.ring_player);

    if (!strcmp(media_file, "_undef_")) {
        /* disable playing file, use speaker instead */
        laudio_config.media_player_slot = -1;
    } else{
        pj_str_t pj_media_file = pj_str(media_file);
        // 0 means play file repeatedly
        pjsua_player_create(&pj_media_file, 0, &laudio_config.media_player);
        laudio_config.media_player_slot = pjsua_player_get_conf_port(laudio_config.media_player);
    }

}

/* play ring when receive 180 Ringing */
void ringback_start() {
    pjsua_conf_connect(laudio_config.ringback_slot, 0);
}

/* stop ring */
void ring_stop() {
    /* reset ring player */
    pjsua_player_set_pos(laudio_config.ring_player, 0);

    /* disconnect ring slot to output device */
    pjsua_conf_disconnect(laudio_config.ringback_slot, 0);
}

/* release all media */
void media_all_stop() {
    if (laudio_config.media_player_slot != -1) {
        /* reset media player */
        pjsua_player_set_pos(laudio_config.media_player, 0);

        /* disconnect media player slot to remote slot */
        pjsua_conf_disconnect(laudio_config.media_player_slot, laudio_config.remote_slot);
    } else {
        /* disconnect input device to remote slot */
        pjsua_conf_disconnect(0, laudio_config.remote_slot);
    }
    /* disconnect remote slot to output device */
    pjsua_conf_disconnect(laudio_config.remote_slot, 0);
    ring_stop();
}

void codec_setting(int cdcount, pjsua_codec_info supported_codecs[], char allow_codecs[MAX_ALLOWED_CODEC][20]) {
    for (int i = 0; i < cdcount; i++) {
        /* disable all codecs not listed */
        pj_str_t codec_id = supported_codecs[i].codec_id;
        int allow = 0;
        for (int j = 0; j < MAX_ALLOWED_CODEC; j++)
        {
            if (allow_codecs[j][0] != '\0' && !strcasecmp(allow_codecs[j], codec_id.ptr)){
                allow = 1;
                PJ_LOG(3, (THIS_FILE, "Codec %s allowed", allow_codecs[j]));
                break;
            }
        }

        if (allow == 0){
            if (pjsua_codec_set_priority(&codec_id, PJMEDIA_CODEC_PRIO_DISABLED) == PJ_SUCCESS)
                PJ_LOG(3, (THIS_FILE, "Codec %s disabled", codec_id));
        }
    }
}

/* Callback called by the library when call's state has changed */
void on_call_state(pjsua_call_id call_id, pjsip_event *e)
{
    pjsua_call_info ci;

    PJ_UNUSED_ARG(e);

    pjsua_call_get_info(call_id, &ci);
    PJ_LOG(3, (THIS_FILE, "Call %d state=%s, %d", call_id, ci.state_text.ptr, ci.last_status));

    switch (ci.state) {
        case PJSIP_INV_STATE_DISCONNECTED:
            // destroy media
            media_all_stop();
            break;
        case PJSIP_INV_STATE_EARLY:
            /* Start ringback for 180 for UAC unless there's SDP in 180 */
            if (ci.role == PJSIP_ROLE_UAC && ci.last_status == 180 && ci.media_status == PJSUA_CALL_MEDIA_NONE) {
                ringback_start();
            }
            break;
        case PJSIP_INV_STATE_CONFIRMED:
            ring_stop();
            break;
        default:
            break;
    }
}

/* Callback called by the library when call's media state has changed */
void on_call_media_state(pjsua_call_id call_id)
{
    pjsua_call_info ci;

    pjsua_call_get_info(call_id, &ci);

    if (ci.media_status == PJSUA_CALL_MEDIA_ACTIVE) {
        // When media is active, connect call to sound device.

        laudio_config.remote_slot = pjsua_call_get_conf_port(call_id);
        if (laudio_config.media_player_slot != -1) {
            // play localfile instead
            pjsua_conf_connect(laudio_config.media_player_slot, laudio_config.remote_slot);
        } else {
            // play from local microphone
            pjsua_conf_connect(0, laudio_config.remote_slot);
        }

        // remote stream to local sound device
        pjsua_conf_connect(laudio_config.remote_slot, 0);
        ring_stop();
    }
}

/* Callback called by the library upon receiving incoming call */
void on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id,
                 pjsip_rx_data *rdata)
{
    pjsua_call_info ci;

    PJ_UNUSED_ARG(acc_id);
    PJ_UNUSED_ARG(rdata);

    pjsua_call_get_info(call_id, &ci);
    ringback_start();

    PJ_LOG(3,(THIS_FILE, "Incoming call from %.*s!!", (int)ci.remote_info.slen, ci.remote_info.ptr));

    PJ_LOG(3,(THIS_FILE, "--------------------- Enter answer/reject-<code>/redirect-<target> respectively ---------------------"));
}
