#pragma once
#include <windows.h>
#include <vector>

inline std::vector<std::vector<DWORD>> GetLayoutRows() {
    return {
        { VK_ESCAPE, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', VK_OEM_MINUS, VK_OEM_PLUS, VK_BACK },
        { VK_TAB, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', VK_OEM_4, VK_OEM_6, VK_OEM_5 },
        { VK_CAPITAL, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', VK_OEM_1, VK_OEM_7, VK_RETURN },
        { VK_LSHIFT, 'Z', 'X', 'C', 'V', 'B', 'N', 'M', VK_OEM_COMMA, VK_OEM_PERIOD, VK_OEM_2, VK_RSHIFT },
        { VK_LCONTROL, VK_LWIN, VK_LMENU, VK_SPACE, VK_RMENU, VK_RCONTROL }
    };
}
