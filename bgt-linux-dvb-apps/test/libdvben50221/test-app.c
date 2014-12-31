/*
    en50221 encoder An implementation for libdvb
    an implementation for the en50221 transport layer

    Copyright (C) 2004, 2005 Manu Abraham (manu@kromtek.com)
    Copyright (C) 2005 Julian Scheel (julian at jusst dot de)
    Copyright (C) 2006 Andrew de Quincey (adq_dvb@lidskialf.net)

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation; either version 2.1 of
    the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <libdvben50221/en50221_session.h>
#include <libdvben50221/en50221_app_utils.h>
#include <libdvben50221/en50221_app_ai.h>
#include <libdvben50221/en50221_app_auth.h>
#include <libdvben50221/en50221_app_ca.h>
#include <libdvben50221/en50221_app_datetime.h>
#include <libdvben50221/en50221_app_dvb.h>
#include <libdvben50221/en50221_app_epg.h>
#include <libdvben50221/en50221_app_lowspeed.h>
#include <libdvben50221/en50221_app_mmi.h>
#include <libdvben50221/en50221_app_rm.h>
#include <libdvben50221/en50221_app_smartcard.h>
#include <libdvben50221/en50221_app_teletext.h>
#include <libdvbapi/dvbca.h>
#include <pthread.h>
#include <libdvbcfg/dvbcfg_zapchannel.h>
#include <libdvbapi/dvbdemux.h>
#include <libucsi/section.h>
#include <libucsi/mpeg/section.h>

#define DEFAULT_SLOT 0

#define MAX_SESSIONS 256
#define MAX_TC 32

void *stackthread_func(void* arg);
void *pmtthread_func(void* arg);
int test_lookup_callback(void *arg, uint8_t slot_id, uint32_t requested_resource_id,
                         en50221_sl_resource_callback *callback_out, void **arg_out, uint32_t *connected_resource_id);
int test_session_callback(void *arg, int reason, uint8_t slot_id, uint16_t session_number, uint32_t resource_id);

int test_datetime_enquiry_callback(void *arg, uint8_t slot_id, uint16_t session_number, uint8_t response_interval);

int test_rm_enq_callback(void *arg, uint8_t slot_id, uint16_t session_number);
int test_rm_reply_callback(void *arg, uint8_t slot_id, uint16_t session_number, uint32_t resource_id_count, uint32_t *resource_ids);
int test_rm_changed_callback(void *arg, uint8_t slot_id, uint16_t session_number);

int test_ai_callback(void *arg, uint8_t slot_id, uint16_t session_number,
                     uint8_t application_type, uint16_t application_manufacturer,
                     uint16_t manufacturer_code, uint8_t menu_string_length,
                     uint8_t *menu_string);

int test_ca_info_callback(void *arg, uint8_t slot_id, uint16_t session_number, uint32_t ca_id_count, uint16_t *ca_ids);
int test_ca_pmt_reply_callback(void *arg, uint8_t slot_id, uint16_t session_number,
                               struct en50221_app_pmt_reply *reply, uint32_t reply_size);

int test_mmi_close_callback(void *arg, uint8_t slot_id, uint16_t session_number, uint8_t cmd_id, uint8_t delay);

int test_mmi_display_control_callback(void *arg, uint8_t slot_id, uint16_t session_number,
                                        uint8_t cmd_id, uint8_t mmi_mode);

int test_mmi_keypad_control_callback(void *arg, uint8_t slot_id, uint16_t session_number,
                                        uint8_t cmd_id, uint8_t *key_codes, uint32_t key_codes_count);

int test_mmi_subtitle_segment_callback(void *arg, uint8_t slot_id, uint16_t session_number,
                                        uint8_t *segment, uint32_t segment_size);

int test_mmi_scene_end_mark_callback(void *arg, uint8_t slot_id, uint16_t session_number,
                                        uint8_t decoder_continue_flag, uint8_t scene_reveal_flag,
                                        uint8_t send_scene_done, uint8_t scene_tag);

int test_mmi_scene_control_callback(void *arg, uint8_t slot_id, uint16_t session_number,
                                    uint8_t decoder_continue_flag, uint8_t scene_reveal_flag,
                                    uint8_t scene_tag);

int test_mmi_subtitle_download_callback(void *arg, uint8_t slot_id, uint16_t session_number,
                                        uint8_t *segment, uint32_t segment_size);

int test_mmi_flush_download_callback(void *arg, uint8_t slot_id, uint16_t session_number);

int test_mmi_enq_callback(void *arg, uint8_t slot_id, uint16_t session_number,
                            uint8_t blind_answer, uint8_t expected_answer_length,
                            uint8_t *text, uint32_t text_size);

int test_mmi_menu_callback(void *arg, uint8_t slot_id, uint16_t session_number,
                            struct en50221_app_mmi_text *title,
                            struct en50221_app_mmi_text *sub_title,
                            struct en50221_app_mmi_text *bottom,
                            uint32_t item_count, struct en50221_app_mmi_text *items,
                            uint32_t item_raw_length, uint8_t *items_raw);

int test_app_mmi_list_callback(void *arg, uint8_t slot_id, uint16_t session_number,
                                struct en50221_app_mmi_text *title,
                                struct en50221_app_mmi_text *sub_title,
                                struct en50221_app_mmi_text *bottom,
                                uint32_t item_count, struct en50221_app_mmi_text *items,
                                uint32_t item_raw_length, uint8_t *items_raw);

struct section_ext *read_section_ext(char *buf, int buflen, int adapter, int demux, int pid, int table_id);







int adapterid;

int shutdown_stackthread = 0;
int shutdown_pmtthread = 0;
int in_menu = 0;
int in_enq = 0;
int ca_connected = 0;
int pmt_pid = -1;
int ca_session_number = 0;


// instances of resources we actually implement here
struct en50221_app_rm *rm_resource;
struct en50221_app_datetime *datetime_resource;
struct en50221_app_ai *ai_resource;
struct en50221_app_ca *ca_resource;
struct en50221_app_mmi *mmi_resource;

// lookup table used in resource manager implementation
struct resource {
    struct en50221_app_public_resource_id resid;
    uint32_t binary_resource_id;
    en50221_sl_resource_callback callback;
    void *arg;
};
struct resource resources[20];
int resources_count = 0;

// this contains all known resource ids so we can see if the cam asks for something exotic
uint32_t resource_ids[] = { EN50221_APP_TELETEXT_RESOURCEID,
                            EN50221_APP_SMARTCARD_RESOURCEID(1),
                            EN50221_APP_RM_RESOURCEID,
                            EN50221_APP_MMI_RESOURCEID,
                            EN50221_APP_LOWSPEED_RESOURCEID(1,1),
                            EN50221_APP_EPG_RESOURCEID(1),
                            EN50221_APP_DVB_RESOURCEID,
                            EN50221_APP_CA_RESOURCEID,
                            EN50221_APP_DATETIME_RESOURCEID,
                            EN50221_APP_AUTH_RESOURCEID,
                            EN50221_APP_AI_RESOURCEID, };
int resource_ids_count = sizeof(resource_ids)/4;


uint16_t ai_session_numbers[5];

uint16_t mmi_session_number;

int main(int argc, char * argv[])
{
    pthread_t stackthread;
    pthread_t pmtthread;
    struct en50221_app_send_functions sendfuncs;

    if ((argc < 2) || (argc > 3)) {
        fprintf(stderr, "Syntax: test-app <adapterid> [<pmtpid>]\n");
        exit(1);
    }
    adapterid = atoi(argv[1]);
    if (argc == 3) {
        if (sscanf(argv[2], "%i", &pmt_pid) != 1) {
            fprintf(stderr, "Unable to parse PMT PID\n");
            exit(1);
        }
    }

    // create transport layer
    struct en50221_transport_layer *tl = en50221_tl_create(5, 32);
    if (tl == NULL) {
        fprintf(stderr, "Failed to create transport layer\n");
        exit(1);
    }

    // find CAMs
    int cafd;
    if (((cafd = dvbca_open(adapterid, 0)) < 0) || (dvbca_get_cam_state(cafd, DEFAULT_SLOT) == DVBCA_CAMSTATE_MISSING)) {
        fprintf(stderr, "Unable to open CAM on adapter %i\n", adapterid);
        exit(1);
    }

    // reset it and wait
    dvbca_reset(cafd, DEFAULT_SLOT);
    printf("Found a CAM on adapter%i... waiting...\n", adapterid);
    while(dvbca_get_cam_state(cafd, DEFAULT_SLOT) != DVBCA_CAMSTATE_READY) {
        usleep(1000);
    }

    // register it with the CA stack
    int slot_id = 0;
    if ((slot_id = en50221_tl_register_slot(tl, cafd, DEFAULT_SLOT, 1000, 100)) < 0) {
        fprintf(stderr, "Slot registration failed\n");
        exit(1);
    }
    printf("slotid: %i\n", slot_id);

    // create session layer
    struct en50221_session_layer *sl = en50221_sl_create(tl, 256);
    if (sl == NULL) {
        fprintf(stderr, "Failed to create session layer\n");
        exit(1);
    }

    // create the sendfuncs
    sendfuncs.arg        = sl;
    sendfuncs.send_data  = (en50221_send_data) en50221_sl_send_data;
    sendfuncs.send_datav = (en50221_send_datav) en50221_sl_send_datav;

    // create the resource manager resource
    rm_resource = en50221_app_rm_create(&sendfuncs);
    en50221_app_decode_public_resource_id(&resources[resources_count].resid, EN50221_APP_RM_RESOURCEID);
    resources[resources_count].binary_resource_id = EN50221_APP_RM_RESOURCEID;
    resources[resources_count].callback = (en50221_sl_resource_callback) en50221_app_rm_message;
    resources[resources_count].arg = rm_resource;
    en50221_app_rm_register_enq_callback(rm_resource, test_rm_enq_callback, NULL);
    en50221_app_rm_register_reply_callback(rm_resource, test_rm_reply_callback, NULL);
    en50221_app_rm_register_changed_callback(rm_resource, test_rm_changed_callback, NULL);
    resources_count++;

    // create the datetime resource
    datetime_resource = en50221_app_datetime_create(&sendfuncs);
    en50221_app_decode_public_resource_id(&resources[resources_count].resid, EN50221_APP_DATETIME_RESOURCEID);
    resources[resources_count].binary_resource_id = EN50221_APP_DATETIME_RESOURCEID;
    resources[resources_count].callback = (en50221_sl_resource_callback) en50221_app_datetime_message;
    resources[resources_count].arg = datetime_resource;
    en50221_app_datetime_register_enquiry_callback(datetime_resource, test_datetime_enquiry_callback, NULL);
    resources_count++;

    // create the application information resource
    ai_resource = en50221_app_ai_create(&sendfuncs);
    en50221_app_decode_public_resource_id(&resources[resources_count].resid, EN50221_APP_AI_RESOURCEID);
    resources[resources_count].binary_resource_id = EN50221_APP_AI_RESOURCEID;
    resources[resources_count].callback = (en50221_sl_resource_callback) en50221_app_ai_message;
    resources[resources_count].arg = ai_resource;
    en50221_app_ai_register_callback(ai_resource, test_ai_callback, NULL);
    resources_count++;

    // create the CA resource
    ca_resource = en50221_app_ca_create(&sendfuncs);
    en50221_app_decode_public_resource_id(&resources[resources_count].resid, EN50221_APP_CA_RESOURCEID);
    resources[resources_count].binary_resource_id = EN50221_APP_CA_RESOURCEID;
    resources[resources_count].callback = (en50221_sl_resource_callback) en50221_app_ca_message;
    resources[resources_count].arg = ca_resource;
    en50221_app_ca_register_info_callback(ca_resource, test_ca_info_callback, NULL);
    en50221_app_ca_register_pmt_reply_callback(ca_resource, test_ca_pmt_reply_callback, NULL);
    resources_count++;

    // create the MMI resource
    mmi_resource = en50221_app_mmi_create(&sendfuncs);
    en50221_app_decode_public_resource_id(&resources[resources_count].resid, EN50221_APP_MMI_RESOURCEID);
    resources[resources_count].binary_resource_id = EN50221_APP_MMI_RESOURCEID;
    resources[resources_count].callback = (en50221_sl_resource_callback) en50221_app_mmi_message;
    resources[resources_count].arg = mmi_resource;
    en50221_app_mmi_register_close_callback(mmi_resource, test_mmi_close_callback, NULL);
    en50221_app_mmi_register_display_control_callback(mmi_resource, test_mmi_display_control_callback, NULL);
    en50221_app_mmi_register_keypad_control_callback(mmi_resource, test_mmi_keypad_control_callback, NULL);
    en50221_app_mmi_register_subtitle_segment_callback(mmi_resource, test_mmi_subtitle_segment_callback, NULL);
    en50221_app_mmi_register_scene_end_mark_callback(mmi_resource, test_mmi_scene_end_mark_callback, NULL);
    en50221_app_mmi_register_scene_control_callback(mmi_resource, test_mmi_scene_control_callback, NULL);
    en50221_app_mmi_register_subtitle_download_callback(mmi_resource, test_mmi_subtitle_download_callback, NULL);
    en50221_app_mmi_register_flush_download_callback(mmi_resource, test_mmi_flush_download_callback, NULL);
    en50221_app_mmi_register_enq_callback(mmi_resource, test_mmi_enq_callback, NULL);
    en50221_app_mmi_register_menu_callback(mmi_resource, test_mmi_menu_callback, NULL);
    en50221_app_mmi_register_list_callback(mmi_resource, test_app_mmi_list_callback, NULL);
    resources_count++;

    // start another thread running the stack
    pthread_create(&stackthread, NULL, stackthread_func, tl);

    // start another thread parsing PMT
    if (pmt_pid != -1) {
        pthread_create(&pmtthread, NULL, pmtthread_func, tl);
    }

    // register callbacks
    en50221_sl_register_lookup_callback(sl, test_lookup_callback, sl);
    en50221_sl_register_session_callback(sl, test_session_callback, sl);

    // create a new connection on each slot
    int tc = en50221_tl_new_tc(tl, slot_id);
    printf("tcid: %i\n", tc);

    printf("Press a key to enter menu\n");
    getchar();
    en50221_app_ai_entermenu(ai_resource, ai_session_numbers[slot_id]);

    // wait
    char tmp[256];
    while(1) {
        fgets(tmp, sizeof(tmp), stdin);
        int choice = atoi(tmp);

        if (in_menu) {
            en50221_app_mmi_menu_answ(mmi_resource, mmi_session_number, choice);
            in_menu = 0;
        }
        if (in_enq) {
            uint32_t i;
            uint32_t len = strlen(tmp);
            for(i=0; i< len; i++) {
                if (!isdigit(tmp[i])) {
                    len = i;
                    break;
                }
            }
	    en50221_app_mmi_answ(mmi_resource, mmi_session_number, MMI_ANSW_ID_ANSWER, (uint8_t*) tmp, len);
            in_enq = 0;
        }
    }
    printf("Press a key to exit\n");
    getchar();

    // destroy slots
    en50221_tl_destroy_slot(tl, slot_id);
    shutdown_stackthread = 1;
    shutdown_pmtthread = 1;
    pthread_join(stackthread, NULL);
    if (pmt_pid != -1) {
        pthread_join(pmtthread, NULL);
    }

    // destroy session layer
    en50221_sl_destroy(sl);

    // destroy transport layer
    en50221_tl_destroy(tl);

    return 0;
}

int test_lookup_callback(void *arg, uint8_t slot_id, uint32_t requested_resource_id,
                         en50221_sl_resource_callback *callback_out, void **arg_out, uint32_t *connected_resource_id)
{
    struct en50221_app_public_resource_id resid;
    (void)arg;

    // decode the resource id
    if (en50221_app_decode_public_resource_id(&resid, requested_resource_id)) {
        printf("%02x:Public resource lookup callback %i %i %i\n", slot_id,
               resid.resource_class, resid.resource_type, resid.resource_version);
    } else {
        printf("%02x:Private resource lookup callback %08x\n", slot_id, requested_resource_id);
        return -1;
    }

    // FIXME: need better comparison
    // FIXME: return resourceid we actually connected to

    // try and find an instance of the resource
    int i;
    for(i=0; i<resources_count; i++) {
        if ((resid.resource_class == resources[i].resid.resource_class) &&
            (resid.resource_type == resources[i].resid.resource_type)) {
            *callback_out = resources[i].callback;
            *arg_out = resources[i].arg;
            *connected_resource_id = resources[i].binary_resource_id;
            return 0;
        }
    }

    return -1;
}

int test_session_callback(void *arg, int reason, uint8_t slot_id, uint16_t session_number, uint32_t resource_id)
{
    (void)arg;
    switch(reason) {
        case S_SCALLBACK_REASON_CAMCONNECTING:
            printf("%02x:CAM connecting to resource %08x, session_number %i\n",
                   slot_id, resource_id, session_number);
            break;
        case S_SCALLBACK_REASON_CAMCONNECTED:
            printf("%02x:CAM successfully connected to resource %08x, session_number %i\n",
                   slot_id, resource_id, session_number);

            if (resource_id == EN50221_APP_RM_RESOURCEID) {
                en50221_app_rm_enq(rm_resource, session_number);
            } else if (resource_id == EN50221_APP_AI_RESOURCEID) {
                en50221_app_ai_enquiry(ai_resource, session_number);
            } else if (resource_id == EN50221_APP_CA_RESOURCEID) {
                en50221_app_ca_info_enq(ca_resource, session_number);
                ca_session_number = session_number;
            }

            break;
        case S_SCALLBACK_REASON_CAMCONNECTFAIL:
            printf("%02x:CAM on failed to connect to resource %08x\n", slot_id, resource_id);
            break;
        case S_SCALLBACK_REASON_CONNECTED:
            printf("%02x:Host connection to resource %08x connected successfully, session_number %i\n",
                   slot_id, resource_id, session_number);
            break;
        case S_SCALLBACK_REASON_CONNECTFAIL:
            printf("%02x:Host connection to resource %08x failed, session_number %i\n",
                   slot_id, resource_id, session_number);
            break;
        case S_SCALLBACK_REASON_CLOSE:
            printf("%02x:Connection to resource %08x, session_number %i closed\n",
                   slot_id, resource_id, session_number);
            break;
        case S_SCALLBACK_REASON_TC_CONNECT:
            printf("%02x:Host originated transport connection %i connected\n", slot_id, session_number);
            break;
        case S_SCALLBACK_REASON_TC_CAMCONNECT:
            printf("%02x:CAM originated transport connection %i connected\n", slot_id, session_number);
            break;
    }
    return 0;
}



int test_rm_enq_callback(void *arg, uint8_t slot_id, uint16_t session_number)
{
    (void)arg;

    printf("%02x:%s\n", slot_id, __func__);

    if (en50221_app_rm_reply(rm_resource, session_number, resource_ids_count, resource_ids)) {
        printf("%02x:Failed to send reply to ENQ\n", slot_id);
    }

    return 0;
}

int test_rm_reply_callback(void *arg, uint8_t slot_id, uint16_t session_number, uint32_t resource_id_count, uint32_t *_resource_ids)
{
    (void)arg;
    printf("%02x:%s\n", slot_id, __func__);

    uint32_t i;
    for(i=0; i< resource_id_count; i++) {
        printf("  CAM provided resource id: %08x\n", _resource_ids[i]);
    }

    if (en50221_app_rm_changed(rm_resource, session_number)) {
        printf("%02x:Failed to send REPLY\n", slot_id);
    }

    return 0;
}

int test_rm_changed_callback(void *arg, uint8_t slot_id, uint16_t session_number)
{
    (void)arg;
    printf("%02x:%s\n", slot_id, __func__);

    if (en50221_app_rm_enq(rm_resource, session_number)) {
        printf("%02x:Failed to send ENQ\n", slot_id);
    }

    return 0;
}



int test_datetime_enquiry_callback(void *arg, uint8_t slot_id, uint16_t session_number, uint8_t response_interval)
{
    (void)arg;
    printf("%02x:%s\n", slot_id, __func__);
    printf("  response_interval:%i\n", response_interval);

    if (en50221_app_datetime_send(datetime_resource, session_number, time(NULL), -1)) {
        printf("%02x:Failed to send datetime\n", slot_id);
    }

    return 0;
}



int test_ai_callback(void *arg, uint8_t slot_id, uint16_t session_number,
                     uint8_t application_type, uint16_t application_manufacturer,
                     uint16_t manufacturer_code, uint8_t menu_string_length,
                     uint8_t *menu_string)
{
    (void)arg;

    printf("%02x:%s\n", slot_id, __func__);
    printf("  Application type: %02x\n", application_type);
    printf("  Application manufacturer: %04x\n", application_manufacturer);
    printf("  Manufacturer code: %04x\n", manufacturer_code);
    printf("  Menu string: %.*s\n", menu_string_length, menu_string);

    ai_session_numbers[slot_id] = session_number;

    return 0;
}



int test_ca_info_callback(void *arg, uint8_t slot_id, uint16_t session_number, uint32_t ca_id_count, uint16_t *ca_ids)
{
    (void)arg;
    (void)session_number;

    printf("%02x:%s\n", slot_id, __func__);
    uint32_t i;
    for(i=0; i< ca_id_count; i++) {
        printf("  Supported CA ID: %04x\n", ca_ids[i]);
    }

    ca_connected = 1;
    return 0;
}

int test_ca_pmt_reply_callback(void *arg, uint8_t slot_id, uint16_t session_number,
                               struct en50221_app_pmt_reply *reply, uint32_t reply_size)
{
    (void)arg;
    (void)session_number;
    (void)reply;
    (void)reply_size;

    printf("%02x:%s\n", slot_id, __func__);

    return 0;
}


int test_mmi_close_callback(void *arg, uint8_t slot_id, uint16_t session_number, uint8_t cmd_id, uint8_t delay)
{
    (void)arg;
    (void)session_number;

    printf("%02x:%s\n", slot_id, __func__);
    printf("  cmd_id: %02x\n", cmd_id);
    printf("  delay: %02x\n", delay);

    return 0;
}

int test_mmi_display_control_callback(void *arg, uint8_t slot_id, uint16_t session_number,
                                        uint8_t cmd_id, uint8_t mmi_mode)
{
    (void)arg;
    (void)session_number;

    printf("%02x:%s\n", slot_id, __func__);
    printf("  cmd_id: %02x\n", cmd_id);
    printf("  mode: %02x\n", mmi_mode);

    if (cmd_id == MMI_DISPLAY_CONTROL_CMD_ID_SET_MMI_MODE) {
        struct en50221_app_mmi_display_reply_details details;

        details.u.mode_ack.mmi_mode = mmi_mode;
        if (en50221_app_mmi_display_reply(mmi_resource, session_number, MMI_DISPLAY_REPLY_ID_MMI_MODE_ACK, &details)) {
            printf("%02x:Failed to send mode ack\n", slot_id);
        }
    }

    return 0;
}

int test_mmi_keypad_control_callback(void *arg, uint8_t slot_id, uint16_t session_number,
                                        uint8_t cmd_id, uint8_t *key_codes, uint32_t key_codes_count)
{
    (void)arg;
    (void)session_number;
    (void)cmd_id;
    (void)key_codes;
    (void)key_codes_count;

    printf("%02x:%s\n", slot_id, __func__);

    return 0;
}

int test_mmi_subtitle_segment_callback(void *arg, uint8_t slot_id, uint16_t session_number,
                                        uint8_t *segment, uint32_t segment_size)
{
    (void)arg;
    (void)session_number;
    (void)segment;
    (void)segment_size;

    printf("%02x:%s\n", slot_id, __func__);

    return 0;
}

int test_mmi_scene_end_mark_callback(void *arg, uint8_t slot_id, uint16_t session_number,
                                        uint8_t decoder_continue_flag, uint8_t scene_reveal_flag,
                                        uint8_t send_scene_done, uint8_t scene_tag)
{
    (void)arg;
    (void)session_number;
    (void)decoder_continue_flag;
    (void)scene_reveal_flag;
    (void)send_scene_done;
    (void)scene_tag;

    printf("%02x:%s\n", slot_id, __func__);

    return 0;
}

int test_mmi_scene_control_callback(void *arg, uint8_t slot_id, uint16_t session_number,
                                    uint8_t decoder_continue_flag, uint8_t scene_reveal_flag,
                                    uint8_t scene_tag)
{
    (void)arg;
    (void)session_number;
    (void)decoder_continue_flag;
    (void)scene_reveal_flag;
    (void)scene_tag;

    printf("%02x:%s\n", slot_id, __func__);

    return 0;
}

int test_mmi_subtitle_download_callback(void *arg, uint8_t slot_id, uint16_t session_number,
                                        uint8_t *segment, uint32_t segment_size)
{
    (void)arg;
    (void)session_number;
    (void)segment;
    (void)segment_size;

    printf("%02x:%s\n", slot_id, __func__);

    return 0;
}

int test_mmi_flush_download_callback(void *arg, uint8_t slot_id, uint16_t session_number)
{
    (void)arg;
    (void)session_number;

    printf("%02x:%s\n", slot_id, __func__);

    return 0;
}

int test_mmi_enq_callback(void *arg, uint8_t slot_id, uint16_t session_number,
                            uint8_t blind_answer, uint8_t expected_answer_length,
                            uint8_t *text, uint32_t text_size)
{
    (void)arg;
    (void)text;
    (void)text_size;

    printf("%02x:%s\n", slot_id, __func__);
    printf("  blind: %i\n", blind_answer);
    printf("  expected_answer_length: %i\n", expected_answer_length);

    mmi_session_number = session_number;
    in_enq = 1;

    return 0;
}

int test_mmi_menu_callback(void *arg, uint8_t slot_id, uint16_t session_number,
                            struct en50221_app_mmi_text *title,
                            struct en50221_app_mmi_text *sub_title,
                            struct en50221_app_mmi_text *bottom,
                            uint32_t item_count, struct en50221_app_mmi_text *items,
                            uint32_t item_raw_length, uint8_t *items_raw)
{
    (void)arg;
    (void)items_raw;

    printf("%02x:%s\n", slot_id, __func__);

    printf("  title: %.*s\n", title->text_length, title->text);
    printf("  sub_title: %.*s\n", sub_title->text_length, sub_title->text);
    printf("  bottom: %.*s\n", bottom->text_length, bottom->text);

    uint32_t i;
    for(i=0; i< item_count; i++) {
        printf("  item %i: %.*s\n", i+1, items[i].text_length, items[i].text);
    }
    printf("  raw_length: %i\n", item_raw_length);

    mmi_session_number = session_number;
    in_menu = 1;

    return 0;
}

int test_app_mmi_list_callback(void *arg, uint8_t slot_id, uint16_t session_number,
                                struct en50221_app_mmi_text *title,
                                struct en50221_app_mmi_text *sub_title,
                                struct en50221_app_mmi_text *bottom,
                                uint32_t item_count, struct en50221_app_mmi_text *items,
                                uint32_t item_raw_length, uint8_t *items_raw)
{
    (void)arg;
    (void)items_raw;
    (void)arg;

    printf("%02x:%s\n", slot_id, __func__);

    printf("  title: %.*s\n", title->text_length, title->text);
    printf("  sub_title: %.*s\n", sub_title->text_length, sub_title->text);
    printf("  bottom: %.*s\n", bottom->text_length, bottom->text);

    uint32_t i;
    for(i=0; i< item_count; i++) {
        printf("  item %i: %.*s\n", i+1, items[i].text_length, items[i].text);
    }
    printf("  raw_length: %i\n", item_raw_length);

    mmi_session_number = session_number;
    in_menu = 1;

    return 0;
}







void *stackthread_func(void* arg) {
    struct en50221_transport_layer *tl = arg;
    int lasterror = 0;

    while(!shutdown_stackthread) {
        int error;
        if ((error = en50221_tl_poll(tl)) != 0) {
            if (error != lasterror) {
                fprintf(stderr, "Error reported by stack slot:%i error:%i\n",
                        en50221_tl_get_error_slot(tl),
                        en50221_tl_get_error(tl));
            }
            lasterror = error;
        }
    }

    shutdown_stackthread = 0;
    return 0;
}

void *pmtthread_func(void* arg) {
    (void)arg;
    char buf[4096];
    uint8_t capmt[4096];
    int pmtversion = -1;

    while(!shutdown_pmtthread) {

        if (!ca_connected) {
            sleep(1);
            continue;
        }

        // read the PMT
        struct section_ext *section_ext = read_section_ext(buf, sizeof(buf), adapterid, 0, pmt_pid, stag_mpeg_program_map);
        if (section_ext == NULL) {
            fprintf(stderr, "Failed to read PMT\n");
            exit(1);
        }
        struct mpeg_pmt_section *pmt = mpeg_pmt_section_codec(section_ext);
        if (pmt == NULL) {
            fprintf(stderr, "Bad PMT received\n");
            exit(1);
        }
        if (pmt->head.version_number == pmtversion) {
            continue;
        }

        // translate it into a CA PMT
        int listmgmt = CA_LIST_MANAGEMENT_ONLY;
        if (pmtversion != -1) {
            listmgmt = CA_LIST_MANAGEMENT_UPDATE;
        }
        int size;
        if ((size = en50221_ca_format_pmt(pmt,
                                         capmt,
                                         sizeof(capmt),
                                         listmgmt,
                                         0,
                                         CA_PMT_CMD_ID_OK_DESCRAMBLING)) < 0) {
            fprintf(stderr, "Failed to format CA PMT object\n");
            exit(1);
        }

        // set it
        if (en50221_app_ca_pmt(ca_resource, ca_session_number, capmt, size)) {
            fprintf(stderr, "Failed to send CA PMT object\n");
            exit(1);
        }
        pmtversion = pmt->head.version_number;
    }
    shutdown_pmtthread = 0;
    return 0;
}


struct section_ext *read_section_ext(char *buf, int buflen, int adapter, int demux, int pid, int table_id)
{
    int demux_fd = -1;
    uint8_t filter[18];
    uint8_t mask[18];
    int size;
    struct section *section;
    struct section_ext *result = NULL;

    // open the demuxer
    if ((demux_fd = dvbdemux_open_demux(adapter, demux, 0)) < 0) {
        goto exit;
    }

    // create a section filter
    memset(filter, 0, sizeof(filter));
    memset(mask, 0, sizeof(mask));
    filter[0] = table_id;
    mask[0] = 0xFF;
    if (dvbdemux_set_section_filter(demux_fd, pid, filter, mask, 1, 1)) {
        goto exit;
    }

    // read the section
    if ((size = read(demux_fd, buf, buflen)) < 0) {
        goto exit;
    }

    // parse it as a section
    section = section_codec((uint8_t*) buf, size);
    if (section == NULL) {
        goto exit;
    }

    // parse it as a section_ext
    result = section_ext_decode(section, 0);

exit:
    if (demux_fd != -1)
        close(demux_fd);
    return result;
}
