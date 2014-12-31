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
#include <unistd.h>
#include <libdvben50221/en50221_session.h>
#include <libdvben50221/en50221_app_utils.h>
#include <libdvbapi/dvbca.h>
#include <pthread.h>

void *stackthread_func(void* arg);
int test_lookup_callback(void *arg, uint8_t slot_id, uint32_t requested_resource_id,
                         en50221_sl_resource_callback *callback_out, void **arg_out, uint32_t *connected_resource_id);
int test_session_callback(void *arg, int reason, uint8_t slot_id, uint16_t session_number, uint32_t resource_id);


int shutdown_stackthread = 0;

#define DEFAULT_SLOT 0

int main(int argc, char * argv[])
{
    (void)argc;
    (void)argv;

    int i;
    pthread_t stackthread;

    // create transport layer
    struct en50221_transport_layer *tl = en50221_tl_create(5, 32);
    if (tl == NULL) {
        fprintf(stderr, "Failed to create transport layer\n");
        exit(1);
    }

    // find CAMs
    int slot_count = 0;
    int cafd= -1;
    for(i=0; i<20; i++) {
        if ((cafd = dvbca_open(i, 0)) > 0) {
            if (dvbca_get_cam_state(cafd, DEFAULT_SLOT) == DVBCA_CAMSTATE_MISSING) {
                close(cafd);
                continue;
            }

            // reset it and wait
            dvbca_reset(cafd, DEFAULT_SLOT);
            printf("Found a CAM on adapter%i... waiting...\n", i);
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
            slot_count++;
        }
    }

    // create session layer
    struct en50221_session_layer *sl = en50221_sl_create(tl, 256);
    if (sl == NULL) {
        fprintf(stderr, "Failed to create session layer\n");
        exit(1);
    }

    // start another thread running the stack
    pthread_create(&stackthread, NULL, stackthread_func, tl);

    // register callbacks
    en50221_sl_register_lookup_callback(sl, test_lookup_callback, sl);
    en50221_sl_register_session_callback(sl, test_session_callback, sl);

    // create a new connection
    for(i=0; i<slot_count; i++) {
        int tc = en50221_tl_new_tc(tl, i);
        printf("tcid: %i\n", tc);
    }

    // wait
    printf("Press a key to exit\n");
    getchar();

    // destroy slots
    for(i=0; i<slot_count; i++) {
        en50221_tl_destroy_slot(tl, i);
    }
    shutdown_stackthread = 1;
    pthread_join(stackthread, NULL);

    // destroy session layer
    en50221_sl_destroy(sl);

    // destroy transport layer
    en50221_tl_destroy(tl);

    return 0;
}

int test_lookup_callback(void *arg, uint8_t slot_id, uint32_t requested_resource_id,
                         en50221_sl_resource_callback *callback_out, void **arg_out, uint32_t *connected_resource_id)
{
    (void)arg;
    (void)callback_out;
    (void)arg_out;
    (void)connected_resource_id;

    struct en50221_app_public_resource_id resid;

    if (en50221_app_decode_public_resource_id(&resid, requested_resource_id)) {
        printf("Public resource lookup callback %i %i %i %i\n", slot_id,
               resid.resource_class, resid.resource_type, resid.resource_version);
    } else {
        printf("Private resource lookup callback %i %08x\n", slot_id, requested_resource_id);
    }

    return -1;
}

int test_session_callback(void *arg, int reason, uint8_t slot_id, uint16_t session_number, uint32_t resource_id)
{
   (void)arg;

    printf("Session callback %i %i %i %04x\n", slot_id, session_number, reason, resource_id);

    return -1;
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
