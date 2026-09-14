#include "tff_key_codes.h"
#include <unordered_map>
#include <algorithm>
#include <cctype>

namespace tff {

namespace {

struct KeyTable {
    std::unordered_map<std::string, KeyCode> word_to_code;
    std::unordered_map<KeyCode, std::string> code_to_word;
    std::unordered_map<KeyCode, std::string> code_to_short;
    std::unordered_map<std::string, KeyCode> name_to_code;

    KeyTable() {
        name_to_code["KEY_RESERVED"] = 0;
        word_to_code["reserved"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "reserved";
            code_to_short[0] = "RESERVED";
        }
        name_to_code["KEY_ESC"] = 1;
        word_to_code["esc"] = 1;
        if (code_to_word.find(1) == code_to_word.end()) {
            code_to_word[1] = "esc";
            code_to_short[1] = "ESC";
        }
        name_to_code["KEY_1"] = 2;
        word_to_code["1"] = 2;
        if (code_to_word.find(2) == code_to_word.end()) {
            code_to_word[2] = "1";
            code_to_short[2] = "1";
        }
        name_to_code["KEY_2"] = 3;
        word_to_code["2"] = 3;
        if (code_to_word.find(3) == code_to_word.end()) {
            code_to_word[3] = "2";
            code_to_short[3] = "2";
        }
        name_to_code["KEY_3"] = 4;
        word_to_code["3"] = 4;
        if (code_to_word.find(4) == code_to_word.end()) {
            code_to_word[4] = "3";
            code_to_short[4] = "3";
        }
        name_to_code["KEY_4"] = 5;
        word_to_code["4"] = 5;
        if (code_to_word.find(5) == code_to_word.end()) {
            code_to_word[5] = "4";
            code_to_short[5] = "4";
        }
        name_to_code["KEY_5"] = 6;
        word_to_code["5"] = 6;
        if (code_to_word.find(6) == code_to_word.end()) {
            code_to_word[6] = "5";
            code_to_short[6] = "5";
        }
        name_to_code["KEY_6"] = 7;
        word_to_code["6"] = 7;
        if (code_to_word.find(7) == code_to_word.end()) {
            code_to_word[7] = "6";
            code_to_short[7] = "6";
        }
        name_to_code["KEY_7"] = 8;
        word_to_code["7"] = 8;
        if (code_to_word.find(8) == code_to_word.end()) {
            code_to_word[8] = "7";
            code_to_short[8] = "7";
        }
        name_to_code["KEY_8"] = 9;
        word_to_code["8"] = 9;
        if (code_to_word.find(9) == code_to_word.end()) {
            code_to_word[9] = "8";
            code_to_short[9] = "8";
        }
        name_to_code["KEY_9"] = 10;
        word_to_code["9"] = 10;
        if (code_to_word.find(10) == code_to_word.end()) {
            code_to_word[10] = "9";
            code_to_short[10] = "9";
        }
        name_to_code["KEY_0"] = 11;
        word_to_code["0"] = 11;
        if (code_to_word.find(11) == code_to_word.end()) {
            code_to_word[11] = "0";
            code_to_short[11] = "0";
        }
        name_to_code["KEY_MINUS"] = 12;
        word_to_code["minus"] = 12;
        if (code_to_word.find(12) == code_to_word.end()) {
            code_to_word[12] = "minus";
            code_to_short[12] = "MINUS";
        }
        name_to_code["KEY_EQUAL"] = 13;
        word_to_code["equal"] = 13;
        if (code_to_word.find(13) == code_to_word.end()) {
            code_to_word[13] = "equal";
            code_to_short[13] = "EQUAL";
        }
        name_to_code["KEY_BACKSPACE"] = 14;
        word_to_code["backspace"] = 14;
        if (code_to_word.find(14) == code_to_word.end()) {
            code_to_word[14] = "backspace";
            code_to_short[14] = "BACKSPACE";
        }
        name_to_code["KEY_TAB"] = 15;
        word_to_code["tab"] = 15;
        if (code_to_word.find(15) == code_to_word.end()) {
            code_to_word[15] = "tab";
            code_to_short[15] = "TAB";
        }
        name_to_code["KEY_Q"] = 16;
        word_to_code["q"] = 16;
        if (code_to_word.find(16) == code_to_word.end()) {
            code_to_word[16] = "q";
            code_to_short[16] = "Q";
        }
        name_to_code["KEY_W"] = 17;
        word_to_code["w"] = 17;
        if (code_to_word.find(17) == code_to_word.end()) {
            code_to_word[17] = "w";
            code_to_short[17] = "W";
        }
        name_to_code["KEY_E"] = 18;
        word_to_code["e"] = 18;
        if (code_to_word.find(18) == code_to_word.end()) {
            code_to_word[18] = "e";
            code_to_short[18] = "E";
        }
        name_to_code["KEY_R"] = 19;
        word_to_code["r"] = 19;
        if (code_to_word.find(19) == code_to_word.end()) {
            code_to_word[19] = "r";
            code_to_short[19] = "R";
        }
        name_to_code["KEY_T"] = 20;
        word_to_code["t"] = 20;
        if (code_to_word.find(20) == code_to_word.end()) {
            code_to_word[20] = "t";
            code_to_short[20] = "T";
        }
        name_to_code["KEY_Y"] = 21;
        word_to_code["y"] = 21;
        if (code_to_word.find(21) == code_to_word.end()) {
            code_to_word[21] = "y";
            code_to_short[21] = "Y";
        }
        name_to_code["KEY_U"] = 22;
        word_to_code["u"] = 22;
        if (code_to_word.find(22) == code_to_word.end()) {
            code_to_word[22] = "u";
            code_to_short[22] = "U";
        }
        name_to_code["KEY_I"] = 23;
        word_to_code["i"] = 23;
        if (code_to_word.find(23) == code_to_word.end()) {
            code_to_word[23] = "i";
            code_to_short[23] = "I";
        }
        name_to_code["KEY_O"] = 24;
        word_to_code["o"] = 24;
        if (code_to_word.find(24) == code_to_word.end()) {
            code_to_word[24] = "o";
            code_to_short[24] = "O";
        }
        name_to_code["KEY_P"] = 25;
        word_to_code["p"] = 25;
        if (code_to_word.find(25) == code_to_word.end()) {
            code_to_word[25] = "p";
            code_to_short[25] = "P";
        }
        name_to_code["KEY_LEFTBRACE"] = 26;
        word_to_code["leftbrace"] = 26;
        if (code_to_word.find(26) == code_to_word.end()) {
            code_to_word[26] = "leftbrace";
            code_to_short[26] = "LEFTBRACE";
        }
        name_to_code["KEY_RIGHTBRACE"] = 27;
        word_to_code["rightbrace"] = 27;
        if (code_to_word.find(27) == code_to_word.end()) {
            code_to_word[27] = "rightbrace";
            code_to_short[27] = "RIGHTBRACE";
        }
        name_to_code["KEY_ENTER"] = 28;
        word_to_code["enter"] = 28;
        if (code_to_word.find(28) == code_to_word.end()) {
            code_to_word[28] = "enter";
            code_to_short[28] = "ENTER";
        }
        name_to_code["KEY_LEFTCTRL"] = 29;
        word_to_code["leftctrl"] = 29;
        if (code_to_word.find(29) == code_to_word.end()) {
            code_to_word[29] = "leftctrl";
            code_to_short[29] = "LEFTCTRL";
        }
        name_to_code["KEY_A"] = 30;
        word_to_code["a"] = 30;
        if (code_to_word.find(30) == code_to_word.end()) {
            code_to_word[30] = "a";
            code_to_short[30] = "A";
        }
        name_to_code["KEY_S"] = 31;
        word_to_code["s"] = 31;
        if (code_to_word.find(31) == code_to_word.end()) {
            code_to_word[31] = "s";
            code_to_short[31] = "S";
        }
        name_to_code["KEY_D"] = 32;
        word_to_code["d"] = 32;
        if (code_to_word.find(32) == code_to_word.end()) {
            code_to_word[32] = "d";
            code_to_short[32] = "D";
        }
        name_to_code["KEY_F"] = 33;
        word_to_code["f"] = 33;
        if (code_to_word.find(33) == code_to_word.end()) {
            code_to_word[33] = "f";
            code_to_short[33] = "F";
        }
        name_to_code["KEY_G"] = 34;
        word_to_code["g"] = 34;
        if (code_to_word.find(34) == code_to_word.end()) {
            code_to_word[34] = "g";
            code_to_short[34] = "G";
        }
        name_to_code["KEY_H"] = 35;
        word_to_code["h"] = 35;
        if (code_to_word.find(35) == code_to_word.end()) {
            code_to_word[35] = "h";
            code_to_short[35] = "H";
        }
        name_to_code["KEY_J"] = 36;
        word_to_code["j"] = 36;
        if (code_to_word.find(36) == code_to_word.end()) {
            code_to_word[36] = "j";
            code_to_short[36] = "J";
        }
        name_to_code["KEY_K"] = 37;
        word_to_code["k"] = 37;
        if (code_to_word.find(37) == code_to_word.end()) {
            code_to_word[37] = "k";
            code_to_short[37] = "K";
        }
        name_to_code["KEY_L"] = 38;
        word_to_code["l"] = 38;
        if (code_to_word.find(38) == code_to_word.end()) {
            code_to_word[38] = "l";
            code_to_short[38] = "L";
        }
        name_to_code["KEY_SEMICOLON"] = 39;
        word_to_code["semicolon"] = 39;
        if (code_to_word.find(39) == code_to_word.end()) {
            code_to_word[39] = "semicolon";
            code_to_short[39] = "SEMICOLON";
        }
        name_to_code["KEY_APOSTROPHE"] = 40;
        word_to_code["apostrophe"] = 40;
        if (code_to_word.find(40) == code_to_word.end()) {
            code_to_word[40] = "apostrophe";
            code_to_short[40] = "APOSTROPHE";
        }
        name_to_code["KEY_GRAVE"] = 41;
        word_to_code["grave"] = 41;
        if (code_to_word.find(41) == code_to_word.end()) {
            code_to_word[41] = "grave";
            code_to_short[41] = "GRAVE";
        }
        name_to_code["KEY_LEFTSHIFT"] = 42;
        word_to_code["leftshift"] = 42;
        if (code_to_word.find(42) == code_to_word.end()) {
            code_to_word[42] = "leftshift";
            code_to_short[42] = "LEFTSHIFT";
        }
        name_to_code["KEY_BACKSLASH"] = 43;
        word_to_code["backslash"] = 43;
        if (code_to_word.find(43) == code_to_word.end()) {
            code_to_word[43] = "backslash";
            code_to_short[43] = "BACKSLASH";
        }
        name_to_code["KEY_Z"] = 44;
        word_to_code["z"] = 44;
        if (code_to_word.find(44) == code_to_word.end()) {
            code_to_word[44] = "z";
            code_to_short[44] = "Z";
        }
        name_to_code["KEY_X"] = 45;
        word_to_code["x"] = 45;
        if (code_to_word.find(45) == code_to_word.end()) {
            code_to_word[45] = "x";
            code_to_short[45] = "X";
        }
        name_to_code["KEY_C"] = 46;
        word_to_code["c"] = 46;
        if (code_to_word.find(46) == code_to_word.end()) {
            code_to_word[46] = "c";
            code_to_short[46] = "C";
        }
        name_to_code["KEY_V"] = 47;
        word_to_code["v"] = 47;
        if (code_to_word.find(47) == code_to_word.end()) {
            code_to_word[47] = "v";
            code_to_short[47] = "V";
        }
        name_to_code["KEY_B"] = 48;
        word_to_code["b"] = 48;
        if (code_to_word.find(48) == code_to_word.end()) {
            code_to_word[48] = "b";
            code_to_short[48] = "B";
        }
        name_to_code["KEY_N"] = 49;
        word_to_code["n"] = 49;
        if (code_to_word.find(49) == code_to_word.end()) {
            code_to_word[49] = "n";
            code_to_short[49] = "N";
        }
        name_to_code["KEY_M"] = 50;
        word_to_code["m"] = 50;
        if (code_to_word.find(50) == code_to_word.end()) {
            code_to_word[50] = "m";
            code_to_short[50] = "M";
        }
        name_to_code["KEY_COMMA"] = 51;
        word_to_code["comma"] = 51;
        if (code_to_word.find(51) == code_to_word.end()) {
            code_to_word[51] = "comma";
            code_to_short[51] = "COMMA";
        }
        name_to_code["KEY_DOT"] = 52;
        word_to_code["dot"] = 52;
        if (code_to_word.find(52) == code_to_word.end()) {
            code_to_word[52] = "dot";
            code_to_short[52] = "DOT";
        }
        name_to_code["KEY_SLASH"] = 53;
        word_to_code["slash"] = 53;
        if (code_to_word.find(53) == code_to_word.end()) {
            code_to_word[53] = "slash";
            code_to_short[53] = "SLASH";
        }
        name_to_code["KEY_RIGHTSHIFT"] = 54;
        word_to_code["rightshift"] = 54;
        if (code_to_word.find(54) == code_to_word.end()) {
            code_to_word[54] = "rightshift";
            code_to_short[54] = "RIGHTSHIFT";
        }
        name_to_code["KEY_KPASTERISK"] = 55;
        word_to_code["kpasterisk"] = 55;
        if (code_to_word.find(55) == code_to_word.end()) {
            code_to_word[55] = "kpasterisk";
            code_to_short[55] = "KPASTERISK";
        }
        name_to_code["KEY_LEFTALT"] = 56;
        word_to_code["leftalt"] = 56;
        if (code_to_word.find(56) == code_to_word.end()) {
            code_to_word[56] = "leftalt";
            code_to_short[56] = "LEFTALT";
        }
        name_to_code["KEY_SPACE"] = 57;
        word_to_code["space"] = 57;
        if (code_to_word.find(57) == code_to_word.end()) {
            code_to_word[57] = "space";
            code_to_short[57] = "SPACE";
        }
        name_to_code["KEY_CAPSLOCK"] = 58;
        word_to_code["capslock"] = 58;
        if (code_to_word.find(58) == code_to_word.end()) {
            code_to_word[58] = "capslock";
            code_to_short[58] = "CAPSLOCK";
        }
        name_to_code["KEY_F1"] = 59;
        word_to_code["f1"] = 59;
        if (code_to_word.find(59) == code_to_word.end()) {
            code_to_word[59] = "f1";
            code_to_short[59] = "F1";
        }
        name_to_code["KEY_F2"] = 60;
        word_to_code["f2"] = 60;
        if (code_to_word.find(60) == code_to_word.end()) {
            code_to_word[60] = "f2";
            code_to_short[60] = "F2";
        }
        name_to_code["KEY_F3"] = 61;
        word_to_code["f3"] = 61;
        if (code_to_word.find(61) == code_to_word.end()) {
            code_to_word[61] = "f3";
            code_to_short[61] = "F3";
        }
        name_to_code["KEY_F4"] = 62;
        word_to_code["f4"] = 62;
        if (code_to_word.find(62) == code_to_word.end()) {
            code_to_word[62] = "f4";
            code_to_short[62] = "F4";
        }
        name_to_code["KEY_F5"] = 63;
        word_to_code["f5"] = 63;
        if (code_to_word.find(63) == code_to_word.end()) {
            code_to_word[63] = "f5";
            code_to_short[63] = "F5";
        }
        name_to_code["KEY_F6"] = 64;
        word_to_code["f6"] = 64;
        if (code_to_word.find(64) == code_to_word.end()) {
            code_to_word[64] = "f6";
            code_to_short[64] = "F6";
        }
        name_to_code["KEY_F7"] = 65;
        word_to_code["f7"] = 65;
        if (code_to_word.find(65) == code_to_word.end()) {
            code_to_word[65] = "f7";
            code_to_short[65] = "F7";
        }
        name_to_code["KEY_F8"] = 66;
        word_to_code["f8"] = 66;
        if (code_to_word.find(66) == code_to_word.end()) {
            code_to_word[66] = "f8";
            code_to_short[66] = "F8";
        }
        name_to_code["KEY_F9"] = 67;
        word_to_code["f9"] = 67;
        if (code_to_word.find(67) == code_to_word.end()) {
            code_to_word[67] = "f9";
            code_to_short[67] = "F9";
        }
        name_to_code["KEY_F10"] = 68;
        word_to_code["f10"] = 68;
        if (code_to_word.find(68) == code_to_word.end()) {
            code_to_word[68] = "f10";
            code_to_short[68] = "F10";
        }
        name_to_code["KEY_NUMLOCK"] = 69;
        word_to_code["numlock"] = 69;
        if (code_to_word.find(69) == code_to_word.end()) {
            code_to_word[69] = "numlock";
            code_to_short[69] = "NUMLOCK";
        }
        name_to_code["KEY_SCROLLLOCK"] = 70;
        word_to_code["scrolllock"] = 70;
        if (code_to_word.find(70) == code_to_word.end()) {
            code_to_word[70] = "scrolllock";
            code_to_short[70] = "SCROLLLOCK";
        }
        name_to_code["KEY_KP7"] = 71;
        word_to_code["kp7"] = 71;
        if (code_to_word.find(71) == code_to_word.end()) {
            code_to_word[71] = "kp7";
            code_to_short[71] = "KP7";
        }
        name_to_code["KEY_KP8"] = 72;
        word_to_code["kp8"] = 72;
        if (code_to_word.find(72) == code_to_word.end()) {
            code_to_word[72] = "kp8";
            code_to_short[72] = "KP8";
        }
        name_to_code["KEY_KP9"] = 73;
        word_to_code["kp9"] = 73;
        if (code_to_word.find(73) == code_to_word.end()) {
            code_to_word[73] = "kp9";
            code_to_short[73] = "KP9";
        }
        name_to_code["KEY_KPMINUS"] = 74;
        word_to_code["kpminus"] = 74;
        if (code_to_word.find(74) == code_to_word.end()) {
            code_to_word[74] = "kpminus";
            code_to_short[74] = "KPMINUS";
        }
        name_to_code["KEY_KP4"] = 75;
        word_to_code["kp4"] = 75;
        if (code_to_word.find(75) == code_to_word.end()) {
            code_to_word[75] = "kp4";
            code_to_short[75] = "KP4";
        }
        name_to_code["KEY_KP5"] = 76;
        word_to_code["kp5"] = 76;
        if (code_to_word.find(76) == code_to_word.end()) {
            code_to_word[76] = "kp5";
            code_to_short[76] = "KP5";
        }
        name_to_code["KEY_KP6"] = 77;
        word_to_code["kp6"] = 77;
        if (code_to_word.find(77) == code_to_word.end()) {
            code_to_word[77] = "kp6";
            code_to_short[77] = "KP6";
        }
        name_to_code["KEY_KPPLUS"] = 78;
        word_to_code["kpplus"] = 78;
        if (code_to_word.find(78) == code_to_word.end()) {
            code_to_word[78] = "kpplus";
            code_to_short[78] = "KPPLUS";
        }
        name_to_code["KEY_KP1"] = 79;
        word_to_code["kp1"] = 79;
        if (code_to_word.find(79) == code_to_word.end()) {
            code_to_word[79] = "kp1";
            code_to_short[79] = "KP1";
        }
        name_to_code["KEY_KP2"] = 80;
        word_to_code["kp2"] = 80;
        if (code_to_word.find(80) == code_to_word.end()) {
            code_to_word[80] = "kp2";
            code_to_short[80] = "KP2";
        }
        name_to_code["KEY_KP3"] = 81;
        word_to_code["kp3"] = 81;
        if (code_to_word.find(81) == code_to_word.end()) {
            code_to_word[81] = "kp3";
            code_to_short[81] = "KP3";
        }
        name_to_code["KEY_KP0"] = 82;
        word_to_code["kp0"] = 82;
        if (code_to_word.find(82) == code_to_word.end()) {
            code_to_word[82] = "kp0";
            code_to_short[82] = "KP0";
        }
        name_to_code["KEY_KPDOT"] = 83;
        word_to_code["kpdot"] = 83;
        if (code_to_word.find(83) == code_to_word.end()) {
            code_to_word[83] = "kpdot";
            code_to_short[83] = "KPDOT";
        }
        name_to_code["KEY_ZENKAKUHANKAKU"] = 85;
        word_to_code["zenkakuhankaku"] = 85;
        if (code_to_word.find(85) == code_to_word.end()) {
            code_to_word[85] = "zenkakuhankaku";
            code_to_short[85] = "ZENKAKUHANKAKU";
        }
        name_to_code["KEY_102ND"] = 86;
        word_to_code["102nd"] = 86;
        if (code_to_word.find(86) == code_to_word.end()) {
            code_to_word[86] = "102nd";
            code_to_short[86] = "102ND";
        }
        name_to_code["KEY_F11"] = 87;
        word_to_code["f11"] = 87;
        if (code_to_word.find(87) == code_to_word.end()) {
            code_to_word[87] = "f11";
            code_to_short[87] = "F11";
        }
        name_to_code["KEY_F12"] = 88;
        word_to_code["f12"] = 88;
        if (code_to_word.find(88) == code_to_word.end()) {
            code_to_word[88] = "f12";
            code_to_short[88] = "F12";
        }
        name_to_code["KEY_RO"] = 89;
        word_to_code["ro"] = 89;
        if (code_to_word.find(89) == code_to_word.end()) {
            code_to_word[89] = "ro";
            code_to_short[89] = "RO";
        }
        name_to_code["KEY_KATAKANA"] = 90;
        word_to_code["katakana"] = 90;
        if (code_to_word.find(90) == code_to_word.end()) {
            code_to_word[90] = "katakana";
            code_to_short[90] = "KATAKANA";
        }
        name_to_code["KEY_HIRAGANA"] = 91;
        word_to_code["hiragana"] = 91;
        if (code_to_word.find(91) == code_to_word.end()) {
            code_to_word[91] = "hiragana";
            code_to_short[91] = "HIRAGANA";
        }
        name_to_code["KEY_HENKAN"] = 92;
        word_to_code["henkan"] = 92;
        if (code_to_word.find(92) == code_to_word.end()) {
            code_to_word[92] = "henkan";
            code_to_short[92] = "HENKAN";
        }
        name_to_code["KEY_KATAKANAHIRAGANA"] = 93;
        word_to_code["katakanahiragana"] = 93;
        if (code_to_word.find(93) == code_to_word.end()) {
            code_to_word[93] = "katakanahiragana";
            code_to_short[93] = "KATAKANAHIRAGANA";
        }
        name_to_code["KEY_MUHENKAN"] = 94;
        word_to_code["muhenkan"] = 94;
        if (code_to_word.find(94) == code_to_word.end()) {
            code_to_word[94] = "muhenkan";
            code_to_short[94] = "MUHENKAN";
        }
        name_to_code["KEY_KPJPCOMMA"] = 95;
        word_to_code["kpjpcomma"] = 95;
        if (code_to_word.find(95) == code_to_word.end()) {
            code_to_word[95] = "kpjpcomma";
            code_to_short[95] = "KPJPCOMMA";
        }
        name_to_code["KEY_KPENTER"] = 96;
        word_to_code["kpenter"] = 96;
        if (code_to_word.find(96) == code_to_word.end()) {
            code_to_word[96] = "kpenter";
            code_to_short[96] = "KPENTER";
        }
        name_to_code["KEY_RIGHTCTRL"] = 97;
        word_to_code["rightctrl"] = 97;
        if (code_to_word.find(97) == code_to_word.end()) {
            code_to_word[97] = "rightctrl";
            code_to_short[97] = "RIGHTCTRL";
        }
        name_to_code["KEY_KPSLASH"] = 98;
        word_to_code["kpslash"] = 98;
        if (code_to_word.find(98) == code_to_word.end()) {
            code_to_word[98] = "kpslash";
            code_to_short[98] = "KPSLASH";
        }
        name_to_code["KEY_SYSRQ"] = 99;
        word_to_code["sysrq"] = 99;
        if (code_to_word.find(99) == code_to_word.end()) {
            code_to_word[99] = "sysrq";
            code_to_short[99] = "SYSRQ";
        }
        name_to_code["KEY_RIGHTALT"] = 100;
        word_to_code["rightalt"] = 100;
        if (code_to_word.find(100) == code_to_word.end()) {
            code_to_word[100] = "rightalt";
            code_to_short[100] = "RIGHTALT";
        }
        name_to_code["KEY_LINEFEED"] = 101;
        word_to_code["linefeed"] = 101;
        if (code_to_word.find(101) == code_to_word.end()) {
            code_to_word[101] = "linefeed";
            code_to_short[101] = "LINEFEED";
        }
        name_to_code["KEY_HOME"] = 102;
        word_to_code["home"] = 102;
        if (code_to_word.find(102) == code_to_word.end()) {
            code_to_word[102] = "home";
            code_to_short[102] = "HOME";
        }
        name_to_code["KEY_UP"] = 103;
        word_to_code["up"] = 103;
        if (code_to_word.find(103) == code_to_word.end()) {
            code_to_word[103] = "up";
            code_to_short[103] = "UP";
        }
        name_to_code["KEY_PAGEUP"] = 104;
        word_to_code["pageup"] = 104;
        if (code_to_word.find(104) == code_to_word.end()) {
            code_to_word[104] = "pageup";
            code_to_short[104] = "PAGEUP";
        }
        name_to_code["KEY_LEFT"] = 105;
        word_to_code["left"] = 105;
        if (code_to_word.find(105) == code_to_word.end()) {
            code_to_word[105] = "left";
            code_to_short[105] = "LEFT";
        }
        name_to_code["KEY_RIGHT"] = 106;
        word_to_code["right"] = 106;
        if (code_to_word.find(106) == code_to_word.end()) {
            code_to_word[106] = "right";
            code_to_short[106] = "RIGHT";
        }
        name_to_code["KEY_END"] = 107;
        word_to_code["end"] = 107;
        if (code_to_word.find(107) == code_to_word.end()) {
            code_to_word[107] = "end";
            code_to_short[107] = "END";
        }
        name_to_code["KEY_DOWN"] = 108;
        word_to_code["down"] = 108;
        if (code_to_word.find(108) == code_to_word.end()) {
            code_to_word[108] = "down";
            code_to_short[108] = "DOWN";
        }
        name_to_code["KEY_PAGEDOWN"] = 109;
        word_to_code["pagedown"] = 109;
        if (code_to_word.find(109) == code_to_word.end()) {
            code_to_word[109] = "pagedown";
            code_to_short[109] = "PAGEDOWN";
        }
        name_to_code["KEY_INSERT"] = 110;
        word_to_code["insert"] = 110;
        if (code_to_word.find(110) == code_to_word.end()) {
            code_to_word[110] = "insert";
            code_to_short[110] = "INSERT";
        }
        name_to_code["KEY_DELETE"] = 111;
        word_to_code["delete"] = 111;
        if (code_to_word.find(111) == code_to_word.end()) {
            code_to_word[111] = "delete";
            code_to_short[111] = "DELETE";
        }
        name_to_code["KEY_MACRO"] = 112;
        word_to_code["macro"] = 112;
        if (code_to_word.find(112) == code_to_word.end()) {
            code_to_word[112] = "macro";
            code_to_short[112] = "MACRO";
        }
        name_to_code["KEY_MUTE"] = 113;
        word_to_code["mute"] = 113;
        if (code_to_word.find(113) == code_to_word.end()) {
            code_to_word[113] = "mute";
            code_to_short[113] = "MUTE";
        }
        name_to_code["KEY_VOLUMEDOWN"] = 114;
        word_to_code["volumedown"] = 114;
        if (code_to_word.find(114) == code_to_word.end()) {
            code_to_word[114] = "volumedown";
            code_to_short[114] = "VOLUMEDOWN";
        }
        name_to_code["KEY_VOLUMEUP"] = 115;
        word_to_code["volumeup"] = 115;
        if (code_to_word.find(115) == code_to_word.end()) {
            code_to_word[115] = "volumeup";
            code_to_short[115] = "VOLUMEUP";
        }
        name_to_code["KEY_POWER"] = 116;
        word_to_code["power"] = 116;
        if (code_to_word.find(116) == code_to_word.end()) {
            code_to_word[116] = "power";
            code_to_short[116] = "POWER";
        }
        name_to_code["KEY_KPEQUAL"] = 117;
        word_to_code["kpequal"] = 117;
        if (code_to_word.find(117) == code_to_word.end()) {
            code_to_word[117] = "kpequal";
            code_to_short[117] = "KPEQUAL";
        }
        name_to_code["KEY_KPPLUSMINUS"] = 118;
        word_to_code["kpplusminus"] = 118;
        if (code_to_word.find(118) == code_to_word.end()) {
            code_to_word[118] = "kpplusminus";
            code_to_short[118] = "KPPLUSMINUS";
        }
        name_to_code["KEY_PAUSE"] = 119;
        word_to_code["pause"] = 119;
        if (code_to_word.find(119) == code_to_word.end()) {
            code_to_word[119] = "pause";
            code_to_short[119] = "PAUSE";
        }
        name_to_code["KEY_SCALE"] = 120;
        word_to_code["scale"] = 120;
        if (code_to_word.find(120) == code_to_word.end()) {
            code_to_word[120] = "scale";
            code_to_short[120] = "SCALE";
        }
        name_to_code["KEY_KPCOMMA"] = 121;
        word_to_code["kpcomma"] = 121;
        if (code_to_word.find(121) == code_to_word.end()) {
            code_to_word[121] = "kpcomma";
            code_to_short[121] = "KPCOMMA";
        }
        name_to_code["KEY_HANGEUL"] = 122;
        word_to_code["hangeul"] = 122;
        if (code_to_word.find(122) == code_to_word.end()) {
            code_to_word[122] = "hangeul";
            code_to_short[122] = "HANGEUL";
        }
        name_to_code["KEY_HANJA"] = 123;
        word_to_code["hanja"] = 123;
        if (code_to_word.find(123) == code_to_word.end()) {
            code_to_word[123] = "hanja";
            code_to_short[123] = "HANJA";
        }
        name_to_code["KEY_YEN"] = 124;
        word_to_code["yen"] = 124;
        if (code_to_word.find(124) == code_to_word.end()) {
            code_to_word[124] = "yen";
            code_to_short[124] = "YEN";
        }
        name_to_code["KEY_LEFTMETA"] = 125;
        word_to_code["leftmeta"] = 125;
        if (code_to_word.find(125) == code_to_word.end()) {
            code_to_word[125] = "leftmeta";
            code_to_short[125] = "LEFTMETA";
        }
        name_to_code["KEY_RIGHTMETA"] = 126;
        word_to_code["rightmeta"] = 126;
        if (code_to_word.find(126) == code_to_word.end()) {
            code_to_word[126] = "rightmeta";
            code_to_short[126] = "RIGHTMETA";
        }
        name_to_code["KEY_COMPOSE"] = 127;
        word_to_code["compose"] = 127;
        if (code_to_word.find(127) == code_to_word.end()) {
            code_to_word[127] = "compose";
            code_to_short[127] = "COMPOSE";
        }
        name_to_code["KEY_STOP"] = 128;
        word_to_code["stop"] = 128;
        if (code_to_word.find(128) == code_to_word.end()) {
            code_to_word[128] = "stop";
            code_to_short[128] = "STOP";
        }
        name_to_code["KEY_AGAIN"] = 129;
        word_to_code["again"] = 129;
        if (code_to_word.find(129) == code_to_word.end()) {
            code_to_word[129] = "again";
            code_to_short[129] = "AGAIN";
        }
        name_to_code["KEY_PROPS"] = 130;
        word_to_code["props"] = 130;
        if (code_to_word.find(130) == code_to_word.end()) {
            code_to_word[130] = "props";
            code_to_short[130] = "PROPS";
        }
        name_to_code["KEY_UNDO"] = 131;
        word_to_code["undo"] = 131;
        if (code_to_word.find(131) == code_to_word.end()) {
            code_to_word[131] = "undo";
            code_to_short[131] = "UNDO";
        }
        name_to_code["KEY_FRONT"] = 132;
        word_to_code["front"] = 132;
        if (code_to_word.find(132) == code_to_word.end()) {
            code_to_word[132] = "front";
            code_to_short[132] = "FRONT";
        }
        name_to_code["KEY_COPY"] = 133;
        word_to_code["copy"] = 133;
        if (code_to_word.find(133) == code_to_word.end()) {
            code_to_word[133] = "copy";
            code_to_short[133] = "COPY";
        }
        name_to_code["KEY_OPEN"] = 134;
        word_to_code["open"] = 134;
        if (code_to_word.find(134) == code_to_word.end()) {
            code_to_word[134] = "open";
            code_to_short[134] = "OPEN";
        }
        name_to_code["KEY_PASTE"] = 135;
        word_to_code["paste"] = 135;
        if (code_to_word.find(135) == code_to_word.end()) {
            code_to_word[135] = "paste";
            code_to_short[135] = "PASTE";
        }
        name_to_code["KEY_FIND"] = 136;
        word_to_code["find"] = 136;
        if (code_to_word.find(136) == code_to_word.end()) {
            code_to_word[136] = "find";
            code_to_short[136] = "FIND";
        }
        name_to_code["KEY_CUT"] = 137;
        word_to_code["cut"] = 137;
        if (code_to_word.find(137) == code_to_word.end()) {
            code_to_word[137] = "cut";
            code_to_short[137] = "CUT";
        }
        name_to_code["KEY_HELP"] = 138;
        word_to_code["help"] = 138;
        if (code_to_word.find(138) == code_to_word.end()) {
            code_to_word[138] = "help";
            code_to_short[138] = "HELP";
        }
        name_to_code["KEY_MENU"] = 139;
        word_to_code["menu"] = 139;
        if (code_to_word.find(139) == code_to_word.end()) {
            code_to_word[139] = "menu";
            code_to_short[139] = "MENU";
        }
        name_to_code["KEY_CALC"] = 140;
        word_to_code["calc"] = 140;
        if (code_to_word.find(140) == code_to_word.end()) {
            code_to_word[140] = "calc";
            code_to_short[140] = "CALC";
        }
        name_to_code["KEY_SETUP"] = 141;
        word_to_code["setup"] = 141;
        if (code_to_word.find(141) == code_to_word.end()) {
            code_to_word[141] = "setup";
            code_to_short[141] = "SETUP";
        }
        name_to_code["KEY_SLEEP"] = 142;
        word_to_code["sleep"] = 142;
        if (code_to_word.find(142) == code_to_word.end()) {
            code_to_word[142] = "sleep";
            code_to_short[142] = "SLEEP";
        }
        name_to_code["KEY_WAKEUP"] = 143;
        word_to_code["wakeup"] = 143;
        if (code_to_word.find(143) == code_to_word.end()) {
            code_to_word[143] = "wakeup";
            code_to_short[143] = "WAKEUP";
        }
        name_to_code["KEY_FILE"] = 144;
        word_to_code["file"] = 144;
        if (code_to_word.find(144) == code_to_word.end()) {
            code_to_word[144] = "file";
            code_to_short[144] = "FILE";
        }
        name_to_code["KEY_SENDFILE"] = 145;
        word_to_code["sendfile"] = 145;
        if (code_to_word.find(145) == code_to_word.end()) {
            code_to_word[145] = "sendfile";
            code_to_short[145] = "SENDFILE";
        }
        name_to_code["KEY_DELETEFILE"] = 146;
        word_to_code["deletefile"] = 146;
        if (code_to_word.find(146) == code_to_word.end()) {
            code_to_word[146] = "deletefile";
            code_to_short[146] = "DELETEFILE";
        }
        name_to_code["KEY_XFER"] = 147;
        word_to_code["xfer"] = 147;
        if (code_to_word.find(147) == code_to_word.end()) {
            code_to_word[147] = "xfer";
            code_to_short[147] = "XFER";
        }
        name_to_code["KEY_PROG1"] = 148;
        word_to_code["prog1"] = 148;
        if (code_to_word.find(148) == code_to_word.end()) {
            code_to_word[148] = "prog1";
            code_to_short[148] = "PROG1";
        }
        name_to_code["KEY_PROG2"] = 149;
        word_to_code["prog2"] = 149;
        if (code_to_word.find(149) == code_to_word.end()) {
            code_to_word[149] = "prog2";
            code_to_short[149] = "PROG2";
        }
        name_to_code["KEY_WWW"] = 150;
        word_to_code["www"] = 150;
        if (code_to_word.find(150) == code_to_word.end()) {
            code_to_word[150] = "www";
            code_to_short[150] = "WWW";
        }
        name_to_code["KEY_MSDOS"] = 151;
        word_to_code["msdos"] = 151;
        if (code_to_word.find(151) == code_to_word.end()) {
            code_to_word[151] = "msdos";
            code_to_short[151] = "MSDOS";
        }
        name_to_code["KEY_COFFEE"] = 152;
        word_to_code["coffee"] = 152;
        if (code_to_word.find(152) == code_to_word.end()) {
            code_to_word[152] = "coffee";
            code_to_short[152] = "COFFEE";
        }
        name_to_code["KEY_ROTATE_DISPLAY"] = 153;
        word_to_code["rotate_display"] = 153;
        if (code_to_word.find(153) == code_to_word.end()) {
            code_to_word[153] = "rotate_display";
            code_to_short[153] = "ROTATE_DISPLAY";
        }
        name_to_code["KEY_CYCLEWINDOWS"] = 154;
        word_to_code["cyclewindows"] = 154;
        if (code_to_word.find(154) == code_to_word.end()) {
            code_to_word[154] = "cyclewindows";
            code_to_short[154] = "CYCLEWINDOWS";
        }
        name_to_code["KEY_MAIL"] = 155;
        word_to_code["mail"] = 155;
        if (code_to_word.find(155) == code_to_word.end()) {
            code_to_word[155] = "mail";
            code_to_short[155] = "MAIL";
        }
        name_to_code["KEY_BOOKMARKS"] = 156;
        word_to_code["bookmarks"] = 156;
        if (code_to_word.find(156) == code_to_word.end()) {
            code_to_word[156] = "bookmarks";
            code_to_short[156] = "BOOKMARKS";
        }
        name_to_code["KEY_COMPUTER"] = 157;
        word_to_code["computer"] = 157;
        if (code_to_word.find(157) == code_to_word.end()) {
            code_to_word[157] = "computer";
            code_to_short[157] = "COMPUTER";
        }
        name_to_code["KEY_BACK"] = 158;
        word_to_code["back"] = 158;
        if (code_to_word.find(158) == code_to_word.end()) {
            code_to_word[158] = "back";
            code_to_short[158] = "BACK";
        }
        name_to_code["KEY_FORWARD"] = 159;
        word_to_code["forward"] = 159;
        if (code_to_word.find(159) == code_to_word.end()) {
            code_to_word[159] = "forward";
            code_to_short[159] = "FORWARD";
        }
        name_to_code["KEY_CLOSECD"] = 160;
        word_to_code["closecd"] = 160;
        if (code_to_word.find(160) == code_to_word.end()) {
            code_to_word[160] = "closecd";
            code_to_short[160] = "CLOSECD";
        }
        name_to_code["KEY_EJECTCD"] = 161;
        word_to_code["ejectcd"] = 161;
        if (code_to_word.find(161) == code_to_word.end()) {
            code_to_word[161] = "ejectcd";
            code_to_short[161] = "EJECTCD";
        }
        name_to_code["KEY_EJECTCLOSECD"] = 162;
        word_to_code["ejectclosecd"] = 162;
        if (code_to_word.find(162) == code_to_word.end()) {
            code_to_word[162] = "ejectclosecd";
            code_to_short[162] = "EJECTCLOSECD";
        }
        name_to_code["KEY_NEXTSONG"] = 163;
        word_to_code["nextsong"] = 163;
        if (code_to_word.find(163) == code_to_word.end()) {
            code_to_word[163] = "nextsong";
            code_to_short[163] = "NEXTSONG";
        }
        name_to_code["KEY_PLAYPAUSE"] = 164;
        word_to_code["playpause"] = 164;
        if (code_to_word.find(164) == code_to_word.end()) {
            code_to_word[164] = "playpause";
            code_to_short[164] = "PLAYPAUSE";
        }
        name_to_code["KEY_PREVIOUSSONG"] = 165;
        word_to_code["previoussong"] = 165;
        if (code_to_word.find(165) == code_to_word.end()) {
            code_to_word[165] = "previoussong";
            code_to_short[165] = "PREVIOUSSONG";
        }
        name_to_code["KEY_STOPCD"] = 166;
        word_to_code["stopcd"] = 166;
        if (code_to_word.find(166) == code_to_word.end()) {
            code_to_word[166] = "stopcd";
            code_to_short[166] = "STOPCD";
        }
        name_to_code["KEY_RECORD"] = 167;
        word_to_code["record"] = 167;
        if (code_to_word.find(167) == code_to_word.end()) {
            code_to_word[167] = "record";
            code_to_short[167] = "RECORD";
        }
        name_to_code["KEY_REWIND"] = 168;
        word_to_code["rewind"] = 168;
        if (code_to_word.find(168) == code_to_word.end()) {
            code_to_word[168] = "rewind";
            code_to_short[168] = "REWIND";
        }
        name_to_code["KEY_PHONE"] = 169;
        word_to_code["phone"] = 169;
        if (code_to_word.find(169) == code_to_word.end()) {
            code_to_word[169] = "phone";
            code_to_short[169] = "PHONE";
        }
        name_to_code["KEY_ISO"] = 170;
        word_to_code["iso"] = 170;
        if (code_to_word.find(170) == code_to_word.end()) {
            code_to_word[170] = "iso";
            code_to_short[170] = "ISO";
        }
        name_to_code["KEY_CONFIG"] = 171;
        word_to_code["config"] = 171;
        if (code_to_word.find(171) == code_to_word.end()) {
            code_to_word[171] = "config";
            code_to_short[171] = "CONFIG";
        }
        name_to_code["KEY_HOMEPAGE"] = 172;
        word_to_code["homepage"] = 172;
        if (code_to_word.find(172) == code_to_word.end()) {
            code_to_word[172] = "homepage";
            code_to_short[172] = "HOMEPAGE";
        }
        name_to_code["KEY_REFRESH"] = 173;
        word_to_code["refresh"] = 173;
        if (code_to_word.find(173) == code_to_word.end()) {
            code_to_word[173] = "refresh";
            code_to_short[173] = "REFRESH";
        }
        name_to_code["KEY_EXIT"] = 174;
        word_to_code["exit"] = 174;
        if (code_to_word.find(174) == code_to_word.end()) {
            code_to_word[174] = "exit";
            code_to_short[174] = "EXIT";
        }
        name_to_code["KEY_MOVE"] = 175;
        word_to_code["move"] = 175;
        if (code_to_word.find(175) == code_to_word.end()) {
            code_to_word[175] = "move";
            code_to_short[175] = "MOVE";
        }
        name_to_code["KEY_EDIT"] = 176;
        word_to_code["edit"] = 176;
        if (code_to_word.find(176) == code_to_word.end()) {
            code_to_word[176] = "edit";
            code_to_short[176] = "EDIT";
        }
        name_to_code["KEY_SCROLLUP"] = 177;
        word_to_code["scrollup"] = 177;
        if (code_to_word.find(177) == code_to_word.end()) {
            code_to_word[177] = "scrollup";
            code_to_short[177] = "SCROLLUP";
        }
        name_to_code["KEY_SCROLLDOWN"] = 178;
        word_to_code["scrolldown"] = 178;
        if (code_to_word.find(178) == code_to_word.end()) {
            code_to_word[178] = "scrolldown";
            code_to_short[178] = "SCROLLDOWN";
        }
        name_to_code["KEY_KPLEFTPAREN"] = 179;
        word_to_code["kpleftparen"] = 179;
        if (code_to_word.find(179) == code_to_word.end()) {
            code_to_word[179] = "kpleftparen";
            code_to_short[179] = "KPLEFTPAREN";
        }
        name_to_code["KEY_KPRIGHTPAREN"] = 180;
        word_to_code["kprightparen"] = 180;
        if (code_to_word.find(180) == code_to_word.end()) {
            code_to_word[180] = "kprightparen";
            code_to_short[180] = "KPRIGHTPAREN";
        }
        name_to_code["KEY_NEW"] = 181;
        word_to_code["new"] = 181;
        if (code_to_word.find(181) == code_to_word.end()) {
            code_to_word[181] = "new";
            code_to_short[181] = "NEW";
        }
        name_to_code["KEY_REDO"] = 182;
        word_to_code["redo"] = 182;
        if (code_to_word.find(182) == code_to_word.end()) {
            code_to_word[182] = "redo";
            code_to_short[182] = "REDO";
        }
        name_to_code["KEY_F13"] = 183;
        word_to_code["f13"] = 183;
        if (code_to_word.find(183) == code_to_word.end()) {
            code_to_word[183] = "f13";
            code_to_short[183] = "F13";
        }
        name_to_code["KEY_F14"] = 184;
        word_to_code["f14"] = 184;
        if (code_to_word.find(184) == code_to_word.end()) {
            code_to_word[184] = "f14";
            code_to_short[184] = "F14";
        }
        name_to_code["KEY_F15"] = 185;
        word_to_code["f15"] = 185;
        if (code_to_word.find(185) == code_to_word.end()) {
            code_to_word[185] = "f15";
            code_to_short[185] = "F15";
        }
        name_to_code["KEY_F16"] = 186;
        word_to_code["f16"] = 186;
        if (code_to_word.find(186) == code_to_word.end()) {
            code_to_word[186] = "f16";
            code_to_short[186] = "F16";
        }
        name_to_code["KEY_F17"] = 187;
        word_to_code["f17"] = 187;
        if (code_to_word.find(187) == code_to_word.end()) {
            code_to_word[187] = "f17";
            code_to_short[187] = "F17";
        }
        name_to_code["KEY_F18"] = 188;
        word_to_code["f18"] = 188;
        if (code_to_word.find(188) == code_to_word.end()) {
            code_to_word[188] = "f18";
            code_to_short[188] = "F18";
        }
        name_to_code["KEY_F19"] = 189;
        word_to_code["f19"] = 189;
        if (code_to_word.find(189) == code_to_word.end()) {
            code_to_word[189] = "f19";
            code_to_short[189] = "F19";
        }
        name_to_code["KEY_F20"] = 190;
        word_to_code["f20"] = 190;
        if (code_to_word.find(190) == code_to_word.end()) {
            code_to_word[190] = "f20";
            code_to_short[190] = "F20";
        }
        name_to_code["KEY_F21"] = 191;
        word_to_code["f21"] = 191;
        if (code_to_word.find(191) == code_to_word.end()) {
            code_to_word[191] = "f21";
            code_to_short[191] = "F21";
        }
        name_to_code["KEY_F22"] = 192;
        word_to_code["f22"] = 192;
        if (code_to_word.find(192) == code_to_word.end()) {
            code_to_word[192] = "f22";
            code_to_short[192] = "F22";
        }
        name_to_code["KEY_F23"] = 193;
        word_to_code["f23"] = 193;
        if (code_to_word.find(193) == code_to_word.end()) {
            code_to_word[193] = "f23";
            code_to_short[193] = "F23";
        }
        name_to_code["KEY_F24"] = 194;
        word_to_code["f24"] = 194;
        if (code_to_word.find(194) == code_to_word.end()) {
            code_to_word[194] = "f24";
            code_to_short[194] = "F24";
        }
        name_to_code["KEY_PLAYCD"] = 200;
        word_to_code["playcd"] = 200;
        if (code_to_word.find(200) == code_to_word.end()) {
            code_to_word[200] = "playcd";
            code_to_short[200] = "PLAYCD";
        }
        name_to_code["KEY_PAUSECD"] = 201;
        word_to_code["pausecd"] = 201;
        if (code_to_word.find(201) == code_to_word.end()) {
            code_to_word[201] = "pausecd";
            code_to_short[201] = "PAUSECD";
        }
        name_to_code["KEY_PROG3"] = 202;
        word_to_code["prog3"] = 202;
        if (code_to_word.find(202) == code_to_word.end()) {
            code_to_word[202] = "prog3";
            code_to_short[202] = "PROG3";
        }
        name_to_code["KEY_PROG4"] = 203;
        word_to_code["prog4"] = 203;
        if (code_to_word.find(203) == code_to_word.end()) {
            code_to_word[203] = "prog4";
            code_to_short[203] = "PROG4";
        }
        name_to_code["KEY_ALL_APPLICATIONS"] = 204;
        word_to_code["all_applications"] = 204;
        if (code_to_word.find(204) == code_to_word.end()) {
            code_to_word[204] = "all_applications";
            code_to_short[204] = "ALL_APPLICATIONS";
        }
        name_to_code["KEY_SUSPEND"] = 205;
        word_to_code["suspend"] = 205;
        if (code_to_word.find(205) == code_to_word.end()) {
            code_to_word[205] = "suspend";
            code_to_short[205] = "SUSPEND";
        }
        name_to_code["KEY_CLOSE"] = 206;
        word_to_code["close"] = 206;
        if (code_to_word.find(206) == code_to_word.end()) {
            code_to_word[206] = "close";
            code_to_short[206] = "CLOSE";
        }
        name_to_code["KEY_PLAY"] = 207;
        word_to_code["play"] = 207;
        if (code_to_word.find(207) == code_to_word.end()) {
            code_to_word[207] = "play";
            code_to_short[207] = "PLAY";
        }
        name_to_code["KEY_FASTFORWARD"] = 208;
        word_to_code["fastforward"] = 208;
        if (code_to_word.find(208) == code_to_word.end()) {
            code_to_word[208] = "fastforward";
            code_to_short[208] = "FASTFORWARD";
        }
        name_to_code["KEY_BASSBOOST"] = 209;
        word_to_code["bassboost"] = 209;
        if (code_to_word.find(209) == code_to_word.end()) {
            code_to_word[209] = "bassboost";
            code_to_short[209] = "BASSBOOST";
        }
        name_to_code["KEY_PRINT"] = 210;
        word_to_code["print"] = 210;
        if (code_to_word.find(210) == code_to_word.end()) {
            code_to_word[210] = "print";
            code_to_short[210] = "PRINT";
        }
        name_to_code["KEY_HP"] = 211;
        word_to_code["hp"] = 211;
        if (code_to_word.find(211) == code_to_word.end()) {
            code_to_word[211] = "hp";
            code_to_short[211] = "HP";
        }
        name_to_code["KEY_CAMERA"] = 212;
        word_to_code["camera"] = 212;
        if (code_to_word.find(212) == code_to_word.end()) {
            code_to_word[212] = "camera";
            code_to_short[212] = "CAMERA";
        }
        name_to_code["KEY_SOUND"] = 213;
        word_to_code["sound"] = 213;
        if (code_to_word.find(213) == code_to_word.end()) {
            code_to_word[213] = "sound";
            code_to_short[213] = "SOUND";
        }
        name_to_code["KEY_QUESTION"] = 214;
        word_to_code["question"] = 214;
        if (code_to_word.find(214) == code_to_word.end()) {
            code_to_word[214] = "question";
            code_to_short[214] = "QUESTION";
        }
        name_to_code["KEY_EMAIL"] = 215;
        word_to_code["email"] = 215;
        if (code_to_word.find(215) == code_to_word.end()) {
            code_to_word[215] = "email";
            code_to_short[215] = "EMAIL";
        }
        name_to_code["KEY_CHAT"] = 216;
        word_to_code["chat"] = 216;
        if (code_to_word.find(216) == code_to_word.end()) {
            code_to_word[216] = "chat";
            code_to_short[216] = "CHAT";
        }
        name_to_code["KEY_SEARCH"] = 217;
        word_to_code["search"] = 217;
        if (code_to_word.find(217) == code_to_word.end()) {
            code_to_word[217] = "search";
            code_to_short[217] = "SEARCH";
        }
        name_to_code["KEY_CONNECT"] = 218;
        word_to_code["connect"] = 218;
        if (code_to_word.find(218) == code_to_word.end()) {
            code_to_word[218] = "connect";
            code_to_short[218] = "CONNECT";
        }
        name_to_code["KEY_FINANCE"] = 219;
        word_to_code["finance"] = 219;
        if (code_to_word.find(219) == code_to_word.end()) {
            code_to_word[219] = "finance";
            code_to_short[219] = "FINANCE";
        }
        name_to_code["KEY_SPORT"] = 220;
        word_to_code["sport"] = 220;
        if (code_to_word.find(220) == code_to_word.end()) {
            code_to_word[220] = "sport";
            code_to_short[220] = "SPORT";
        }
        name_to_code["KEY_SHOP"] = 221;
        word_to_code["shop"] = 221;
        if (code_to_word.find(221) == code_to_word.end()) {
            code_to_word[221] = "shop";
            code_to_short[221] = "SHOP";
        }
        name_to_code["KEY_ALTERASE"] = 222;
        word_to_code["alterase"] = 222;
        if (code_to_word.find(222) == code_to_word.end()) {
            code_to_word[222] = "alterase";
            code_to_short[222] = "ALTERASE";
        }
        name_to_code["KEY_CANCEL"] = 223;
        word_to_code["cancel"] = 223;
        if (code_to_word.find(223) == code_to_word.end()) {
            code_to_word[223] = "cancel";
            code_to_short[223] = "CANCEL";
        }
        name_to_code["KEY_BRIGHTNESSDOWN"] = 224;
        word_to_code["brightnessdown"] = 224;
        if (code_to_word.find(224) == code_to_word.end()) {
            code_to_word[224] = "brightnessdown";
            code_to_short[224] = "BRIGHTNESSDOWN";
        }
        name_to_code["KEY_BRIGHTNESSUP"] = 225;
        word_to_code["brightnessup"] = 225;
        if (code_to_word.find(225) == code_to_word.end()) {
            code_to_word[225] = "brightnessup";
            code_to_short[225] = "BRIGHTNESSUP";
        }
        name_to_code["KEY_MEDIA"] = 226;
        word_to_code["media"] = 226;
        if (code_to_word.find(226) == code_to_word.end()) {
            code_to_word[226] = "media";
            code_to_short[226] = "MEDIA";
        }
        name_to_code["KEY_SWITCHVIDEOMODE"] = 227;
        word_to_code["switchvideomode"] = 227;
        if (code_to_word.find(227) == code_to_word.end()) {
            code_to_word[227] = "switchvideomode";
            code_to_short[227] = "SWITCHVIDEOMODE";
        }
        name_to_code["KEY_KBDILLUMTOGGLE"] = 228;
        word_to_code["kbdillumtoggle"] = 228;
        if (code_to_word.find(228) == code_to_word.end()) {
            code_to_word[228] = "kbdillumtoggle";
            code_to_short[228] = "KBDILLUMTOGGLE";
        }
        name_to_code["KEY_KBDILLUMDOWN"] = 229;
        word_to_code["kbdillumdown"] = 229;
        if (code_to_word.find(229) == code_to_word.end()) {
            code_to_word[229] = "kbdillumdown";
            code_to_short[229] = "KBDILLUMDOWN";
        }
        name_to_code["KEY_KBDILLUMUP"] = 230;
        word_to_code["kbdillumup"] = 230;
        if (code_to_word.find(230) == code_to_word.end()) {
            code_to_word[230] = "kbdillumup";
            code_to_short[230] = "KBDILLUMUP";
        }
        name_to_code["KEY_SEND"] = 231;
        word_to_code["send"] = 231;
        if (code_to_word.find(231) == code_to_word.end()) {
            code_to_word[231] = "send";
            code_to_short[231] = "SEND";
        }
        name_to_code["KEY_REPLY"] = 232;
        word_to_code["reply"] = 232;
        if (code_to_word.find(232) == code_to_word.end()) {
            code_to_word[232] = "reply";
            code_to_short[232] = "REPLY";
        }
        name_to_code["KEY_FORWARDMAIL"] = 233;
        word_to_code["forwardmail"] = 233;
        if (code_to_word.find(233) == code_to_word.end()) {
            code_to_word[233] = "forwardmail";
            code_to_short[233] = "FORWARDMAIL";
        }
        name_to_code["KEY_SAVE"] = 234;
        word_to_code["save"] = 234;
        if (code_to_word.find(234) == code_to_word.end()) {
            code_to_word[234] = "save";
            code_to_short[234] = "SAVE";
        }
        name_to_code["KEY_DOCUMENTS"] = 235;
        word_to_code["documents"] = 235;
        if (code_to_word.find(235) == code_to_word.end()) {
            code_to_word[235] = "documents";
            code_to_short[235] = "DOCUMENTS";
        }
        name_to_code["KEY_BATTERY"] = 236;
        word_to_code["battery"] = 236;
        if (code_to_word.find(236) == code_to_word.end()) {
            code_to_word[236] = "battery";
            code_to_short[236] = "BATTERY";
        }
        name_to_code["KEY_BLUETOOTH"] = 237;
        word_to_code["bluetooth"] = 237;
        if (code_to_word.find(237) == code_to_word.end()) {
            code_to_word[237] = "bluetooth";
            code_to_short[237] = "BLUETOOTH";
        }
        name_to_code["KEY_WLAN"] = 238;
        word_to_code["wlan"] = 238;
        if (code_to_word.find(238) == code_to_word.end()) {
            code_to_word[238] = "wlan";
            code_to_short[238] = "WLAN";
        }
        name_to_code["KEY_UWB"] = 239;
        word_to_code["uwb"] = 239;
        if (code_to_word.find(239) == code_to_word.end()) {
            code_to_word[239] = "uwb";
            code_to_short[239] = "UWB";
        }
        name_to_code["KEY_UNKNOWN"] = 240;
        word_to_code["unknown"] = 240;
        if (code_to_word.find(240) == code_to_word.end()) {
            code_to_word[240] = "unknown";
            code_to_short[240] = "UNKNOWN";
        }
        name_to_code["KEY_VIDEO_NEXT"] = 241;
        word_to_code["video_next"] = 241;
        if (code_to_word.find(241) == code_to_word.end()) {
            code_to_word[241] = "video_next";
            code_to_short[241] = "VIDEO_NEXT";
        }
        name_to_code["KEY_VIDEO_PREV"] = 242;
        word_to_code["video_prev"] = 242;
        if (code_to_word.find(242) == code_to_word.end()) {
            code_to_word[242] = "video_prev";
            code_to_short[242] = "VIDEO_PREV";
        }
        name_to_code["KEY_BRIGHTNESS_CYCLE"] = 243;
        word_to_code["brightness_cycle"] = 243;
        if (code_to_word.find(243) == code_to_word.end()) {
            code_to_word[243] = "brightness_cycle";
            code_to_short[243] = "BRIGHTNESS_CYCLE";
        }
        name_to_code["KEY_BRIGHTNESS_AUTO"] = 244;
        word_to_code["brightness_auto"] = 244;
        if (code_to_word.find(244) == code_to_word.end()) {
            code_to_word[244] = "brightness_auto";
            code_to_short[244] = "BRIGHTNESS_AUTO";
        }
        name_to_code["KEY_DISPLAY_OFF"] = 245;
        word_to_code["display_off"] = 245;
        if (code_to_word.find(245) == code_to_word.end()) {
            code_to_word[245] = "display_off";
            code_to_short[245] = "DISPLAY_OFF";
        }
        name_to_code["KEY_WWAN"] = 246;
        word_to_code["wwan"] = 246;
        if (code_to_word.find(246) == code_to_word.end()) {
            code_to_word[246] = "wwan";
            code_to_short[246] = "WWAN";
        }
        name_to_code["KEY_RFKILL"] = 247;
        word_to_code["rfkill"] = 247;
        if (code_to_word.find(247) == code_to_word.end()) {
            code_to_word[247] = "rfkill";
            code_to_short[247] = "RFKILL";
        }
        name_to_code["KEY_MICMUTE"] = 248;
        word_to_code["micmute"] = 248;
        if (code_to_word.find(248) == code_to_word.end()) {
            code_to_word[248] = "micmute";
            code_to_short[248] = "MICMUTE";
        }
        name_to_code["KEY_OK"] = 0;
        word_to_code["ok"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "ok";
            code_to_short[0] = "OK";
        }
        name_to_code["KEY_SELECT"] = 0;
        word_to_code["select"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "select";
            code_to_short[0] = "SELECT";
        }
        name_to_code["KEY_GOTO"] = 0;
        word_to_code["goto"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "goto";
            code_to_short[0] = "GOTO";
        }
        name_to_code["KEY_CLEAR"] = 0;
        word_to_code["clear"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "clear";
            code_to_short[0] = "CLEAR";
        }
        name_to_code["KEY_POWER2"] = 0;
        word_to_code["power2"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "power2";
            code_to_short[0] = "POWER2";
        }
        name_to_code["KEY_OPTION"] = 0;
        word_to_code["option"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "option";
            code_to_short[0] = "OPTION";
        }
        name_to_code["KEY_INFO"] = 0;
        word_to_code["info"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "info";
            code_to_short[0] = "INFO";
        }
        name_to_code["KEY_TIME"] = 0;
        word_to_code["time"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "time";
            code_to_short[0] = "TIME";
        }
        name_to_code["KEY_VENDOR"] = 0;
        word_to_code["vendor"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "vendor";
            code_to_short[0] = "VENDOR";
        }
        name_to_code["KEY_ARCHIVE"] = 0;
        word_to_code["archive"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "archive";
            code_to_short[0] = "ARCHIVE";
        }
        name_to_code["KEY_PROGRAM"] = 0;
        word_to_code["program"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "program";
            code_to_short[0] = "PROGRAM";
        }
        name_to_code["KEY_CHANNEL"] = 0;
        word_to_code["channel"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "channel";
            code_to_short[0] = "CHANNEL";
        }
        name_to_code["KEY_FAVORITES"] = 0;
        word_to_code["favorites"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "favorites";
            code_to_short[0] = "FAVORITES";
        }
        name_to_code["KEY_EPG"] = 0;
        word_to_code["epg"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "epg";
            code_to_short[0] = "EPG";
        }
        name_to_code["KEY_PVR"] = 0;
        word_to_code["pvr"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "pvr";
            code_to_short[0] = "PVR";
        }
        name_to_code["KEY_MHP"] = 0;
        word_to_code["mhp"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "mhp";
            code_to_short[0] = "MHP";
        }
        name_to_code["KEY_LANGUAGE"] = 0;
        word_to_code["language"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "language";
            code_to_short[0] = "LANGUAGE";
        }
        name_to_code["KEY_TITLE"] = 0;
        word_to_code["title"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "title";
            code_to_short[0] = "TITLE";
        }
        name_to_code["KEY_SUBTITLE"] = 0;
        word_to_code["subtitle"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "subtitle";
            code_to_short[0] = "SUBTITLE";
        }
        name_to_code["KEY_ANGLE"] = 0;
        word_to_code["angle"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "angle";
            code_to_short[0] = "ANGLE";
        }
        name_to_code["KEY_FULL_SCREEN"] = 0;
        word_to_code["full_screen"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "full_screen";
            code_to_short[0] = "FULL_SCREEN";
        }
        name_to_code["KEY_MODE"] = 0;
        word_to_code["mode"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "mode";
            code_to_short[0] = "MODE";
        }
        name_to_code["KEY_KEYBOARD"] = 0;
        word_to_code["keyboard"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "keyboard";
            code_to_short[0] = "KEYBOARD";
        }
        name_to_code["KEY_ASPECT_RATIO"] = 0;
        word_to_code["aspect_ratio"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "aspect_ratio";
            code_to_short[0] = "ASPECT_RATIO";
        }
        name_to_code["KEY_PC"] = 0;
        word_to_code["pc"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "pc";
            code_to_short[0] = "PC";
        }
        name_to_code["KEY_TV"] = 0;
        word_to_code["tv"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "tv";
            code_to_short[0] = "TV";
        }
        name_to_code["KEY_TV2"] = 0;
        word_to_code["tv2"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "tv2";
            code_to_short[0] = "TV2";
        }
        name_to_code["KEY_VCR"] = 0;
        word_to_code["vcr"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "vcr";
            code_to_short[0] = "VCR";
        }
        name_to_code["KEY_VCR2"] = 0;
        word_to_code["vcr2"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "vcr2";
            code_to_short[0] = "VCR2";
        }
        name_to_code["KEY_SAT"] = 0;
        word_to_code["sat"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "sat";
            code_to_short[0] = "SAT";
        }
        name_to_code["KEY_SAT2"] = 0;
        word_to_code["sat2"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "sat2";
            code_to_short[0] = "SAT2";
        }
        name_to_code["KEY_CD"] = 0;
        word_to_code["cd"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "cd";
            code_to_short[0] = "CD";
        }
        name_to_code["KEY_TAPE"] = 0;
        word_to_code["tape"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "tape";
            code_to_short[0] = "TAPE";
        }
        name_to_code["KEY_RADIO"] = 0;
        word_to_code["radio"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "radio";
            code_to_short[0] = "RADIO";
        }
        name_to_code["KEY_TUNER"] = 0;
        word_to_code["tuner"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "tuner";
            code_to_short[0] = "TUNER";
        }
        name_to_code["KEY_PLAYER"] = 0;
        word_to_code["player"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "player";
            code_to_short[0] = "PLAYER";
        }
        name_to_code["KEY_TEXT"] = 0;
        word_to_code["text"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "text";
            code_to_short[0] = "TEXT";
        }
        name_to_code["KEY_DVD"] = 0;
        word_to_code["dvd"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "dvd";
            code_to_short[0] = "DVD";
        }
        name_to_code["KEY_AUX"] = 0;
        word_to_code["aux"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "aux";
            code_to_short[0] = "AUX";
        }
        name_to_code["KEY_MP3"] = 0;
        word_to_code["mp3"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "mp3";
            code_to_short[0] = "MP3";
        }
        name_to_code["KEY_AUDIO"] = 0;
        word_to_code["audio"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "audio";
            code_to_short[0] = "AUDIO";
        }
        name_to_code["KEY_VIDEO"] = 0;
        word_to_code["video"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "video";
            code_to_short[0] = "VIDEO";
        }
        name_to_code["KEY_DIRECTORY"] = 0;
        word_to_code["directory"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "directory";
            code_to_short[0] = "DIRECTORY";
        }
        name_to_code["KEY_LIST"] = 0;
        word_to_code["list"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "list";
            code_to_short[0] = "LIST";
        }
        name_to_code["KEY_MEMO"] = 0;
        word_to_code["memo"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "memo";
            code_to_short[0] = "MEMO";
        }
        name_to_code["KEY_CALENDAR"] = 0;
        word_to_code["calendar"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "calendar";
            code_to_short[0] = "CALENDAR";
        }
        name_to_code["KEY_RED"] = 0;
        word_to_code["red"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "red";
            code_to_short[0] = "RED";
        }
        name_to_code["KEY_GREEN"] = 0;
        word_to_code["green"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "green";
            code_to_short[0] = "GREEN";
        }
        name_to_code["KEY_YELLOW"] = 0;
        word_to_code["yellow"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "yellow";
            code_to_short[0] = "YELLOW";
        }
        name_to_code["KEY_BLUE"] = 0;
        word_to_code["blue"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "blue";
            code_to_short[0] = "BLUE";
        }
        name_to_code["KEY_CHANNELUP"] = 0;
        word_to_code["channelup"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "channelup";
            code_to_short[0] = "CHANNELUP";
        }
        name_to_code["KEY_CHANNELDOWN"] = 0;
        word_to_code["channeldown"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "channeldown";
            code_to_short[0] = "CHANNELDOWN";
        }
        name_to_code["KEY_FIRST"] = 0;
        word_to_code["first"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "first";
            code_to_short[0] = "FIRST";
        }
        name_to_code["KEY_LAST"] = 0;
        word_to_code["last"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "last";
            code_to_short[0] = "LAST";
        }
        name_to_code["KEY_AB"] = 0;
        word_to_code["ab"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "ab";
            code_to_short[0] = "AB";
        }
        name_to_code["KEY_NEXT"] = 0;
        word_to_code["next"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "next";
            code_to_short[0] = "NEXT";
        }
        name_to_code["KEY_RESTART"] = 0;
        word_to_code["restart"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "restart";
            code_to_short[0] = "RESTART";
        }
        name_to_code["KEY_SLOW"] = 0;
        word_to_code["slow"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "slow";
            code_to_short[0] = "SLOW";
        }
        name_to_code["KEY_SHUFFLE"] = 0;
        word_to_code["shuffle"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "shuffle";
            code_to_short[0] = "SHUFFLE";
        }
        name_to_code["KEY_BREAK"] = 0;
        word_to_code["break"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "break";
            code_to_short[0] = "BREAK";
        }
        name_to_code["KEY_PREVIOUS"] = 0;
        word_to_code["previous"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "previous";
            code_to_short[0] = "PREVIOUS";
        }
        name_to_code["KEY_DIGITS"] = 0;
        word_to_code["digits"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "digits";
            code_to_short[0] = "DIGITS";
        }
        name_to_code["KEY_TEEN"] = 0;
        word_to_code["teen"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "teen";
            code_to_short[0] = "TEEN";
        }
        name_to_code["KEY_TWEN"] = 0;
        word_to_code["twen"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "twen";
            code_to_short[0] = "TWEN";
        }
        name_to_code["KEY_VIDEOPHONE"] = 0;
        word_to_code["videophone"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "videophone";
            code_to_short[0] = "VIDEOPHONE";
        }
        name_to_code["KEY_GAMES"] = 0;
        word_to_code["games"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "games";
            code_to_short[0] = "GAMES";
        }
        name_to_code["KEY_ZOOMIN"] = 0;
        word_to_code["zoomin"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "zoomin";
            code_to_short[0] = "ZOOMIN";
        }
        name_to_code["KEY_ZOOMOUT"] = 0;
        word_to_code["zoomout"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "zoomout";
            code_to_short[0] = "ZOOMOUT";
        }
        name_to_code["KEY_ZOOMRESET"] = 0;
        word_to_code["zoomreset"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "zoomreset";
            code_to_short[0] = "ZOOMRESET";
        }
        name_to_code["KEY_WORDPROCESSOR"] = 0;
        word_to_code["wordprocessor"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "wordprocessor";
            code_to_short[0] = "WORDPROCESSOR";
        }
        name_to_code["KEY_EDITOR"] = 0;
        word_to_code["editor"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "editor";
            code_to_short[0] = "EDITOR";
        }
        name_to_code["KEY_SPREADSHEET"] = 0;
        word_to_code["spreadsheet"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "spreadsheet";
            code_to_short[0] = "SPREADSHEET";
        }
        name_to_code["KEY_GRAPHICSEDITOR"] = 0;
        word_to_code["graphicseditor"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "graphicseditor";
            code_to_short[0] = "GRAPHICSEDITOR";
        }
        name_to_code["KEY_PRESENTATION"] = 0;
        word_to_code["presentation"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "presentation";
            code_to_short[0] = "PRESENTATION";
        }
        name_to_code["KEY_DATABASE"] = 0;
        word_to_code["database"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "database";
            code_to_short[0] = "DATABASE";
        }
        name_to_code["KEY_NEWS"] = 0;
        word_to_code["news"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "news";
            code_to_short[0] = "NEWS";
        }
        name_to_code["KEY_VOICEMAIL"] = 0;
        word_to_code["voicemail"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "voicemail";
            code_to_short[0] = "VOICEMAIL";
        }
        name_to_code["KEY_ADDRESSBOOK"] = 0;
        word_to_code["addressbook"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "addressbook";
            code_to_short[0] = "ADDRESSBOOK";
        }
        name_to_code["KEY_MESSENGER"] = 0;
        word_to_code["messenger"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "messenger";
            code_to_short[0] = "MESSENGER";
        }
        name_to_code["KEY_DISPLAYTOGGLE"] = 0;
        word_to_code["displaytoggle"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "displaytoggle";
            code_to_short[0] = "DISPLAYTOGGLE";
        }
        name_to_code["KEY_SPELLCHECK"] = 0;
        word_to_code["spellcheck"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "spellcheck";
            code_to_short[0] = "SPELLCHECK";
        }
        name_to_code["KEY_LOGOFF"] = 0;
        word_to_code["logoff"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "logoff";
            code_to_short[0] = "LOGOFF";
        }
        name_to_code["KEY_DOLLAR"] = 0;
        word_to_code["dollar"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "dollar";
            code_to_short[0] = "DOLLAR";
        }
        name_to_code["KEY_EURO"] = 0;
        word_to_code["euro"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "euro";
            code_to_short[0] = "EURO";
        }
        name_to_code["KEY_FRAMEBACK"] = 0;
        word_to_code["frameback"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "frameback";
            code_to_short[0] = "FRAMEBACK";
        }
        name_to_code["KEY_FRAMEFORWARD"] = 0;
        word_to_code["frameforward"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "frameforward";
            code_to_short[0] = "FRAMEFORWARD";
        }
        name_to_code["KEY_CONTEXT_MENU"] = 0;
        word_to_code["context_menu"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "context_menu";
            code_to_short[0] = "CONTEXT_MENU";
        }
        name_to_code["KEY_MEDIA_REPEAT"] = 0;
        word_to_code["media_repeat"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "media_repeat";
            code_to_short[0] = "MEDIA_REPEAT";
        }
        name_to_code["KEY_10CHANNELSUP"] = 0;
        word_to_code["10channelsup"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "10channelsup";
            code_to_short[0] = "10CHANNELSUP";
        }
        name_to_code["KEY_10CHANNELSDOWN"] = 0;
        word_to_code["10channelsdown"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "10channelsdown";
            code_to_short[0] = "10CHANNELSDOWN";
        }
        name_to_code["KEY_IMAGES"] = 0;
        word_to_code["images"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "images";
            code_to_short[0] = "IMAGES";
        }
        name_to_code["KEY_NOTIFICATION_CENTER"] = 0;
        word_to_code["notification_center"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "notification_center";
            code_to_short[0] = "NOTIFICATION_CENTER";
        }
        name_to_code["KEY_PICKUP_PHONE"] = 0;
        word_to_code["pickup_phone"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "pickup_phone";
            code_to_short[0] = "PICKUP_PHONE";
        }
        name_to_code["KEY_HANGUP_PHONE"] = 0;
        word_to_code["hangup_phone"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "hangup_phone";
            code_to_short[0] = "HANGUP_PHONE";
        }
        name_to_code["KEY_LINK_PHONE"] = 0;
        word_to_code["link_phone"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "link_phone";
            code_to_short[0] = "LINK_PHONE";
        }
        name_to_code["KEY_DEL_EOL"] = 0;
        word_to_code["del_eol"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "del_eol";
            code_to_short[0] = "DEL_EOL";
        }
        name_to_code["KEY_DEL_EOS"] = 0;
        word_to_code["del_eos"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "del_eos";
            code_to_short[0] = "DEL_EOS";
        }
        name_to_code["KEY_INS_LINE"] = 0;
        word_to_code["ins_line"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "ins_line";
            code_to_short[0] = "INS_LINE";
        }
        name_to_code["KEY_DEL_LINE"] = 0;
        word_to_code["del_line"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "del_line";
            code_to_short[0] = "DEL_LINE";
        }
        name_to_code["KEY_FN"] = 0;
        word_to_code["fn"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "fn";
            code_to_short[0] = "FN";
        }
        name_to_code["KEY_FN_ESC"] = 0;
        word_to_code["fn_esc"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "fn_esc";
            code_to_short[0] = "FN_ESC";
        }
        name_to_code["KEY_FN_F1"] = 0;
        word_to_code["fn_f1"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "fn_f1";
            code_to_short[0] = "FN_F1";
        }
        name_to_code["KEY_FN_F2"] = 0;
        word_to_code["fn_f2"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "fn_f2";
            code_to_short[0] = "FN_F2";
        }
        name_to_code["KEY_FN_F3"] = 0;
        word_to_code["fn_f3"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "fn_f3";
            code_to_short[0] = "FN_F3";
        }
        name_to_code["KEY_FN_F4"] = 0;
        word_to_code["fn_f4"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "fn_f4";
            code_to_short[0] = "FN_F4";
        }
        name_to_code["KEY_FN_F5"] = 0;
        word_to_code["fn_f5"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "fn_f5";
            code_to_short[0] = "FN_F5";
        }
        name_to_code["KEY_FN_F6"] = 0;
        word_to_code["fn_f6"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "fn_f6";
            code_to_short[0] = "FN_F6";
        }
        name_to_code["KEY_FN_F7"] = 0;
        word_to_code["fn_f7"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "fn_f7";
            code_to_short[0] = "FN_F7";
        }
        name_to_code["KEY_FN_F8"] = 0;
        word_to_code["fn_f8"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "fn_f8";
            code_to_short[0] = "FN_F8";
        }
        name_to_code["KEY_FN_F9"] = 0;
        word_to_code["fn_f9"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "fn_f9";
            code_to_short[0] = "FN_F9";
        }
        name_to_code["KEY_FN_F10"] = 0;
        word_to_code["fn_f10"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "fn_f10";
            code_to_short[0] = "FN_F10";
        }
        name_to_code["KEY_FN_F11"] = 0;
        word_to_code["fn_f11"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "fn_f11";
            code_to_short[0] = "FN_F11";
        }
        name_to_code["KEY_FN_F12"] = 0;
        word_to_code["fn_f12"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "fn_f12";
            code_to_short[0] = "FN_F12";
        }
        name_to_code["KEY_FN_1"] = 0;
        word_to_code["fn_1"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "fn_1";
            code_to_short[0] = "FN_1";
        }
        name_to_code["KEY_FN_2"] = 0;
        word_to_code["fn_2"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "fn_2";
            code_to_short[0] = "FN_2";
        }
        name_to_code["KEY_FN_D"] = 0;
        word_to_code["fn_d"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "fn_d";
            code_to_short[0] = "FN_D";
        }
        name_to_code["KEY_FN_E"] = 0;
        word_to_code["fn_e"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "fn_e";
            code_to_short[0] = "FN_E";
        }
        name_to_code["KEY_FN_F"] = 0;
        word_to_code["fn_f"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "fn_f";
            code_to_short[0] = "FN_F";
        }
        name_to_code["KEY_FN_S"] = 0;
        word_to_code["fn_s"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "fn_s";
            code_to_short[0] = "FN_S";
        }
        name_to_code["KEY_FN_B"] = 0;
        word_to_code["fn_b"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "fn_b";
            code_to_short[0] = "FN_B";
        }
        name_to_code["KEY_FN_RIGHT_SHIFT"] = 0;
        word_to_code["fn_right_shift"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "fn_right_shift";
            code_to_short[0] = "FN_RIGHT_SHIFT";
        }
        name_to_code["KEY_BRL_DOT1"] = 0;
        word_to_code["brl_dot1"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "brl_dot1";
            code_to_short[0] = "BRL_DOT1";
        }
        name_to_code["KEY_BRL_DOT2"] = 0;
        word_to_code["brl_dot2"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "brl_dot2";
            code_to_short[0] = "BRL_DOT2";
        }
        name_to_code["KEY_BRL_DOT3"] = 0;
        word_to_code["brl_dot3"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "brl_dot3";
            code_to_short[0] = "BRL_DOT3";
        }
        name_to_code["KEY_BRL_DOT4"] = 0;
        word_to_code["brl_dot4"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "brl_dot4";
            code_to_short[0] = "BRL_DOT4";
        }
        name_to_code["KEY_BRL_DOT5"] = 0;
        word_to_code["brl_dot5"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "brl_dot5";
            code_to_short[0] = "BRL_DOT5";
        }
        name_to_code["KEY_BRL_DOT6"] = 0;
        word_to_code["brl_dot6"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "brl_dot6";
            code_to_short[0] = "BRL_DOT6";
        }
        name_to_code["KEY_BRL_DOT7"] = 0;
        word_to_code["brl_dot7"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "brl_dot7";
            code_to_short[0] = "BRL_DOT7";
        }
        name_to_code["KEY_BRL_DOT8"] = 0;
        word_to_code["brl_dot8"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "brl_dot8";
            code_to_short[0] = "BRL_DOT8";
        }
        name_to_code["KEY_BRL_DOT9"] = 0;
        word_to_code["brl_dot9"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "brl_dot9";
            code_to_short[0] = "BRL_DOT9";
        }
        name_to_code["KEY_BRL_DOT10"] = 0;
        word_to_code["brl_dot10"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "brl_dot10";
            code_to_short[0] = "BRL_DOT10";
        }
        name_to_code["KEY_NUMERIC_0"] = 0;
        word_to_code["numeric_0"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "numeric_0";
            code_to_short[0] = "NUMERIC_0";
        }
        name_to_code["KEY_NUMERIC_1"] = 0;
        word_to_code["numeric_1"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "numeric_1";
            code_to_short[0] = "NUMERIC_1";
        }
        name_to_code["KEY_NUMERIC_2"] = 0;
        word_to_code["numeric_2"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "numeric_2";
            code_to_short[0] = "NUMERIC_2";
        }
        name_to_code["KEY_NUMERIC_3"] = 0;
        word_to_code["numeric_3"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "numeric_3";
            code_to_short[0] = "NUMERIC_3";
        }
        name_to_code["KEY_NUMERIC_4"] = 0;
        word_to_code["numeric_4"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "numeric_4";
            code_to_short[0] = "NUMERIC_4";
        }
        name_to_code["KEY_NUMERIC_5"] = 0;
        word_to_code["numeric_5"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "numeric_5";
            code_to_short[0] = "NUMERIC_5";
        }
        name_to_code["KEY_NUMERIC_6"] = 0;
        word_to_code["numeric_6"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "numeric_6";
            code_to_short[0] = "NUMERIC_6";
        }
        name_to_code["KEY_NUMERIC_7"] = 0;
        word_to_code["numeric_7"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "numeric_7";
            code_to_short[0] = "NUMERIC_7";
        }
        name_to_code["KEY_NUMERIC_8"] = 0;
        word_to_code["numeric_8"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "numeric_8";
            code_to_short[0] = "NUMERIC_8";
        }
        name_to_code["KEY_NUMERIC_9"] = 0;
        word_to_code["numeric_9"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "numeric_9";
            code_to_short[0] = "NUMERIC_9";
        }
        name_to_code["KEY_NUMERIC_STAR"] = 0;
        word_to_code["numeric_star"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "numeric_star";
            code_to_short[0] = "NUMERIC_STAR";
        }
        name_to_code["KEY_NUMERIC_POUND"] = 0;
        word_to_code["numeric_pound"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "numeric_pound";
            code_to_short[0] = "NUMERIC_POUND";
        }
        name_to_code["KEY_NUMERIC_A"] = 0;
        word_to_code["numeric_a"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "numeric_a";
            code_to_short[0] = "NUMERIC_A";
        }
        name_to_code["KEY_NUMERIC_B"] = 0;
        word_to_code["numeric_b"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "numeric_b";
            code_to_short[0] = "NUMERIC_B";
        }
        name_to_code["KEY_NUMERIC_C"] = 0;
        word_to_code["numeric_c"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "numeric_c";
            code_to_short[0] = "NUMERIC_C";
        }
        name_to_code["KEY_NUMERIC_D"] = 0;
        word_to_code["numeric_d"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "numeric_d";
            code_to_short[0] = "NUMERIC_D";
        }
        name_to_code["KEY_CAMERA_FOCUS"] = 0;
        word_to_code["camera_focus"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "camera_focus";
            code_to_short[0] = "CAMERA_FOCUS";
        }
        name_to_code["KEY_WPS_BUTTON"] = 0;
        word_to_code["wps_button"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "wps_button";
            code_to_short[0] = "WPS_BUTTON";
        }
        name_to_code["KEY_TOUCHPAD_TOGGLE"] = 0;
        word_to_code["touchpad_toggle"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "touchpad_toggle";
            code_to_short[0] = "TOUCHPAD_TOGGLE";
        }
        name_to_code["KEY_TOUCHPAD_ON"] = 0;
        word_to_code["touchpad_on"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "touchpad_on";
            code_to_short[0] = "TOUCHPAD_ON";
        }
        name_to_code["KEY_TOUCHPAD_OFF"] = 0;
        word_to_code["touchpad_off"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "touchpad_off";
            code_to_short[0] = "TOUCHPAD_OFF";
        }
        name_to_code["KEY_CAMERA_ZOOMIN"] = 0;
        word_to_code["camera_zoomin"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "camera_zoomin";
            code_to_short[0] = "CAMERA_ZOOMIN";
        }
        name_to_code["KEY_CAMERA_ZOOMOUT"] = 0;
        word_to_code["camera_zoomout"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "camera_zoomout";
            code_to_short[0] = "CAMERA_ZOOMOUT";
        }
        name_to_code["KEY_CAMERA_UP"] = 0;
        word_to_code["camera_up"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "camera_up";
            code_to_short[0] = "CAMERA_UP";
        }
        name_to_code["KEY_CAMERA_DOWN"] = 0;
        word_to_code["camera_down"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "camera_down";
            code_to_short[0] = "CAMERA_DOWN";
        }
        name_to_code["KEY_CAMERA_LEFT"] = 0;
        word_to_code["camera_left"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "camera_left";
            code_to_short[0] = "CAMERA_LEFT";
        }
        name_to_code["KEY_CAMERA_RIGHT"] = 0;
        word_to_code["camera_right"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "camera_right";
            code_to_short[0] = "CAMERA_RIGHT";
        }
        name_to_code["KEY_ATTENDANT_ON"] = 0;
        word_to_code["attendant_on"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "attendant_on";
            code_to_short[0] = "ATTENDANT_ON";
        }
        name_to_code["KEY_ATTENDANT_OFF"] = 0;
        word_to_code["attendant_off"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "attendant_off";
            code_to_short[0] = "ATTENDANT_OFF";
        }
        name_to_code["KEY_ATTENDANT_TOGGLE"] = 0;
        word_to_code["attendant_toggle"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "attendant_toggle";
            code_to_short[0] = "ATTENDANT_TOGGLE";
        }
        name_to_code["KEY_LIGHTS_TOGGLE"] = 0;
        word_to_code["lights_toggle"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "lights_toggle";
            code_to_short[0] = "LIGHTS_TOGGLE";
        }
        name_to_code["KEY_ALS_TOGGLE"] = 0;
        word_to_code["als_toggle"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "als_toggle";
            code_to_short[0] = "ALS_TOGGLE";
        }
        name_to_code["KEY_ROTATE_LOCK_TOGGLE"] = 0;
        word_to_code["rotate_lock_toggle"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "rotate_lock_toggle";
            code_to_short[0] = "ROTATE_LOCK_TOGGLE";
        }
        name_to_code["KEY_REFRESH_RATE_TOGGLE"] = 0;
        word_to_code["refresh_rate_toggle"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "refresh_rate_toggle";
            code_to_short[0] = "REFRESH_RATE_TOGGLE";
        }
        name_to_code["KEY_BUTTONCONFIG"] = 0;
        word_to_code["buttonconfig"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "buttonconfig";
            code_to_short[0] = "BUTTONCONFIG";
        }
        name_to_code["KEY_TASKMANAGER"] = 0;
        word_to_code["taskmanager"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "taskmanager";
            code_to_short[0] = "TASKMANAGER";
        }
        name_to_code["KEY_JOURNAL"] = 0;
        word_to_code["journal"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "journal";
            code_to_short[0] = "JOURNAL";
        }
        name_to_code["KEY_CONTROLPANEL"] = 0;
        word_to_code["controlpanel"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "controlpanel";
            code_to_short[0] = "CONTROLPANEL";
        }
        name_to_code["KEY_APPSELECT"] = 0;
        word_to_code["appselect"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "appselect";
            code_to_short[0] = "APPSELECT";
        }
        name_to_code["KEY_SCREENSAVER"] = 0;
        word_to_code["screensaver"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "screensaver";
            code_to_short[0] = "SCREENSAVER";
        }
        name_to_code["KEY_VOICECOMMAND"] = 0;
        word_to_code["voicecommand"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "voicecommand";
            code_to_short[0] = "VOICECOMMAND";
        }
        name_to_code["KEY_ASSISTANT"] = 0;
        word_to_code["assistant"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "assistant";
            code_to_short[0] = "ASSISTANT";
        }
        name_to_code["KEY_KBD_LAYOUT_NEXT"] = 0;
        word_to_code["kbd_layout_next"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "kbd_layout_next";
            code_to_short[0] = "KBD_LAYOUT_NEXT";
        }
        name_to_code["KEY_EMOJI_PICKER"] = 0;
        word_to_code["emoji_picker"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "emoji_picker";
            code_to_short[0] = "EMOJI_PICKER";
        }
        name_to_code["KEY_DICTATE"] = 0;
        word_to_code["dictate"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "dictate";
            code_to_short[0] = "DICTATE";
        }
        name_to_code["KEY_CAMERA_ACCESS_ENABLE"] = 0;
        word_to_code["camera_access_enable"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "camera_access_enable";
            code_to_short[0] = "CAMERA_ACCESS_ENABLE";
        }
        name_to_code["KEY_CAMERA_ACCESS_DISABLE"] = 0;
        word_to_code["camera_access_disable"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "camera_access_disable";
            code_to_short[0] = "CAMERA_ACCESS_DISABLE";
        }
        name_to_code["KEY_CAMERA_ACCESS_TOGGLE"] = 0;
        word_to_code["camera_access_toggle"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "camera_access_toggle";
            code_to_short[0] = "CAMERA_ACCESS_TOGGLE";
        }
        name_to_code["KEY_ACCESSIBILITY"] = 0;
        word_to_code["accessibility"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "accessibility";
            code_to_short[0] = "ACCESSIBILITY";
        }
        name_to_code["KEY_DO_NOT_DISTURB"] = 0;
        word_to_code["do_not_disturb"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "do_not_disturb";
            code_to_short[0] = "DO_NOT_DISTURB";
        }
        name_to_code["KEY_BRIGHTNESS_MIN"] = 0;
        word_to_code["brightness_min"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "brightness_min";
            code_to_short[0] = "BRIGHTNESS_MIN";
        }
        name_to_code["KEY_BRIGHTNESS_MAX"] = 0;
        word_to_code["brightness_max"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "brightness_max";
            code_to_short[0] = "BRIGHTNESS_MAX";
        }
        name_to_code["KEY_EPRIVACY_SCREEN_ON"] = 0;
        word_to_code["eprivacy_screen_on"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "eprivacy_screen_on";
            code_to_short[0] = "EPRIVACY_SCREEN_ON";
        }
        name_to_code["KEY_EPRIVACY_SCREEN_OFF"] = 0;
        word_to_code["eprivacy_screen_off"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "eprivacy_screen_off";
            code_to_short[0] = "EPRIVACY_SCREEN_OFF";
        }
        name_to_code["KEY_ACTION_ON_SELECTION"] = 0;
        word_to_code["action_on_selection"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "action_on_selection";
            code_to_short[0] = "ACTION_ON_SELECTION";
        }
        name_to_code["KEY_CONTEXTUAL_INSERT"] = 0;
        word_to_code["contextual_insert"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "contextual_insert";
            code_to_short[0] = "CONTEXTUAL_INSERT";
        }
        name_to_code["KEY_CONTEXTUAL_QUERY"] = 0;
        word_to_code["contextual_query"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "contextual_query";
            code_to_short[0] = "CONTEXTUAL_QUERY";
        }
        name_to_code["KEY_KBDINPUTASSIST_PREV"] = 0;
        word_to_code["kbdinputassist_prev"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "kbdinputassist_prev";
            code_to_short[0] = "KBDINPUTASSIST_PREV";
        }
        name_to_code["KEY_KBDINPUTASSIST_NEXT"] = 0;
        word_to_code["kbdinputassist_next"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "kbdinputassist_next";
            code_to_short[0] = "KBDINPUTASSIST_NEXT";
        }
        name_to_code["KEY_KBDINPUTASSIST_PREVGROUP"] = 0;
        word_to_code["kbdinputassist_prevgroup"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "kbdinputassist_prevgroup";
            code_to_short[0] = "KBDINPUTASSIST_PREVGROUP";
        }
        name_to_code["KEY_KBDINPUTASSIST_NEXTGROUP"] = 0;
        word_to_code["kbdinputassist_nextgroup"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "kbdinputassist_nextgroup";
            code_to_short[0] = "KBDINPUTASSIST_NEXTGROUP";
        }
        name_to_code["KEY_KBDINPUTASSIST_ACCEPT"] = 0;
        word_to_code["kbdinputassist_accept"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "kbdinputassist_accept";
            code_to_short[0] = "KBDINPUTASSIST_ACCEPT";
        }
        name_to_code["KEY_KBDINPUTASSIST_CANCEL"] = 0;
        word_to_code["kbdinputassist_cancel"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "kbdinputassist_cancel";
            code_to_short[0] = "KBDINPUTASSIST_CANCEL";
        }
        name_to_code["KEY_RIGHT_UP"] = 0;
        word_to_code["right_up"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "right_up";
            code_to_short[0] = "RIGHT_UP";
        }
        name_to_code["KEY_RIGHT_DOWN"] = 0;
        word_to_code["right_down"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "right_down";
            code_to_short[0] = "RIGHT_DOWN";
        }
        name_to_code["KEY_LEFT_UP"] = 0;
        word_to_code["left_up"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "left_up";
            code_to_short[0] = "LEFT_UP";
        }
        name_to_code["KEY_LEFT_DOWN"] = 0;
        word_to_code["left_down"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "left_down";
            code_to_short[0] = "LEFT_DOWN";
        }
        name_to_code["KEY_ROOT_MENU"] = 0;
        word_to_code["root_menu"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "root_menu";
            code_to_short[0] = "ROOT_MENU";
        }
        name_to_code["KEY_MEDIA_TOP_MENU"] = 0;
        word_to_code["media_top_menu"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "media_top_menu";
            code_to_short[0] = "MEDIA_TOP_MENU";
        }
        name_to_code["KEY_NUMERIC_11"] = 0;
        word_to_code["numeric_11"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "numeric_11";
            code_to_short[0] = "NUMERIC_11";
        }
        name_to_code["KEY_NUMERIC_12"] = 0;
        word_to_code["numeric_12"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "numeric_12";
            code_to_short[0] = "NUMERIC_12";
        }
        name_to_code["KEY_AUDIO_DESC"] = 0;
        word_to_code["audio_desc"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "audio_desc";
            code_to_short[0] = "AUDIO_DESC";
        }
        name_to_code["KEY_3D_MODE"] = 0;
        word_to_code["3d_mode"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "3d_mode";
            code_to_short[0] = "3D_MODE";
        }
        name_to_code["KEY_NEXT_FAVORITE"] = 0;
        word_to_code["next_favorite"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "next_favorite";
            code_to_short[0] = "NEXT_FAVORITE";
        }
        name_to_code["KEY_STOP_RECORD"] = 0;
        word_to_code["stop_record"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "stop_record";
            code_to_short[0] = "STOP_RECORD";
        }
        name_to_code["KEY_PAUSE_RECORD"] = 0;
        word_to_code["pause_record"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "pause_record";
            code_to_short[0] = "PAUSE_RECORD";
        }
        name_to_code["KEY_VOD"] = 0;
        word_to_code["vod"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "vod";
            code_to_short[0] = "VOD";
        }
        name_to_code["KEY_UNMUTE"] = 0;
        word_to_code["unmute"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "unmute";
            code_to_short[0] = "UNMUTE";
        }
        name_to_code["KEY_FASTREVERSE"] = 0;
        word_to_code["fastreverse"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "fastreverse";
            code_to_short[0] = "FASTREVERSE";
        }
        name_to_code["KEY_SLOWREVERSE"] = 0;
        word_to_code["slowreverse"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "slowreverse";
            code_to_short[0] = "SLOWREVERSE";
        }
        name_to_code["KEY_DATA"] = 0;
        word_to_code["data"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "data";
            code_to_short[0] = "DATA";
        }
        name_to_code["KEY_ONSCREEN_KEYBOARD"] = 0;
        word_to_code["onscreen_keyboard"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "onscreen_keyboard";
            code_to_short[0] = "ONSCREEN_KEYBOARD";
        }
        name_to_code["KEY_PRIVACY_SCREEN_TOGGLE"] = 0;
        word_to_code["privacy_screen_toggle"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "privacy_screen_toggle";
            code_to_short[0] = "PRIVACY_SCREEN_TOGGLE";
        }
        name_to_code["KEY_SELECTIVE_SCREENSHOT"] = 0;
        word_to_code["selective_screenshot"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "selective_screenshot";
            code_to_short[0] = "SELECTIVE_SCREENSHOT";
        }
        name_to_code["KEY_NEXT_ELEMENT"] = 0;
        word_to_code["next_element"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "next_element";
            code_to_short[0] = "NEXT_ELEMENT";
        }
        name_to_code["KEY_PREVIOUS_ELEMENT"] = 0;
        word_to_code["previous_element"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "previous_element";
            code_to_short[0] = "PREVIOUS_ELEMENT";
        }
        name_to_code["KEY_AUTOPILOT_ENGAGE_TOGGLE"] = 0;
        word_to_code["autopilot_engage_toggle"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "autopilot_engage_toggle";
            code_to_short[0] = "AUTOPILOT_ENGAGE_TOGGLE";
        }
        name_to_code["KEY_MARK_WAYPOINT"] = 0;
        word_to_code["mark_waypoint"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "mark_waypoint";
            code_to_short[0] = "MARK_WAYPOINT";
        }
        name_to_code["KEY_SOS"] = 0;
        word_to_code["sos"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "sos";
            code_to_short[0] = "SOS";
        }
        name_to_code["KEY_NAV_CHART"] = 0;
        word_to_code["nav_chart"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "nav_chart";
            code_to_short[0] = "NAV_CHART";
        }
        name_to_code["KEY_FISHING_CHART"] = 0;
        word_to_code["fishing_chart"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "fishing_chart";
            code_to_short[0] = "FISHING_CHART";
        }
        name_to_code["KEY_SINGLE_RANGE_RADAR"] = 0;
        word_to_code["single_range_radar"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "single_range_radar";
            code_to_short[0] = "SINGLE_RANGE_RADAR";
        }
        name_to_code["KEY_DUAL_RANGE_RADAR"] = 0;
        word_to_code["dual_range_radar"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "dual_range_radar";
            code_to_short[0] = "DUAL_RANGE_RADAR";
        }
        name_to_code["KEY_RADAR_OVERLAY"] = 0;
        word_to_code["radar_overlay"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "radar_overlay";
            code_to_short[0] = "RADAR_OVERLAY";
        }
        name_to_code["KEY_TRADITIONAL_SONAR"] = 0;
        word_to_code["traditional_sonar"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "traditional_sonar";
            code_to_short[0] = "TRADITIONAL_SONAR";
        }
        name_to_code["KEY_CLEARVU_SONAR"] = 0;
        word_to_code["clearvu_sonar"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "clearvu_sonar";
            code_to_short[0] = "CLEARVU_SONAR";
        }
        name_to_code["KEY_SIDEVU_SONAR"] = 0;
        word_to_code["sidevu_sonar"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "sidevu_sonar";
            code_to_short[0] = "SIDEVU_SONAR";
        }
        name_to_code["KEY_NAV_INFO"] = 0;
        word_to_code["nav_info"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "nav_info";
            code_to_short[0] = "NAV_INFO";
        }
        name_to_code["KEY_BRIGHTNESS_MENU"] = 0;
        word_to_code["brightness_menu"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "brightness_menu";
            code_to_short[0] = "BRIGHTNESS_MENU";
        }
        name_to_code["KEY_MACRO1"] = 0;
        word_to_code["macro1"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro1";
            code_to_short[0] = "MACRO1";
        }
        name_to_code["KEY_MACRO2"] = 0;
        word_to_code["macro2"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro2";
            code_to_short[0] = "MACRO2";
        }
        name_to_code["KEY_MACRO3"] = 0;
        word_to_code["macro3"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro3";
            code_to_short[0] = "MACRO3";
        }
        name_to_code["KEY_MACRO4"] = 0;
        word_to_code["macro4"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro4";
            code_to_short[0] = "MACRO4";
        }
        name_to_code["KEY_MACRO5"] = 0;
        word_to_code["macro5"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro5";
            code_to_short[0] = "MACRO5";
        }
        name_to_code["KEY_MACRO6"] = 0;
        word_to_code["macro6"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro6";
            code_to_short[0] = "MACRO6";
        }
        name_to_code["KEY_MACRO7"] = 0;
        word_to_code["macro7"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro7";
            code_to_short[0] = "MACRO7";
        }
        name_to_code["KEY_MACRO8"] = 0;
        word_to_code["macro8"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro8";
            code_to_short[0] = "MACRO8";
        }
        name_to_code["KEY_MACRO9"] = 0;
        word_to_code["macro9"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro9";
            code_to_short[0] = "MACRO9";
        }
        name_to_code["KEY_MACRO10"] = 0;
        word_to_code["macro10"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro10";
            code_to_short[0] = "MACRO10";
        }
        name_to_code["KEY_MACRO11"] = 0;
        word_to_code["macro11"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro11";
            code_to_short[0] = "MACRO11";
        }
        name_to_code["KEY_MACRO12"] = 0;
        word_to_code["macro12"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro12";
            code_to_short[0] = "MACRO12";
        }
        name_to_code["KEY_MACRO13"] = 0;
        word_to_code["macro13"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro13";
            code_to_short[0] = "MACRO13";
        }
        name_to_code["KEY_MACRO14"] = 0;
        word_to_code["macro14"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro14";
            code_to_short[0] = "MACRO14";
        }
        name_to_code["KEY_MACRO15"] = 0;
        word_to_code["macro15"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro15";
            code_to_short[0] = "MACRO15";
        }
        name_to_code["KEY_MACRO16"] = 0;
        word_to_code["macro16"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro16";
            code_to_short[0] = "MACRO16";
        }
        name_to_code["KEY_MACRO17"] = 0;
        word_to_code["macro17"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro17";
            code_to_short[0] = "MACRO17";
        }
        name_to_code["KEY_MACRO18"] = 0;
        word_to_code["macro18"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro18";
            code_to_short[0] = "MACRO18";
        }
        name_to_code["KEY_MACRO19"] = 0;
        word_to_code["macro19"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro19";
            code_to_short[0] = "MACRO19";
        }
        name_to_code["KEY_MACRO20"] = 0;
        word_to_code["macro20"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro20";
            code_to_short[0] = "MACRO20";
        }
        name_to_code["KEY_MACRO21"] = 0;
        word_to_code["macro21"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro21";
            code_to_short[0] = "MACRO21";
        }
        name_to_code["KEY_MACRO22"] = 0;
        word_to_code["macro22"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro22";
            code_to_short[0] = "MACRO22";
        }
        name_to_code["KEY_MACRO23"] = 0;
        word_to_code["macro23"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro23";
            code_to_short[0] = "MACRO23";
        }
        name_to_code["KEY_MACRO24"] = 0;
        word_to_code["macro24"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro24";
            code_to_short[0] = "MACRO24";
        }
        name_to_code["KEY_MACRO25"] = 0;
        word_to_code["macro25"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro25";
            code_to_short[0] = "MACRO25";
        }
        name_to_code["KEY_MACRO26"] = 0;
        word_to_code["macro26"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro26";
            code_to_short[0] = "MACRO26";
        }
        name_to_code["KEY_MACRO27"] = 0;
        word_to_code["macro27"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro27";
            code_to_short[0] = "MACRO27";
        }
        name_to_code["KEY_MACRO28"] = 0;
        word_to_code["macro28"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro28";
            code_to_short[0] = "MACRO28";
        }
        name_to_code["KEY_MACRO29"] = 0;
        word_to_code["macro29"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro29";
            code_to_short[0] = "MACRO29";
        }
        name_to_code["KEY_MACRO30"] = 0;
        word_to_code["macro30"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro30";
            code_to_short[0] = "MACRO30";
        }
        name_to_code["KEY_MACRO_RECORD_START"] = 0;
        word_to_code["macro_record_start"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro_record_start";
            code_to_short[0] = "MACRO_RECORD_START";
        }
        name_to_code["KEY_MACRO_RECORD_STOP"] = 0;
        word_to_code["macro_record_stop"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro_record_stop";
            code_to_short[0] = "MACRO_RECORD_STOP";
        }
        name_to_code["KEY_MACRO_PRESET_CYCLE"] = 0;
        word_to_code["macro_preset_cycle"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro_preset_cycle";
            code_to_short[0] = "MACRO_PRESET_CYCLE";
        }
        name_to_code["KEY_MACRO_PRESET1"] = 0;
        word_to_code["macro_preset1"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro_preset1";
            code_to_short[0] = "MACRO_PRESET1";
        }
        name_to_code["KEY_MACRO_PRESET2"] = 0;
        word_to_code["macro_preset2"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro_preset2";
            code_to_short[0] = "MACRO_PRESET2";
        }
        name_to_code["KEY_MACRO_PRESET3"] = 0;
        word_to_code["macro_preset3"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "macro_preset3";
            code_to_short[0] = "MACRO_PRESET3";
        }
        name_to_code["KEY_KBD_LCD_MENU1"] = 0;
        word_to_code["kbd_lcd_menu1"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "kbd_lcd_menu1";
            code_to_short[0] = "KBD_LCD_MENU1";
        }
        name_to_code["KEY_KBD_LCD_MENU2"] = 0;
        word_to_code["kbd_lcd_menu2"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "kbd_lcd_menu2";
            code_to_short[0] = "KBD_LCD_MENU2";
        }
        name_to_code["KEY_KBD_LCD_MENU3"] = 0;
        word_to_code["kbd_lcd_menu3"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "kbd_lcd_menu3";
            code_to_short[0] = "KBD_LCD_MENU3";
        }
        name_to_code["KEY_KBD_LCD_MENU4"] = 0;
        word_to_code["kbd_lcd_menu4"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "kbd_lcd_menu4";
            code_to_short[0] = "KBD_LCD_MENU4";
        }
        name_to_code["KEY_KBD_LCD_MENU5"] = 0;
        word_to_code["kbd_lcd_menu5"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "kbd_lcd_menu5";
            code_to_short[0] = "KBD_LCD_MENU5";
        }
        name_to_code["KEY_PERFORMANCE"] = 0;
        word_to_code["performance"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "performance";
            code_to_short[0] = "PERFORMANCE";
        }
        name_to_code["KEY_MAX"] = 0;
        word_to_code["max"] = 0;
        if (code_to_word.find(0) == code_to_word.end()) {
            code_to_word[0] = "max";
            code_to_short[0] = "MAX";
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
