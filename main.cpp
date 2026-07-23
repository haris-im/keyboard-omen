#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include <map>
#include <string>
#include <cmath>
#include "layout.h"

struct LightWave { float x, y, currentRadius, maxRadius, intensity; };
struct PhysicalKey { int x, y, width, height; std::wstring displayChar; float currentBrightness; };

std::map<DWORD, PhysicalKey> g_KeyboardGrid;
std::map<DWORD, bool> g_KeysPressedState;
std::vector<LightWave> g_Waves;
int g_ScreenWidth = 0, g_ScreenHeight = 0;
HWND g_hWnd = NULL; HHOOK g_KeyboardHook = NULL;

void BuildFullKeyboardLayout() {
    int layoutHeight = (g_ScreenHeight * 20) / 100; 
    int startY = g_ScreenHeight - layoutHeight - 60;
    int rowHeight = layoutHeight / 5, colWidth = g_ScreenWidth / 16; 
    auto rows = GetLayoutRows();

    for (size_t r = 0; r < rows.size(); ++r) {
        int currentX = colWidth / 2;
        int posY = startY + (r * rowHeight) + (rowHeight / 2);
        for (size_t c = 0; c < rows[r].size(); ++c) {
            DWORD vk = rows[r][c];
            int kWidth = colWidth - 12;
            if (vk == VK_SPACE) kWidth = colWidth * 5;
            else if (vk == VK_LSHIFT || vk == VK_RSHIFT || vk == VK_RETURN || vk == VK_BACK) kWidth = colWidth * 2;
            else if (vk == VK_TAB || vk == VK_CAPITAL) kWidth = (colWidth * 3) / 2;

            std::wstring label = L"";
            if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) label += (wchar_t)vk;
            else if (vk == VK_SPACE) label = L"SPACE";
            else if (vk == VK_RETURN) label = L"ENTER";
            else if (vk == VK_BACK) label = L"BCKSP";
            else if (vk == VK_LSHIFT || vk == VK_RSHIFT) label = L"SHIFT";
            else {
                BYTE state[256] = {0}; wchar_t buf[4] = {0};
                if (ToUnicode(vk, MapVirtualKeyW(vk, MAPVK_VK_TO_VSC), state, buf, 4, 0) > 0) label = buf;
                else label = L"•";
            }

            PhysicalKey pk = { currentX + (kWidth / 2), posY, kWidth, rowHeight - 12, label, 0.0f };
            g_KeyboardGrid[vk] = pk;
            currentX += kWidth + 12;
        }
    }
}

LRESULT CALLBACK KeyboardProc(int n, WPARAM wp, LPARAM lp) {
    if (n >= 0) {
        DWORD vk = ((KBDLLHOOKSTRUCT*)lp)->vkCode;
        if (g_KeyboardGrid.find(vk) != g_KeyboardGrid.end()) {
            
            if (wp == WM_KEYDOWN || wp == WM_SYSKEYDOWN) {
                if (!g_KeysPressedState[vk]) {
                    g_KeysPressedState[vk] = true;
                    g_KeyboardGrid[vk].currentBrightness = 1.0f;
                    
                    g_Waves.push_back({ (float)g_KeyboardGrid[vk].x, (float)g_KeyboardGrid[vk].y, 0.0f, 80.0f, 0.5f });
                    InvalidateRect(g_hWnd, NULL, TRUE);
                }
            }

            else if (wp == WM_KEYUP || wp == WM_SYSKEYUP) {
                g_KeysPressedState[vk] = false;
            }
        }
    }
    return CallNextHookEx(g_KeyboardHook, n, wp, lp);
}

