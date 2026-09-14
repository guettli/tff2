#include "tff_key_codes.h"
#include <unordered_map>
#include <algorithm>
#include <cctype>

namespace tff {

namespace {

struct KeyItem {
    const char* name;
    const char* word;
    KeyCode code;
    const char* short_name;
};

static const KeyItem KEY_ITEMS[] = {
    {"KEY_RESERVED", "reserved", 0, "RESERVED"},
    {"KEY_ESC", "esc", 1, "ESC"},
    {"KEY_1", "1", 2, "1"},
    {"KEY_2", "2", 3, "2"},
    {"KEY_3", "3", 4, "3"},
    {"KEY_4", "4", 5, "4"},
    {"KEY_5", "5", 6, "5"},
    {"KEY_6", "6", 7, "6"},
    {"KEY_7", "7", 8, "7"},
    {"KEY_8", "8", 9, "8"},
    {"KEY_9", "9", 10, "9"},
    {"KEY_0", "0", 11, "0"},
    {"KEY_MINUS", "minus", 12, "MINUS"},
    {"KEY_EQUAL", "equal", 13, "EQUAL"},
    {"KEY_BACKSPACE", "backspace", 14, "BACKSPACE"},
    {"KEY_TAB", "tab", 15, "TAB"},
    {"KEY_Q", "q", 16, "Q"},
    {"KEY_W", "w", 17, "W"},
    {"KEY_E", "e", 18, "E"},
    {"KEY_R", "r", 19, "R"},
    {"KEY_T", "t", 20, "T"},
    {"KEY_Y", "y", 21, "Y"},
    {"KEY_U", "u", 22, "U"},
    {"KEY_I", "i", 23, "I"},
    {"KEY_O", "o", 24, "O"},
    {"KEY_P", "p", 25, "P"},
    {"KEY_LEFTBRACE", "leftbrace", 26, "LEFTBRACE"},
    {"KEY_RIGHTBRACE", "rightbrace", 27, "RIGHTBRACE"},
    {"KEY_ENTER", "enter", 28, "ENTER"},
    {"KEY_LEFTCTRL", "leftctrl", 29, "LEFTCTRL"},
    {"KEY_A", "a", 30, "A"},
    {"KEY_S", "s", 31, "S"},
    {"KEY_D", "d", 32, "D"},
    {"KEY_F", "f", 33, "F"},
    {"KEY_G", "g", 34, "G"},
    {"KEY_H", "h", 35, "H"},
    {"KEY_J", "j", 36, "J"},
    {"KEY_K", "k", 37, "K"},
    {"KEY_L", "l", 38, "L"},
    {"KEY_SEMICOLON", "semicolon", 39, "SEMICOLON"},
    {"KEY_APOSTROPHE", "apostrophe", 40, "APOSTROPHE"},
    {"KEY_GRAVE", "grave", 41, "GRAVE"},
    {"KEY_LEFTSHIFT", "leftshift", 42, "LEFTSHIFT"},
    {"KEY_BACKSLASH", "backslash", 43, "BACKSLASH"},
    {"KEY_Z", "z", 44, "Z"},
    {"KEY_X", "x", 45, "X"},
    {"KEY_C", "c", 46, "C"},
    {"KEY_V", "v", 47, "V"},
    {"KEY_B", "b", 48, "B"},
    {"KEY_N", "n", 49, "N"},
    {"KEY_M", "m", 50, "M"},
    {"KEY_COMMA", "comma", 51, "COMMA"},
    {"KEY_DOT", "dot", 52, "DOT"},
    {"KEY_SLASH", "slash", 53, "SLASH"},
    {"KEY_RIGHTSHIFT", "rightshift", 54, "RIGHTSHIFT"},
    {"KEY_KPASTERISK", "kpasterisk", 55, "KPASTERISK"},
    {"KEY_LEFTALT", "leftalt", 56, "LEFTALT"},
    {"KEY_SPACE", "space", 57, "SPACE"},
    {"KEY_CAPSLOCK", "capslock", 58, "CAPSLOCK"},
    {"KEY_F1", "f1", 59, "F1"},
    {"KEY_F2", "f2", 60, "F2"},
    {"KEY_F3", "f3", 61, "F3"},
    {"KEY_F4", "f4", 62, "F4"},
    {"KEY_F5", "f5", 63, "F5"},
    {"KEY_F6", "f6", 64, "F6"},
    {"KEY_F7", "f7", 65, "F7"},
    {"KEY_F8", "f8", 66, "F8"},
    {"KEY_F9", "f9", 67, "F9"},
    {"KEY_F10", "f10", 68, "F10"},
    {"KEY_NUMLOCK", "numlock", 69, "NUMLOCK"},
    {"KEY_SCROLLLOCK", "scrolllock", 70, "SCROLLLOCK"},
    {"KEY_KP7", "kp7", 71, "KP7"},
    {"KEY_KP8", "kp8", 72, "KP8"},
    {"KEY_KP9", "kp9", 73, "KP9"},
    {"KEY_KPMINUS", "kpminus", 74, "KPMINUS"},
    {"KEY_KP4", "kp4", 75, "KP4"},
    {"KEY_KP5", "kp5", 76, "KP5"},
    {"KEY_KP6", "kp6", 77, "KP6"},
    {"KEY_KPPLUS", "kpplus", 78, "KPPLUS"},
    {"KEY_KP1", "kp1", 79, "KP1"},
    {"KEY_KP2", "kp2", 80, "KP2"},
    {"KEY_KP3", "kp3", 81, "KP3"},
    {"KEY_KP0", "kp0", 82, "KP0"},
    {"KEY_KPDOT", "kpdot", 83, "KPDOT"},
    {"KEY_ZENKAKUHANKAKU", "zenkakuhankaku", 85, "ZENKAKUHANKAKU"},
    {"KEY_102ND", "102nd", 86, "102ND"},
    {"KEY_F11", "f11", 87, "F11"},
    {"KEY_F12", "f12", 88, "F12"},
    {"KEY_RO", "ro", 89, "RO"},
    {"KEY_KATAKANA", "katakana", 90, "KATAKANA"},
    {"KEY_HIRAGANA", "hiragana", 91, "HIRAGANA"},
    {"KEY_HENKAN", "henkan", 92, "HENKAN"},
    {"KEY_KATAKANAHIRAGANA", "katakanahiragana", 93, "KATAKANAHIRAGANA"},
    {"KEY_MUHENKAN", "muhenkan", 94, "MUHENKAN"},
    {"KEY_KPJPCOMMA", "kpjpcomma", 95, "KPJPCOMMA"},
    {"KEY_KPENTER", "kpenter", 96, "KPENTER"},
    {"KEY_RIGHTCTRL", "rightctrl", 97, "RIGHTCTRL"},
    {"KEY_KPSLASH", "kpslash", 98, "KPSLASH"},
    {"KEY_SYSRQ", "sysrq", 99, "SYSRQ"},
    {"KEY_RIGHTALT", "rightalt", 100, "RIGHTALT"},
    {"KEY_LINEFEED", "linefeed", 101, "LINEFEED"},
    {"KEY_HOME", "home", 102, "HOME"},
    {"KEY_UP", "up", 103, "UP"},
    {"KEY_PAGEUP", "pageup", 104, "PAGEUP"},
    {"KEY_LEFT", "left", 105, "LEFT"},
    {"KEY_RIGHT", "right", 106, "RIGHT"},
    {"KEY_END", "end", 107, "END"},
    {"KEY_DOWN", "down", 108, "DOWN"},
    {"KEY_PAGEDOWN", "pagedown", 109, "PAGEDOWN"},
    {"KEY_INSERT", "insert", 110, "INSERT"},
    {"KEY_DELETE", "delete", 111, "DELETE"},
    {"KEY_MACRO", "macro", 112, "MACRO"},
    {"KEY_MUTE", "mute", 113, "MUTE"},
    {"KEY_VOLUMEDOWN", "volumedown", 114, "VOLUMEDOWN"},
    {"KEY_VOLUMEUP", "volumeup", 115, "VOLUMEUP"},
    {"KEY_POWER", "power", 116, "POWER"},
    {"KEY_KPEQUAL", "kpequal", 117, "KPEQUAL"},
    {"KEY_KPPLUSMINUS", "kpplusminus", 118, "KPPLUSMINUS"},
    {"KEY_PAUSE", "pause", 119, "PAUSE"},
    {"KEY_SCALE", "scale", 120, "SCALE"},
    {"KEY_KPCOMMA", "kpcomma", 121, "KPCOMMA"},
    {"KEY_HANGEUL", "hangeul", 122, "HANGEUL"},
    {"KEY_HANJA", "hanja", 123, "HANJA"},
    {"KEY_YEN", "yen", 124, "YEN"},
    {"KEY_LEFTMETA", "leftmeta", 125, "LEFTMETA"},
    {"KEY_RIGHTMETA", "rightmeta", 126, "RIGHTMETA"},
    {"KEY_COMPOSE", "compose", 127, "COMPOSE"},
    {"KEY_STOP", "stop", 128, "STOP"},
    {"KEY_AGAIN", "again", 129, "AGAIN"},
    {"KEY_PROPS", "props", 130, "PROPS"},
    {"KEY_UNDO", "undo", 131, "UNDO"},
    {"KEY_FRONT", "front", 132, "FRONT"},
    {"KEY_COPY", "copy", 133, "COPY"},
    {"KEY_OPEN", "open", 134, "OPEN"},
    {"KEY_PASTE", "paste", 135, "PASTE"},
    {"KEY_FIND", "find", 136, "FIND"},
    {"KEY_CUT", "cut", 137, "CUT"},
    {"KEY_HELP", "help", 138, "HELP"},
    {"KEY_MENU", "menu", 139, "MENU"},
    {"KEY_CALC", "calc", 140, "CALC"},
    {"KEY_SETUP", "setup", 141, "SETUP"},
    {"KEY_SLEEP", "sleep", 142, "SLEEP"},
    {"KEY_WAKEUP", "wakeup", 143, "WAKEUP"},
    {"KEY_FILE", "file", 144, "FILE"},
    {"KEY_SENDFILE", "sendfile", 145, "SENDFILE"},
    {"KEY_DELETEFILE", "deletefile", 146, "DELETEFILE"},
    {"KEY_XFER", "xfer", 147, "XFER"},
    {"KEY_PROG1", "prog1", 148, "PROG1"},
    {"KEY_PROG2", "prog2", 149, "PROG2"},
    {"KEY_WWW", "www", 150, "WWW"},
    {"KEY_MSDOS", "msdos", 151, "MSDOS"},
    {"KEY_COFFEE", "coffee", 152, "COFFEE"},
    {"KEY_ROTATE_DISPLAY", "rotate_display", 153, "ROTATE_DISPLAY"},
    {"KEY_CYCLEWINDOWS", "cyclewindows", 154, "CYCLEWINDOWS"},
    {"KEY_MAIL", "mail", 155, "MAIL"},
    {"KEY_BOOKMARKS", "bookmarks", 156, "BOOKMARKS"},
    {"KEY_COMPUTER", "computer", 157, "COMPUTER"},
    {"KEY_BACK", "back", 158, "BACK"},
    {"KEY_FORWARD", "forward", 159, "FORWARD"},
    {"KEY_CLOSECD", "closecd", 160, "CLOSECD"},
    {"KEY_EJECTCD", "ejectcd", 161, "EJECTCD"},
    {"KEY_EJECTCLOSECD", "ejectclosecd", 162, "EJECTCLOSECD"},
    {"KEY_NEXTSONG", "nextsong", 163, "NEXTSONG"},
    {"KEY_PLAYPAUSE", "playpause", 164, "PLAYPAUSE"},
    {"KEY_PREVIOUSSONG", "previoussong", 165, "PREVIOUSSONG"},
    {"KEY_STOPCD", "stopcd", 166, "STOPCD"},
    {"KEY_RECORD", "record", 167, "RECORD"},
    {"KEY_REWIND", "rewind", 168, "REWIND"},
    {"KEY_PHONE", "phone", 169, "PHONE"},
    {"KEY_ISO", "iso", 170, "ISO"},
    {"KEY_CONFIG", "config", 171, "CONFIG"},
    {"KEY_HOMEPAGE", "homepage", 172, "HOMEPAGE"},
    {"KEY_REFRESH", "refresh", 173, "REFRESH"},
    {"KEY_EXIT", "exit", 174, "EXIT"},
    {"KEY_MOVE", "move", 175, "MOVE"},
    {"KEY_EDIT", "edit", 176, "EDIT"},
    {"KEY_SCROLLUP", "scrollup", 177, "SCROLLUP"},
    {"KEY_SCROLLDOWN", "scrolldown", 178, "SCROLLDOWN"},
    {"KEY_KPLEFTPAREN", "kpleftparen", 179, "KPLEFTPAREN"},
    {"KEY_KPRIGHTPAREN", "kprightparen", 180, "KPRIGHTPAREN"},
    {"KEY_NEW", "new", 181, "NEW"},
    {"KEY_REDO", "redo", 182, "REDO"},
    {"KEY_F13", "f13", 183, "F13"},
    {"KEY_F14", "f14", 184, "F14"},
    {"KEY_F15", "f15", 185, "F15"},
    {"KEY_F16", "f16", 186, "F16"},
    {"KEY_F17", "f17", 187, "F17"},
    {"KEY_F18", "f18", 188, "F18"},
    {"KEY_F19", "f19", 189, "F19"},
    {"KEY_F20", "f20", 190, "F20"},
    {"KEY_F21", "f21", 191, "F21"},
    {"KEY_F22", "f22", 192, "F22"},
    {"KEY_F23", "f23", 193, "F23"},
    {"KEY_F24", "f24", 194, "F24"},
    {"KEY_PLAYCD", "playcd", 200, "PLAYCD"},
    {"KEY_PAUSECD", "pausecd", 201, "PAUSECD"},
    {"KEY_PROG3", "prog3", 202, "PROG3"},
    {"KEY_PROG4", "prog4", 203, "PROG4"},
    {"KEY_ALL_APPLICATIONS", "all_applications", 204, "ALL_APPLICATIONS"},
    {"KEY_SUSPEND", "suspend", 205, "SUSPEND"},
    {"KEY_CLOSE", "close", 206, "CLOSE"},
    {"KEY_PLAY", "play", 207, "PLAY"},
    {"KEY_FASTFORWARD", "fastforward", 208, "FASTFORWARD"},
    {"KEY_BASSBOOST", "bassboost", 209, "BASSBOOST"},
    {"KEY_PRINT", "print", 210, "PRINT"},
    {"KEY_HP", "hp", 211, "HP"},
    {"KEY_CAMERA", "camera", 212, "CAMERA"},
    {"KEY_SOUND", "sound", 213, "SOUND"},
    {"KEY_QUESTION", "question", 214, "QUESTION"},
    {"KEY_EMAIL", "email", 215, "EMAIL"},
    {"KEY_CHAT", "chat", 216, "CHAT"},
    {"KEY_SEARCH", "search", 217, "SEARCH"},
    {"KEY_CONNECT", "connect", 218, "CONNECT"},
    {"KEY_FINANCE", "finance", 219, "FINANCE"},
    {"KEY_SPORT", "sport", 220, "SPORT"},
    {"KEY_SHOP", "shop", 221, "SHOP"},
    {"KEY_ALTERASE", "alterase", 222, "ALTERASE"},
    {"KEY_CANCEL", "cancel", 223, "CANCEL"},
    {"KEY_BRIGHTNESSDOWN", "brightnessdown", 224, "BRIGHTNESSDOWN"},
    {"KEY_BRIGHTNESSUP", "brightnessup", 225, "BRIGHTNESSUP"},
    {"KEY_MEDIA", "media", 226, "MEDIA"},
    {"KEY_SWITCHVIDEOMODE", "switchvideomode", 227, "SWITCHVIDEOMODE"},
    {"KEY_KBDILLUMTOGGLE", "kbdillumtoggle", 228, "KBDILLUMTOGGLE"},
    {"KEY_KBDILLUMDOWN", "kbdillumdown", 229, "KBDILLUMDOWN"},
    {"KEY_KBDILLUMUP", "kbdillumup", 230, "KBDILLUMUP"},
    {"KEY_SEND", "send", 231, "SEND"},
    {"KEY_REPLY", "reply", 232, "REPLY"},
    {"KEY_FORWARDMAIL", "forwardmail", 233, "FORWARDMAIL"},
    {"KEY_SAVE", "save", 234, "SAVE"},
    {"KEY_DOCUMENTS", "documents", 235, "DOCUMENTS"},
    {"KEY_BATTERY", "battery", 236, "BATTERY"},
    {"KEY_BLUETOOTH", "bluetooth", 237, "BLUETOOTH"},
    {"KEY_WLAN", "wlan", 238, "WLAN"},
    {"KEY_UWB", "uwb", 239, "UWB"},
    {"KEY_UNKNOWN", "unknown", 240, "UNKNOWN"},
    {"KEY_VIDEO_NEXT", "video_next", 241, "VIDEO_NEXT"},
    {"KEY_VIDEO_PREV", "video_prev", 242, "VIDEO_PREV"},
    {"KEY_BRIGHTNESS_CYCLE", "brightness_cycle", 243, "BRIGHTNESS_CYCLE"},
    {"KEY_BRIGHTNESS_AUTO", "brightness_auto", 244, "BRIGHTNESS_AUTO"},
    {"KEY_DISPLAY_OFF", "display_off", 245, "DISPLAY_OFF"},
    {"KEY_WWAN", "wwan", 246, "WWAN"},
    {"KEY_RFKILL", "rfkill", 247, "RFKILL"},
    {"KEY_MICMUTE", "micmute", 248, "MICMUTE"},
    {"KEY_OK", "ok", 0, "OK"},
    {"KEY_SELECT", "select", 0, "SELECT"},
    {"KEY_GOTO", "goto", 0, "GOTO"},
    {"KEY_CLEAR", "clear", 0, "CLEAR"},
    {"KEY_POWER2", "power2", 0, "POWER2"},
    {"KEY_OPTION", "option", 0, "OPTION"},
    {"KEY_INFO", "info", 0, "INFO"},
    {"KEY_TIME", "time", 0, "TIME"},
    {"KEY_VENDOR", "vendor", 0, "VENDOR"},
    {"KEY_ARCHIVE", "archive", 0, "ARCHIVE"},
    {"KEY_PROGRAM", "program", 0, "PROGRAM"},
    {"KEY_CHANNEL", "channel", 0, "CHANNEL"},
    {"KEY_FAVORITES", "favorites", 0, "FAVORITES"},
    {"KEY_EPG", "epg", 0, "EPG"},
    {"KEY_PVR", "pvr", 0, "PVR"},
    {"KEY_MHP", "mhp", 0, "MHP"},
    {"KEY_LANGUAGE", "language", 0, "LANGUAGE"},
    {"KEY_TITLE", "title", 0, "TITLE"},
    {"KEY_SUBTITLE", "subtitle", 0, "SUBTITLE"},
    {"KEY_ANGLE", "angle", 0, "ANGLE"},
    {"KEY_FULL_SCREEN", "full_screen", 0, "FULL_SCREEN"},
    {"KEY_MODE", "mode", 0, "MODE"},
    {"KEY_KEYBOARD", "keyboard", 0, "KEYBOARD"},
    {"KEY_ASPECT_RATIO", "aspect_ratio", 0, "ASPECT_RATIO"},
    {"KEY_PC", "pc", 0, "PC"},
    {"KEY_TV", "tv", 0, "TV"},
    {"KEY_TV2", "tv2", 0, "TV2"},
    {"KEY_VCR", "vcr", 0, "VCR"},
    {"KEY_VCR2", "vcr2", 0, "VCR2"},
    {"KEY_SAT", "sat", 0, "SAT"},
    {"KEY_SAT2", "sat2", 0, "SAT2"},
    {"KEY_CD", "cd", 0, "CD"},
    {"KEY_TAPE", "tape", 0, "TAPE"},
    {"KEY_RADIO", "radio", 0, "RADIO"},
    {"KEY_TUNER", "tuner", 0, "TUNER"},
    {"KEY_PLAYER", "player", 0, "PLAYER"},
    {"KEY_TEXT", "text", 0, "TEXT"},
    {"KEY_DVD", "dvd", 0, "DVD"},
    {"KEY_AUX", "aux", 0, "AUX"},
    {"KEY_MP3", "mp3", 0, "MP3"},
    {"KEY_AUDIO", "audio", 0, "AUDIO"},
    {"KEY_VIDEO", "video", 0, "VIDEO"},
    {"KEY_DIRECTORY", "directory", 0, "DIRECTORY"},
    {"KEY_LIST", "list", 0, "LIST"},
    {"KEY_MEMO", "memo", 0, "MEMO"},
    {"KEY_CALENDAR", "calendar", 0, "CALENDAR"},
    {"KEY_RED", "red", 0, "RED"},
    {"KEY_GREEN", "green", 0, "GREEN"},
    {"KEY_YELLOW", "yellow", 0, "YELLOW"},
    {"KEY_BLUE", "blue", 0, "BLUE"},
    {"KEY_CHANNELUP", "channelup", 0, "CHANNELUP"},
    {"KEY_CHANNELDOWN", "channeldown", 0, "CHANNELDOWN"},
    {"KEY_FIRST", "first", 0, "FIRST"},
    {"KEY_LAST", "last", 0, "LAST"},
    {"KEY_AB", "ab", 0, "AB"},
    {"KEY_NEXT", "next", 0, "NEXT"},
    {"KEY_RESTART", "restart", 0, "RESTART"},
    {"KEY_SLOW", "slow", 0, "SLOW"},
    {"KEY_SHUFFLE", "shuffle", 0, "SHUFFLE"},
    {"KEY_BREAK", "break", 0, "BREAK"},
    {"KEY_PREVIOUS", "previous", 0, "PREVIOUS"},
    {"KEY_DIGITS", "digits", 0, "DIGITS"},
    {"KEY_TEEN", "teen", 0, "TEEN"},
    {"KEY_TWEN", "twen", 0, "TWEN"},
    {"KEY_VIDEOPHONE", "videophone", 0, "VIDEOPHONE"},
    {"KEY_GAMES", "games", 0, "GAMES"},
    {"KEY_ZOOMIN", "zoomin", 0, "ZOOMIN"},
    {"KEY_ZOOMOUT", "zoomout", 0, "ZOOMOUT"},
    {"KEY_ZOOMRESET", "zoomreset", 0, "ZOOMRESET"},
    {"KEY_WORDPROCESSOR", "wordprocessor", 0, "WORDPROCESSOR"},
    {"KEY_EDITOR", "editor", 0, "EDITOR"},
    {"KEY_SPREADSHEET", "spreadsheet", 0, "SPREADSHEET"},
    {"KEY_GRAPHICSEDITOR", "graphicseditor", 0, "GRAPHICSEDITOR"},
    {"KEY_PRESENTATION", "presentation", 0, "PRESENTATION"},
    {"KEY_DATABASE", "database", 0, "DATABASE"},
    {"KEY_NEWS", "news", 0, "NEWS"},
    {"KEY_VOICEMAIL", "voicemail", 0, "VOICEMAIL"},
    {"KEY_ADDRESSBOOK", "addressbook", 0, "ADDRESSBOOK"},
    {"KEY_MESSENGER", "messenger", 0, "MESSENGER"},
    {"KEY_DISPLAYTOGGLE", "displaytoggle", 0, "DISPLAYTOGGLE"},
    {"KEY_SPELLCHECK", "spellcheck", 0, "SPELLCHECK"},
    {"KEY_LOGOFF", "logoff", 0, "LOGOFF"},
    {"KEY_DOLLAR", "dollar", 0, "DOLLAR"},
    {"KEY_EURO", "euro", 0, "EURO"},
    {"KEY_FRAMEBACK", "frameback", 0, "FRAMEBACK"},
    {"KEY_FRAMEFORWARD", "frameforward", 0, "FRAMEFORWARD"},
    {"KEY_CONTEXT_MENU", "context_menu", 0, "CONTEXT_MENU"},
    {"KEY_MEDIA_REPEAT", "media_repeat", 0, "MEDIA_REPEAT"},
    {"KEY_10CHANNELSUP", "10channelsup", 0, "10CHANNELSUP"},
    {"KEY_10CHANNELSDOWN", "10channelsdown", 0, "10CHANNELSDOWN"},
    {"KEY_IMAGES", "images", 0, "IMAGES"},
    {"KEY_NOTIFICATION_CENTER", "notification_center", 0, "NOTIFICATION_CENTER"},
    {"KEY_PICKUP_PHONE", "pickup_phone", 0, "PICKUP_PHONE"},
    {"KEY_HANGUP_PHONE", "hangup_phone", 0, "HANGUP_PHONE"},
    {"KEY_LINK_PHONE", "link_phone", 0, "LINK_PHONE"},
    {"KEY_DEL_EOL", "del_eol", 0, "DEL_EOL"},
    {"KEY_DEL_EOS", "del_eos", 0, "DEL_EOS"},
    {"KEY_INS_LINE", "ins_line", 0, "INS_LINE"},
    {"KEY_DEL_LINE", "del_line", 0, "DEL_LINE"},
    {"KEY_FN", "fn", 0, "FN"},
    {"KEY_FN_ESC", "fn_esc", 0, "FN_ESC"},
    {"KEY_FN_F1", "fn_f1", 0, "FN_F1"},
    {"KEY_FN_F2", "fn_f2", 0, "FN_F2"},
    {"KEY_FN_F3", "fn_f3", 0, "FN_F3"},
    {"KEY_FN_F4", "fn_f4", 0, "FN_F4"},
    {"KEY_FN_F5", "fn_f5", 0, "FN_F5"},
    {"KEY_FN_F6", "fn_f6", 0, "FN_F6"},
    {"KEY_FN_F7", "fn_f7", 0, "FN_F7"},
    {"KEY_FN_F8", "fn_f8", 0, "FN_F8"},
    {"KEY_FN_F9", "fn_f9", 0, "FN_F9"},
    {"KEY_FN_F10", "fn_f10", 0, "FN_F10"},
    {"KEY_FN_F11", "fn_f11", 0, "FN_F11"},
    {"KEY_FN_F12", "fn_f12", 0, "FN_F12"},
    {"KEY_FN_1", "fn_1", 0, "FN_1"},
    {"KEY_FN_2", "fn_2", 0, "FN_2"},
    {"KEY_FN_D", "fn_d", 0, "FN_D"},
    {"KEY_FN_E", "fn_e", 0, "FN_E"},
    {"KEY_FN_F", "fn_f", 0, "FN_F"},
    {"KEY_FN_S", "fn_s", 0, "FN_S"},
    {"KEY_FN_B", "fn_b", 0, "FN_B"},
    {"KEY_FN_RIGHT_SHIFT", "fn_right_shift", 0, "FN_RIGHT_SHIFT"},
    {"KEY_BRL_DOT1", "brl_dot1", 0, "BRL_DOT1"},
    {"KEY_BRL_DOT2", "brl_dot2", 0, "BRL_DOT2"},
    {"KEY_BRL_DOT3", "brl_dot3", 0, "BRL_DOT3"},
    {"KEY_BRL_DOT4", "brl_dot4", 0, "BRL_DOT4"},
    {"KEY_BRL_DOT5", "brl_dot5", 0, "BRL_DOT5"},
    {"KEY_BRL_DOT6", "brl_dot6", 0, "BRL_DOT6"},
    {"KEY_BRL_DOT7", "brl_dot7", 0, "BRL_DOT7"},
    {"KEY_BRL_DOT8", "brl_dot8", 0, "BRL_DOT8"},
    {"KEY_BRL_DOT9", "brl_dot9", 0, "BRL_DOT9"},
    {"KEY_BRL_DOT10", "brl_dot10", 0, "BRL_DOT10"},
    {"KEY_NUMERIC_0", "numeric_0", 0, "NUMERIC_0"},
    {"KEY_NUMERIC_1", "numeric_1", 0, "NUMERIC_1"},
    {"KEY_NUMERIC_2", "numeric_2", 0, "NUMERIC_2"},
    {"KEY_NUMERIC_3", "numeric_3", 0, "NUMERIC_3"},
    {"KEY_NUMERIC_4", "numeric_4", 0, "NUMERIC_4"},
    {"KEY_NUMERIC_5", "numeric_5", 0, "NUMERIC_5"},
    {"KEY_NUMERIC_6", "numeric_6", 0, "NUMERIC_6"},
    {"KEY_NUMERIC_7", "numeric_7", 0, "NUMERIC_7"},
    {"KEY_NUMERIC_8", "numeric_8", 0, "NUMERIC_8"},
    {"KEY_NUMERIC_9", "numeric_9", 0, "NUMERIC_9"},
    {"KEY_NUMERIC_STAR", "numeric_star", 0, "NUMERIC_STAR"},
    {"KEY_NUMERIC_POUND", "numeric_pound", 0, "NUMERIC_POUND"},
    {"KEY_NUMERIC_A", "numeric_a", 0, "NUMERIC_A"},
    {"KEY_NUMERIC_B", "numeric_b", 0, "NUMERIC_B"},
    {"KEY_NUMERIC_C", "numeric_c", 0, "NUMERIC_C"},
    {"KEY_NUMERIC_D", "numeric_d", 0, "NUMERIC_D"},
    {"KEY_CAMERA_FOCUS", "camera_focus", 0, "CAMERA_FOCUS"},
    {"KEY_WPS_BUTTON", "wps_button", 0, "WPS_BUTTON"},
    {"KEY_TOUCHPAD_TOGGLE", "touchpad_toggle", 0, "TOUCHPAD_TOGGLE"},
    {"KEY_TOUCHPAD_ON", "touchpad_on", 0, "TOUCHPAD_ON"},
    {"KEY_TOUCHPAD_OFF", "touchpad_off", 0, "TOUCHPAD_OFF"},
    {"KEY_CAMERA_ZOOMIN", "camera_zoomin", 0, "CAMERA_ZOOMIN"},
    {"KEY_CAMERA_ZOOMOUT", "camera_zoomout", 0, "CAMERA_ZOOMOUT"},
    {"KEY_CAMERA_UP", "camera_up", 0, "CAMERA_UP"},
    {"KEY_CAMERA_DOWN", "camera_down", 0, "CAMERA_DOWN"},
    {"KEY_CAMERA_LEFT", "camera_left", 0, "CAMERA_LEFT"},
    {"KEY_CAMERA_RIGHT", "camera_right", 0, "CAMERA_RIGHT"},
    {"KEY_ATTENDANT_ON", "attendant_on", 0, "ATTENDANT_ON"},
    {"KEY_ATTENDANT_OFF", "attendant_off", 0, "ATTENDANT_OFF"},
    {"KEY_ATTENDANT_TOGGLE", "attendant_toggle", 0, "ATTENDANT_TOGGLE"},
    {"KEY_LIGHTS_TOGGLE", "lights_toggle", 0, "LIGHTS_TOGGLE"},
    {"KEY_ALS_TOGGLE", "als_toggle", 0, "ALS_TOGGLE"},
    {"KEY_ROTATE_LOCK_TOGGLE", "rotate_lock_toggle", 0, "ROTATE_LOCK_TOGGLE"},
    {"KEY_REFRESH_RATE_TOGGLE", "refresh_rate_toggle", 0, "REFRESH_RATE_TOGGLE"},
    {"KEY_BUTTONCONFIG", "buttonconfig", 0, "BUTTONCONFIG"},
    {"KEY_TASKMANAGER", "taskmanager", 0, "TASKMANAGER"},
    {"KEY_JOURNAL", "journal", 0, "JOURNAL"},
    {"KEY_CONTROLPANEL", "controlpanel", 0, "CONTROLPANEL"},
    {"KEY_APPSELECT", "appselect", 0, "APPSELECT"},
    {"KEY_SCREENSAVER", "screensaver", 0, "SCREENSAVER"},
    {"KEY_VOICECOMMAND", "voicecommand", 0, "VOICECOMMAND"},
    {"KEY_ASSISTANT", "assistant", 0, "ASSISTANT"},
    {"KEY_KBD_LAYOUT_NEXT", "kbd_layout_next", 0, "KBD_LAYOUT_NEXT"},
    {"KEY_EMOJI_PICKER", "emoji_picker", 0, "EMOJI_PICKER"},
    {"KEY_DICTATE", "dictate", 0, "DICTATE"},
    {"KEY_CAMERA_ACCESS_ENABLE", "camera_access_enable", 0, "CAMERA_ACCESS_ENABLE"},
    {"KEY_CAMERA_ACCESS_DISABLE", "camera_access_disable", 0, "CAMERA_ACCESS_DISABLE"},
    {"KEY_CAMERA_ACCESS_TOGGLE", "camera_access_toggle", 0, "CAMERA_ACCESS_TOGGLE"},
    {"KEY_ACCESSIBILITY", "accessibility", 0, "ACCESSIBILITY"},
    {"KEY_DO_NOT_DISTURB", "do_not_disturb", 0, "DO_NOT_DISTURB"},
    {"KEY_BRIGHTNESS_MIN", "brightness_min", 0, "BRIGHTNESS_MIN"},
    {"KEY_BRIGHTNESS_MAX", "brightness_max", 0, "BRIGHTNESS_MAX"},
    {"KEY_EPRIVACY_SCREEN_ON", "eprivacy_screen_on", 0, "EPRIVACY_SCREEN_ON"},
    {"KEY_EPRIVACY_SCREEN_OFF", "eprivacy_screen_off", 0, "EPRIVACY_SCREEN_OFF"},
    {"KEY_ACTION_ON_SELECTION", "action_on_selection", 0, "ACTION_ON_SELECTION"},
    {"KEY_CONTEXTUAL_INSERT", "contextual_insert", 0, "CONTEXTUAL_INSERT"},
    {"KEY_CONTEXTUAL_QUERY", "contextual_query", 0, "CONTEXTUAL_QUERY"},
    {"KEY_KBDINPUTASSIST_PREV", "kbdinputassist_prev", 0, "KBDINPUTASSIST_PREV"},
    {"KEY_KBDINPUTASSIST_NEXT", "kbdinputassist_next", 0, "KBDINPUTASSIST_NEXT"},
    {"KEY_KBDINPUTASSIST_PREVGROUP", "kbdinputassist_prevgroup", 0, "KBDINPUTASSIST_PREVGROUP"},
    {"KEY_KBDINPUTASSIST_NEXTGROUP", "kbdinputassist_nextgroup", 0, "KBDINPUTASSIST_NEXTGROUP"},
    {"KEY_KBDINPUTASSIST_ACCEPT", "kbdinputassist_accept", 0, "KBDINPUTASSIST_ACCEPT"},
    {"KEY_KBDINPUTASSIST_CANCEL", "kbdinputassist_cancel", 0, "KBDINPUTASSIST_CANCEL"},
    {"KEY_RIGHT_UP", "right_up", 0, "RIGHT_UP"},
    {"KEY_RIGHT_DOWN", "right_down", 0, "RIGHT_DOWN"},
    {"KEY_LEFT_UP", "left_up", 0, "LEFT_UP"},
    {"KEY_LEFT_DOWN", "left_down", 0, "LEFT_DOWN"},
    {"KEY_ROOT_MENU", "root_menu", 0, "ROOT_MENU"},
    {"KEY_MEDIA_TOP_MENU", "media_top_menu", 0, "MEDIA_TOP_MENU"},
    {"KEY_NUMERIC_11", "numeric_11", 0, "NUMERIC_11"},
    {"KEY_NUMERIC_12", "numeric_12", 0, "NUMERIC_12"},
    {"KEY_AUDIO_DESC", "audio_desc", 0, "AUDIO_DESC"},
    {"KEY_3D_MODE", "3d_mode", 0, "3D_MODE"},
    {"KEY_NEXT_FAVORITE", "next_favorite", 0, "NEXT_FAVORITE"},
    {"KEY_STOP_RECORD", "stop_record", 0, "STOP_RECORD"},
    {"KEY_PAUSE_RECORD", "pause_record", 0, "PAUSE_RECORD"},
    {"KEY_VOD", "vod", 0, "VOD"},
    {"KEY_UNMUTE", "unmute", 0, "UNMUTE"},
    {"KEY_FASTREVERSE", "fastreverse", 0, "FASTREVERSE"},
    {"KEY_SLOWREVERSE", "slowreverse", 0, "SLOWREVERSE"},
    {"KEY_DATA", "data", 0, "DATA"},
    {"KEY_ONSCREEN_KEYBOARD", "onscreen_keyboard", 0, "ONSCREEN_KEYBOARD"},
    {"KEY_PRIVACY_SCREEN_TOGGLE", "privacy_screen_toggle", 0, "PRIVACY_SCREEN_TOGGLE"},
    {"KEY_SELECTIVE_SCREENSHOT", "selective_screenshot", 0, "SELECTIVE_SCREENSHOT"},
    {"KEY_NEXT_ELEMENT", "next_element", 0, "NEXT_ELEMENT"},
    {"KEY_PREVIOUS_ELEMENT", "previous_element", 0, "PREVIOUS_ELEMENT"},
    {"KEY_AUTOPILOT_ENGAGE_TOGGLE", "autopilot_engage_toggle", 0, "AUTOPILOT_ENGAGE_TOGGLE"},
    {"KEY_MARK_WAYPOINT", "mark_waypoint", 0, "MARK_WAYPOINT"},
    {"KEY_SOS", "sos", 0, "SOS"},
    {"KEY_NAV_CHART", "nav_chart", 0, "NAV_CHART"},
    {"KEY_FISHING_CHART", "fishing_chart", 0, "FISHING_CHART"},
    {"KEY_SINGLE_RANGE_RADAR", "single_range_radar", 0, "SINGLE_RANGE_RADAR"},
    {"KEY_DUAL_RANGE_RADAR", "dual_range_radar", 0, "DUAL_RANGE_RADAR"},
    {"KEY_RADAR_OVERLAY", "radar_overlay", 0, "RADAR_OVERLAY"},
    {"KEY_TRADITIONAL_SONAR", "traditional_sonar", 0, "TRADITIONAL_SONAR"},
    {"KEY_CLEARVU_SONAR", "clearvu_sonar", 0, "CLEARVU_SONAR"},
    {"KEY_SIDEVU_SONAR", "sidevu_sonar", 0, "SIDEVU_SONAR"},
    {"KEY_NAV_INFO", "nav_info", 0, "NAV_INFO"},
    {"KEY_BRIGHTNESS_MENU", "brightness_menu", 0, "BRIGHTNESS_MENU"},
    {"KEY_MACRO1", "macro1", 0, "MACRO1"},
    {"KEY_MACRO2", "macro2", 0, "MACRO2"},
    {"KEY_MACRO3", "macro3", 0, "MACRO3"},
    {"KEY_MACRO4", "macro4", 0, "MACRO4"},
    {"KEY_MACRO5", "macro5", 0, "MACRO5"},
    {"KEY_MACRO6", "macro6", 0, "MACRO6"},
    {"KEY_MACRO7", "macro7", 0, "MACRO7"},
    {"KEY_MACRO8", "macro8", 0, "MACRO8"},
    {"KEY_MACRO9", "macro9", 0, "MACRO9"},
    {"KEY_MACRO10", "macro10", 0, "MACRO10"},
    {"KEY_MACRO11", "macro11", 0, "MACRO11"},
    {"KEY_MACRO12", "macro12", 0, "MACRO12"},
    {"KEY_MACRO13", "macro13", 0, "MACRO13"},
    {"KEY_MACRO14", "macro14", 0, "MACRO14"},
    {"KEY_MACRO15", "macro15", 0, "MACRO15"},
    {"KEY_MACRO16", "macro16", 0, "MACRO16"},
    {"KEY_MACRO17", "macro17", 0, "MACRO17"},
    {"KEY_MACRO18", "macro18", 0, "MACRO18"},
    {"KEY_MACRO19", "macro19", 0, "MACRO19"},
    {"KEY_MACRO20", "macro20", 0, "MACRO20"},
    {"KEY_MACRO21", "macro21", 0, "MACRO21"},
    {"KEY_MACRO22", "macro22", 0, "MACRO22"},
    {"KEY_MACRO23", "macro23", 0, "MACRO23"},
    {"KEY_MACRO24", "macro24", 0, "MACRO24"},
    {"KEY_MACRO25", "macro25", 0, "MACRO25"},
    {"KEY_MACRO26", "macro26", 0, "MACRO26"},
    {"KEY_MACRO27", "macro27", 0, "MACRO27"},
    {"KEY_MACRO28", "macro28", 0, "MACRO28"},
    {"KEY_MACRO29", "macro29", 0, "MACRO29"},
    {"KEY_MACRO30", "macro30", 0, "MACRO30"},
    {"KEY_MACRO_RECORD_START", "macro_record_start", 0, "MACRO_RECORD_START"},
    {"KEY_MACRO_RECORD_STOP", "macro_record_stop", 0, "MACRO_RECORD_STOP"},
    {"KEY_MACRO_PRESET_CYCLE", "macro_preset_cycle", 0, "MACRO_PRESET_CYCLE"},
    {"KEY_MACRO_PRESET1", "macro_preset1", 0, "MACRO_PRESET1"},
    {"KEY_MACRO_PRESET2", "macro_preset2", 0, "MACRO_PRESET2"},
    {"KEY_MACRO_PRESET3", "macro_preset3", 0, "MACRO_PRESET3"},
    {"KEY_KBD_LCD_MENU1", "kbd_lcd_menu1", 0, "KBD_LCD_MENU1"},
    {"KEY_KBD_LCD_MENU2", "kbd_lcd_menu2", 0, "KBD_LCD_MENU2"},
    {"KEY_KBD_LCD_MENU3", "kbd_lcd_menu3", 0, "KBD_LCD_MENU3"},
    {"KEY_KBD_LCD_MENU4", "kbd_lcd_menu4", 0, "KBD_LCD_MENU4"},
    {"KEY_KBD_LCD_MENU5", "kbd_lcd_menu5", 0, "KBD_LCD_MENU5"},
    {"KEY_PERFORMANCE", "performance", 0, "PERFORMANCE"},
    {"KEY_MAX", "max", 0, "MAX"},
};

struct KeyTable {
    std::unordered_map<std::string, KeyCode> word_to_code;
    std::unordered_map<KeyCode, std::string> code_to_word;
    std::unordered_map<KeyCode, std::string> code_to_short;
    std::unordered_map<std::string, KeyCode> name_to_code;

