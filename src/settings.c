#include "settings.h"

#include "logging.h"
#include "macros.h"
#include "utils.h"

#include "../third-party/minini/dev/minIni.h"

#include <stdbool.h>
#include <string.h>

#define STR_TO_BOOL(x) (strcmp(x, "true") == 0)
#define BOOL_TO_STR(x) x ? "true" : "false"

enum SECTION {
    SECTION_BOT,
    SECTION_TOX,
    SECTION_IRC,
    SECTION_UNKNOWN,
};

typedef enum SECTION SECTION;

static const char *sections[SECTION_UNKNOWN + 1] = {
    "Bot",
    "Tox",
    "IRC",
    NULL,
};

// default settings
SETTINGS settings = {
    .name = "toxirc",
    .status = "Send me help for more info.",
    .ipv6 = true,
    .udp = false,
    .master = "",
    .server = "chat.freenode.net",
    .port = "6667",
    .default_channel = "#toxirc",
    .verbose = true,
};

static void settings_write_string(char *file, const char *section, char *key, char *value){
    if (ini_puts(section, key, value, file) != 1) {
        DEBUG("Settings", "Could not write  %s to %s", value, file);
    }
}

static void settings_write_bool(char *file, const char *section, char *key, bool value){
    if (ini_puts(section, key, BOOL_TO_STR(value), file) != 1) {
        DEBUG("Settings", "Could not write %s to %s", BOOL_TO_STR(value), file);
    }
}

void settings_save(char *file){
    //Bot
    settings_write_string(file, sections[SECTION_BOT], "name", settings.name);
    settings_write_string(file, sections[SECTION_BOT], "status", settings.status);
    settings_write_string(file, sections[SECTION_BOT], "master", settings.master);
    settings_write_string(file, sections[SECTION_BOT], "default_channel", settings.default_channel);
    settings_write_bool(file, sections[SECTION_BOT], "verbose", settings.verbose);

    //Tox
    settings_write_bool(file, sections[SECTION_TOX], "ipv6", settings.ipv6);
    settings_write_bool(file, sections[SECTION_TOX], "udp", settings.udp);

    //IRC
    settings_write_string(file, sections[SECTION_IRC], "server", settings.server);
    settings_write_string(file, sections[SECTION_IRC], "port", settings.port);
}

static SECTION get_section(const char *section){
    for (int i = 0; i < SECTION_UNKNOWN; i++) {
        if (strcmp(section, sections[i]) == 0) {
            return i;
        }
    }

    return SECTION_UNKNOWN;
}

static void parse_bot_section(const char *key, const char *value) {
    if (strcmp(key, "name") == 0) {
        strcpy(settings.name, value);
    } else if (strcmp(key, "status") == 0) {
        strcpy(settings.status, value);
    } else if (strcmp(key, "master") == 0) {
        strcpy(settings.master, value);
    } else if (strcmp(key, "default_channel") == 0) {
        strcpy(settings.default_channel, value);
    } else if (strcmp(key, "verbose") == 0) {
        settings.verbose = STR_TO_BOOL(value);
    }
}

static void parse_tox_section(const char *key, const char *value) {
    if (strcmp(key, "ipv6") == 0) {
        settings.ipv6 = STR_TO_BOOL(value);
    } else if (strcmp(key, "udp") == 0){
        settings.udp = STR_TO_BOOL(value);
    }
}

static void parse_irc_section(const char *key, const char *value) {
    if (strcmp(key, "server") == 0) {
        strcpy(settings.server, value);
    } else if (strcmp(key, "port") == 0) {
        strcpy(settings.port, value);
    }
}

static int settings_parser(const char *section, const char *key, const char *value, void *UNUSED(config)) {
    SECTION sec = get_section(section);

    switch (sec) {
        case SECTION_BOT:
            parse_bot_section(key, value);
            break;
        case SECTION_TOX:
            parse_tox_section(key, value);
            break;
        case SECTION_IRC:
            parse_irc_section(key, value);
            break;
        case SECTION_UNKNOWN:
            break;
        default:
            break;
    }

    return 1;
}

bool settings_load(char *file){
    off_t size = get_file_size(file);
    if (size == 0) {
        return false;
    }

    if (!ini_browse(settings_parser, &settings, file)) {
        DEBUG("Settings", "Unable to parse %s.", file);
        return false;
    }

    return true;
}