LRESULT CALLBACK WndProc(HWND hw, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps; HDC hdc = BeginPaint(hw, &ps);
        HDC mDC = CreateCompatibleDC(hdc);
        HBITMAP mBM = CreateCompatibleBitmap(hdc, g_ScreenWidth, g_ScreenHeight);
        SelectObject(mDC, mBM);
        HBRUSH hBG = CreateSolidBrush(RGB(0, 0, 0)); RECT r = { 0, 0, g_ScreenWidth, g_ScreenHeight };
        FillRect(mDC, &r, hBG); DeleteObject(hBG); SetBkMode(mDC, TRANSPARENT);

        for (const auto& pair : g_KeyboardGrid) {
            const auto& k = pair.second;
            if (k.currentBrightness <= 0.01f) continue;

            int b = static_cast<int>(255.0f * k.currentBrightness);
            COLORREF c = RGB(0, b, b); HPEN hP = CreatePen(PS_SOLID, 3, c);
            SelectObject(mDC, hP); SelectObject(mDC, GetStockObject(NULL_BRUSH));
            RoundRect(mDC, k.x - (k.width / 2), k.y - (k.height / 2), k.x + (k.width / 2), k.y + (k.height / 2), 14, 14);
            SetTextColor(mDC, c); 
            
            HFONT hF = CreateFontW(28, 0, 0, 0, FW_BOLD, 0, 0, 0, 1, 0, 0, 2, 0, L"Segoe UI");
            SelectObject(mDC, hF); RECT tR = { k.x - (k.width / 2), k.y - 14, k.x + (k.width / 2), k.y + 14 };
            DrawTextW(mDC, k.displayChar.c_str(), -1, &tR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            DeleteObject(hP); DeleteObject(hF);
        }
        BitBlt(hdc, 0, 0, g_ScreenWidth, g_ScreenHeight, mDC, 0, 0, SRCCOPY);
        DeleteObject(mBM); DeleteDC(mDC); EndPaint(hw, &ps); return 0;
    }
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProc(hw, msg, wp, lp);
}

void PhysicsAnimationThreadLoop() {
    while (true) {
        bool draw = false;
        for (auto it = g_Waves.begin(); it != g_Waves.end();) {
            draw = true; it->currentRadius += 18.0f; it->intensity -= 0.06f;
            if (it->intensity <= 0 || it->currentRadius >= it->maxRadius) it = g_Waves.erase(it); else ++it;
        }
        for (auto& pair : g_KeyboardGrid) {
            DWORD vk = pair.first;
            auto& k = pair.second; bool hit = false;
            
            if (g_KeysPressedState[vk]) {
                k.currentBrightness = 1.0f;
                draw = true;
                continue; 
            }

            for (const auto& w : g_Waves) {
                float dist = std::hypot(k.x - w.x, k.y - w.y);
                if (dist >= w.currentRadius - 60.0f && dist <= w.currentRadius + 60.0f) {
                    float val = w.intensity * (1.0f - (std::abs(dist - w.currentRadius) / 60.0f));
                    if (val > k.currentBrightness) { k.currentBrightness = val; hit = true; }
                }
            }
            if (!hit && k.currentBrightness > 0.0f) { 
                k.currentBrightness -= 0.06f; 
                if (k.currentBrightness < 0.0f) k.currentBrightness = 0.0f; 
                draw = true; 
            }
        }
        if (draw && g_hWnd) InvalidateRect(g_hWnd, NULL, FALSE);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

int main() {
    g_ScreenWidth = GetSystemMetrics(SM_CXSCREEN); g_ScreenHeight = GetSystemMetrics(SM_CYSCREEN);
    BuildFullKeyboardLayout();

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, GetModuleHandle(NULL), NULL, LoadCursor(NULL, IDC_ARROW), NULL, NULL, L"SentifulKB", NULL };

    RegisterClassExW(&wc);

    g_hWnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE, L"SentifulKB", L"Engine", WS_POPUP, 0, 0, g_ScreenWidth, g_ScreenHeight, NULL, NULL, GetModuleHandle(NULL), NULL);

    SetLayeredWindowAttributes(g_hWnd, RGB(0, 0, 0), 0, LWA_COLORKEY); ShowWindow(g_hWnd, SW_SHOW);
    g_KeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);

    std::thread(PhysicsAnimationThreadLoop).detach();
    MSG msg; 
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg); DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(g_KeyboardHook); 
    return 0;
}
