/*
 * Copyright (C) 2013 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"

#include "client.h"
#include "guac_handlers.h"
#include "ssh_key.h"
#include "terminal.h"

#include <cairo/cairo.h>
#include <guacamole/client.h>
#include <libssh2.h>
#include <libssh2_sftp.h>

#include <pthread.h>
#include <stdlib.h>

int ssh_guac_client_handle_messages(guac_client* client) {

    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    return guac_terminal_render_frame(client_data->term);

}

int ssh_guac_client_mouse_handler(guac_client* client, int x, int y, int mask) {

    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    guac_terminal* term = client_data->term;

    /* Send mouse event */
    guac_terminal_send_mouse(term, x, y, mask);
    return 0;

}

int ssh_guac_client_key_handler(guac_client* client, int keysym, int pressed) {

    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    guac_terminal* term = client_data->term;

    /* Send key */
    guac_terminal_send_key(term, keysym, pressed);
    return 0;

}

int ssh_guac_client_size_handler(guac_client* client, int width, int height) {

    /* Get terminal */
    ssh_guac_client_data* guac_client_data = (ssh_guac_client_data*) client->data;
    guac_terminal* terminal = guac_client_data->term;

    /* Resize terminal */
    guac_terminal_resize(terminal, width, height);

    /* Update SSH pty size if connected */
    if (guac_client_data->term_channel != NULL) {
        pthread_mutex_lock(&(guac_client_data->term_channel_lock));
        libssh2_channel_request_pty_size(guac_client_data->term_channel,
                terminal->term_width, terminal->term_height);
        pthread_mutex_unlock(&(guac_client_data->term_channel_lock));
    }

    return 0;
}

int ssh_guac_client_free_handler(guac_client* client) {

    ssh_guac_client_data* guac_client_data = (ssh_guac_client_data*) client->data;

    /* Close SSH channel */
    if (guac_client_data->term_channel != NULL) {
        libssh2_channel_send_eof(guac_client_data->term_channel);
        libssh2_channel_close(guac_client_data->term_channel);
    }

    /* Free terminal */
    guac_terminal_free(guac_client_data->term);
    pthread_join(guac_client_data->client_thread, NULL);

    /* Free channels */
    libssh2_channel_free(guac_client_data->term_channel);

    /* Clean up SFTP */
    if (guac_client_data->sftp_session)
        libssh2_sftp_shutdown(guac_client_data->sftp_session);

    if (guac_client_data->sftp_ssh_session) {
        libssh2_session_disconnect(guac_client_data->sftp_ssh_session, "Bye");
        libssh2_session_free(guac_client_data->sftp_ssh_session);
    }

    /* Free session */
    if (guac_client_data->session != NULL)
        libssh2_session_free(guac_client_data->session);

    /* Free auth key */
    if (guac_client_data->key != NULL)
        ssh_key_free(guac_client_data->key);

    /* Free generic data struct */
    free(client->data);

    return 0;
}

