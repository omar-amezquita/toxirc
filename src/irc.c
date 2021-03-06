#include "irc.h"

#include "logging.h"
#include "settings.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

IRC *irc_init(char *server, char *port){
    IRC *irc = malloc(sizeof(IRC));
    if (!irc) {
        DEBUG("IRC", "Could not allocate memory for irc structure.");
        return NULL;
    }

    memset(irc, 0, sizeof(IRC));

    irc->server = server;
    irc->port = port;

    return irc;
}

bool irc_connect(IRC *irc){
    DEBUG("IRC", "Connecting to %s:%s", irc->server, irc->port);

    struct addrinfo hints;
    struct addrinfo *result, *rp;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;

    int ret = getaddrinfo(irc->server, irc->port, &hints, &result);
    if (ret != 0) {
        DEBUG("IRC", "Error getting address information for %s.", irc->server);
        return false;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        irc->sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (irc->sock == -1) {
            continue;
        }

        if (connect(irc->sock, rp->ai_addr, rp->ai_addrlen) != -1) {
            break;
        }

        close(irc->sock);
    }

    freeaddrinfo(result);

    irc_send(irc->sock, "PASS none\n", sizeof("PASS none\n") - 1);
    irc_send_fmt(irc->sock, "NICK %s\n", settings.name);
    irc_send_fmt(irc->sock, "USER %s %s %s :%s\n", settings.name, settings.name, settings.name, settings.name);

    irc->connected = true;

    DEBUG("IRC", "Connected to %s", irc->server);

    return true;
}

bool irc_reconnect(IRC *irc){
    irc_disconnect(irc);

    if (!irc_connect(irc)) {
        return false;
    }

    for (unsigned int i = 0; i < irc->num_channels; i++) {
        irc_rejoin_channel(irc, i);
    }

    return true;
}

bool irc_join_channel(IRC *irc, char *channel, uint32_t group_num){
    if (group_num >= irc->size_channels) {
        DEBUG("IRC", "Reallocating from %d to %d", irc->size_channels, group_num + 1);
        void *temp = realloc(irc->channels, sizeof(Channel) * (group_num + 1));
        if (!temp) {
            DEBUG("IRC", "Could not reallocate memory from %d to %d.", irc->size_channels, group_num);
            return false;
        }

        irc->channels = temp;

        irc->size_channels++;
        irc->num_channels++;

        memset(&irc->channels[irc->num_channels - 1], 0, sizeof(Channel));
    }

    irc_send_fmt(irc->sock, "JOIN %s\n", channel);

    int index = irc->num_channels - 1;
    memcpy(irc->channels[index].name, channel, strlen(channel));
    irc->channels[index].in_channel = true;
    irc->channels[index].group_num = group_num;

    DEBUG("IRC", "Joining channel: %s", channel);

    return true;
}

void irc_rejoin_channel(IRC *irc, int index){
    irc->channels[index].in_channel = true;
    irc_send_fmt(irc->sock, "JOIN %s\n", irc->channels[index].name);
}

bool irc_leave_channel(IRC *irc, int index){
    irc_send_fmt(irc->sock, "PART %s\n", irc->channels[index].name);

    memset(&irc->channels[index], 0, sizeof(Channel));

    irc->num_channels--;

    return true;
}

void irc_disconnect(IRC *irc){
    irc_send(irc->sock, "QUIT\n", sizeof("QUIT\n") - 1);
    irc->connected = false;
    close(irc->sock);
    for (unsigned int i = 0; i < irc->num_channels; i++) {
        irc->channels[i].in_channel = false;
    }
    DEBUG("IRC", "Disconnected from server: %s.", irc->server);
}

void irc_leave_all_channels(IRC *irc){
    for (unsigned int i = 0; i < irc->num_channels; i++) {
        if (irc->channels[i].in_channel) {
            irc_leave_channel(irc, i);
        }
    }
}

void irc_free(IRC *irc){
    if (!irc) {
        return;
    }

    if (irc->channels) {
        free(irc->channels);
    }

    if (irc->connected) {
        irc_disconnect(irc);
    }

    free(irc);

    irc = NULL;
}

int irc_send(int sock, char *msg, int len){
    if (sock < 0) {
        DEBUG("IRC", "Bad socket. Unable to send data.");
        return -1;
    }

    int sent = 0, bytes = 0;

    while (sent < len) {
        bytes = send(sock, msg + sent, len - sent, MSG_NOSIGNAL);
        if (bytes <= 0) {
            DEBUG("IRC", "Problem sending data.");
            return -1;
        }

        sent += bytes;
    }

    return sent;
}

int irc_send_fmt(int sock, char *fmt, ...){
    char buf[512];
    va_list list;
    int len, sent;

    va_start(list, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, list);
    va_end(list);

    if (len > 512) {
        len = 512;
    }

    sent = irc_send(sock, buf, len);
    if (sent <= 0) {
        return -1;
    }

    return sent;
}

int irc_message(int sock, char *channel, char *name, char *msg){
    return irc_send_fmt(sock, "PRIVMSG %s :<%s> %s\n", channel, name, msg);
}

int irc_get_channel_index(IRC *irc, char *channel){
    for (unsigned int i = 0; i < irc->num_channels; i++) {
        if (strcmp(channel, irc->channels[i].name) == 0) {
            return i;
        }
    }

    return -1;
}

uint32_t irc_get_channel_group(IRC *irc, char *channel){
    for (unsigned int i = 0; i < irc->num_channels; i++) {
        if (strcmp(channel, irc->channels[i].name) == 0) {
            return i;
        }
    }

    return UINT32_MAX;
}

char *irc_get_channel_by_group(IRC *irc, uint32_t group_num){
    for (unsigned int i = 0; i < irc->num_channels; i++) {
        if (irc->channels[i].group_num == group_num) {
            return irc->channels[i].name;
        }
    }

    return NULL;
}

bool irc_in_channel(IRC *irc, char *channel){
    for (unsigned i = 0; i < irc->num_channels; i++) {
        if (strcmp(irc->channels[i].name, channel) == 0 && irc->channels[i].in_channel) {
            return true;
        }
    }

    return false;
}
