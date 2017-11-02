#ifndef IRC_H
#define IRC_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

struct channel {
    char *name;
    uint32_t group_num;
    int index;
    bool in_channel;
};

struct irc {
    char *server;
    int port;
    struct channel *channels;
    int num_channels;
    int size_channels;
    int sock;
    bool connected;
};

typedef struct irc IRC;
typedef struct channel CHANNEL;

#define SILENT_TIMEOUT 20

/*
 * Connects to the IRC server and creates the IRC struct
 * returns a IRC struct on success
 * returns NULL on failure
 */
IRC *irc_connect(char *server, int port);

/*
 * Joins the specified IRC channel
 * returns true on success
 * returns false on failure
 */
bool irc_join_channel(IRC *irc, char *channel);

/*
 * Leaves the specified IRC channel
 * returns true on success
 * returns false on failure
 */
bool irc_leave_channel(IRC *irc, int index);

/*
 * Disconnects from the IRC server
 */
void irc_disconnect(IRC *irc);

/*
 * Leaves all channels
 */
void irc_leave_all_channels(IRC *irc);

/*
 * Send msg with length len to the IRC server
 * on success returns the number of bytes sent
 * on failure returns -1
 */
int irc_send(int sock, char *msg, int len);


/*
 * Send a formated message to the IRC server
 * on success returns the number of bytes sent
 * on failure returns -1
 */
int irc_send_fmt(int sock, char *fmt, ...);

/*
 * Sends the specified name and message to the specified channel
 */
void irc_message(int sock, char *channel, char *name, char *msg);

/*
 * Frees the IRC struct and irc->channels.
 * If the connection hasn't been closed it will also disconnect from the IRC server
 */
void irc_free(IRC *irc);

/*
 * Gets the specifed channel's index
 * returns the index on success
 * returns -1 on failure
 */
int irc_get_channel_index(IRC *irc, char *channel);

/*
 * Gets the channel name for the specified group number
 * on success returns the channel name
 * on failure returns NULL
 */
char *irc_get_channel_by_group(IRC *irc, uint32_t group_num);

#endif