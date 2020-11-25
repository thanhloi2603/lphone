#include <stdio.h>
#include <string.h>
#include <pjsua-lib/pjsua.h>
#include "const.h"

struct laudio_config_s {
    pjsua_conf_port_id ringback_slot;           // for ringing purpose
    pjsua_player_id ring_player;                // player for ringing

    pjsua_conf_port_id media_player_slot;       // for playing media to other endpoint
    pjsua_player_id media_player;               // player for media

    pjsua_conf_port_id remote_slot;             // the other end media slot
    pjsua_conf_port_id user_slot;               // from user's speaker, to user's sound device, default to 0.

    /* There are 2 scenarios for an established call
        1. user-speech
        - local sound device       <- remote - connect(remote_slot, user_slot)
        - local microphone device  -> remote - connect(user_slot, remote_slot)

        2. auto-media voice
        - local sound device       <- remote - connect(remote_slot, user_slot)
        - local preconfigured file -> remote - connect(media_player_slot, remote_slot)
    */
};
typedef struct laudio_config_s laudio_config_t;

extern laudio_config_t laudio_config;

/* Codec descriptor: */
struct codec
{
    unsigned    pt;
    char*   name;
    unsigned    clock_rate;
    unsigned    bit_rate;
    unsigned    ptime;
    char*   identification;
};

/* ------------------------------------------ Call functions --------------------------------------------*/
pj_status_t do_call(pjsua_acc_id acc_id, char *dst_uri, char *proxy, const pjsua_call_setting *opt, 
    void *user_data, const pjsua_msg_data *msg_data, pjsua_call_id *p_call_id);
pj_status_t do_transfer(pjsua_call_id call_id, char *target, char *proxy, const pjsua_msg_data *msg_data);
pj_status_t send_dtmf(pjsua_call_id call_id, char *digits);
pj_status_t send_message(pjsua_call_id call_id, char *content);

/* ------------------------------------------ Callback functions ----------------------------------------*/
void on_call_state(pjsua_call_id call_id, pjsip_event *e);
void on_call_media_state(pjsua_call_id call_id);
void on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata);

/* ------------------------------------------ Utility functions -----------------------------------------*/
void error_exit(const char *title, pj_status_t status);
void init_laudio_config(char *ring_file, char *media_file);
void ringback_start();
void ring_start();
void ring_stop();
void media_all_stop();
void codec_setting(int cdcount, pjsua_codec_info supported_codecs[], char allow_codecs[MAX_ALLOWED_CODEC][20]);