    KeyTable() {
        for (const auto& item : KEY_ITEMS) {
            name_to_code[item.name] = item.code;
            word_to_code[item.word] = item.code;
            if (code_to_word.find(item.code) == code_to_word.end()) {
                code_to_word[item.code] = item.word;
                code_to_short[item.code] = item.short_name;
            }
        }
    }
};
const KeyTable& getTable() {
    static KeyTable table;
    return table;
}

} // namespace

bool wordToKeyCode(const std::string& word, KeyCode& out_code, std::string& err_msg) {
    if (word.empty()) {
        err_msg = "empty word";
        return false;
    }
    for (char c : word) {
        if (std::isupper(static_cast<unsigned char>(c))) {
            err_msg = "only lower case characters are allowed";
            return false;
        }
    }
    const auto& table = getTable();
    auto it = table.word_to_code.find(word);
    if (it != table.word_to_code.end()) {
        out_code = it->second;
        return true;
    }
    err_msg = "failed to get key \"" + word + "\": unknown key";
    return false;
}

std::string keyCodeToWord(KeyCode code) {
    const auto& table = getTable();
    auto it = table.code_to_word.find(code);
    if (it != table.code_to_word.end()) {
        return it->second;
    }
    return "unknown";
}

std::string keyCodeToShortName(KeyCode code) {
    const auto& table = getTable();
    auto it = table.code_to_short.find(code);
    if (it != table.code_to_short.end()) {
        return it->second;
    }
    return "UNKNOWN_" + std::to_string(code);
}

std::string codeName(uint16_t type, uint16_t code) {
    if (type == EV_SYN) {
        if (code == SYN_REPORT) return "SYN_REPORT";
        return "SYN_" + std::to_string(code);
    }
    if (type == EV_MSC) {
        if (code == MSC_SCAN) return "MSC_SCAN";
        return "MSC_" + std::to_string(code);
    }
    if (type == EV_KEY) {
        return "KEY_" + keyCodeToShortName(code);
    }
    return "CODE_" + std::to_string(code);
}

std::string typeName(uint16_t type) {
    if (type == EV_KEY) return "EV_KEY";
    if (type == EV_SYN) return "EV_SYN";
    if (type == EV_MSC) return "EV_MSC";
    return "EV_" + std::to_string(type);
}

bool parseTypeName(const std::string& name, uint16_t& out_type) {
    if (name == "EV_KEY") { out_type = EV_KEY; return true; }
    if (name == "EV_SYN") { out_type = EV_SYN; return true; }
    if (name == "EV_MSC") { out_type = EV_MSC; return true; }
    return false;
}

bool parseCodeName(uint16_t type, const std::string& name, uint16_t& out_code) {
    if (type == EV_SYN && name == "SYN_REPORT") {
        out_code = SYN_REPORT;
        return true;
    }
    if (type == EV_MSC && name == "MSC_SCAN") {
        out_code = MSC_SCAN;
        return true;
    }
    const auto& table = getTable();
    auto it = table.name_to_code.find(name);
    if (it != table.name_to_code.end()) {
        out_code = it->second;
        return true;
    }
    return false;
}

} // namespace tff
