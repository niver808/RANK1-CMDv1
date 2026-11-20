#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dwmapi.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <cstdio>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_CAPTION_COLOR
#define DWMWA_CAPTION_COLOR 35
#endif

// ============================================================================
// COLOR PALETTE
// ============================================================================
static const COLORREF C_BG      = RGB(6, 5, 8);
static const COLORREF C_WHITE   = RGB(215, 215, 220);
static const COLORREF C_GREEN   = RGB(40, 200, 75);
static const COLORREF C_DIM     = RGB(75, 75, 88);
static const COLORREF C_ORAN    = RGB(255, 80, 20);
static const COLORREF C_SPINNER = RGB(255, 70, 20);

static const COLORREF LOGO_COLORS[7] = {
    RGB(255, 120, 120),
    RGB(255, 75, 75),
    RGB(240, 45, 45),
    RGB(215, 25, 25),
    RGB(185, 12, 12),
    RGB(145, 5, 5),
    RGB(100, 2, 2)
};

// ============================================================================
// CONSTANTS & STRINGS (\u escapes for MSVC /utf-8 safety)
// ============================================================================
static const int WIN_W = 630;
static const int WIN_H = 560;

static const UINT_PTR TIMER_TITLE   = 1;
static const UINT_PTR TIMER_BLINK   = 2;
static const UINT_PTR TIMER_SPINNER = 3;

enum AppState {
    S_MENU = 0,
    S_RUN  = 1,
    S_DONE = 2
};

static const wchar_t* TITLE_PHRASES[5] = {
    L"RANK1 | \u30E9\u30F3\u30AF1\u30B7\u30E7\u30C3\u30D7\u3088\u308A\u611B\u3092\u8FBC\u3081\u3066 \u2665",
    L"RANK1 | \u611B\u3057\u3066\u308B\u3001\u30E9\u30F3\u30AF1 \u2665",
    L"RANK1 | rank1shop \u304B\u3089\u3042\u306A\u305F\u3078 \u2665",
    L"RANK1 | \u6C38\u9060\u306B\u3042\u308A\u304C\u3068\u3046 \u2665 rank1shop",
    L"RANK1 | \u6700\u9AD8\u306E\u30C4\u30FC\u30EB\u3001\u6700\u9AD8\u306E\u611B \u2665"
};

// Block art characters:
// \u2588 = █, \u2593 = ▓, \u2592 = ▒, \u2591 = ░
// \u2580 = ▀, \u2584 = ▄, \u258C = ▌, \u2590 = ▐
static const wchar_t* LOGO_LINES[7] = {
    L"\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2588\u2584 \u2584\u2588\u2588\u2588\u2588\u2588\u2588\u2584 \u2588\u2588\u2588\u2584 \u2588\u2588\u2588\u2588 \u2588\u2588\u2588\u2588 \u2588\u2588\u2588\u2588 \u2588\u2588\u2588\u2588",
    L"\u2588\u2588\u2588\u2588 \u2580\u2588\u2588\u2588\u258C \u258C\u2593\u2588\u2593\u2588 \u2588\u2588\u2593\u258C \u2588\u2588\u2593\u2588\u2588\u2584\u2588\u2588\u2593 \u2588\u2588\u2593\u2588 \u2584\u2588\u2588\u2593\u2580 \u2588\u2588\u2593\u2588",
    L"\u2592\u2593\u2588\u2588\u2584\u2593\u2588\u2593\u2580 \u2592\u2593\u2588\u2593\u2593\u2580\u2592\u2588\u2593 \u2592\u2593\u2588\u2593 \u2580\u2592\u2588\u2593 \u2592\u2593\u2588\u2593\u2580\u2592\u2593\u2584 \u2592\u2593\u2588\u2593",
    L"\u2591\u2591\u2592\u2593\u2580\u2593\u2592\u2592\u2593\u2584 \u2591\u2591\u2592\u2593 \u2591\u2591\u2592\u2593 \u2591\u2591\u2592\u2593 \u2591\u2591\u2592\u2593 \u2591\u2591\u2592\u2593 \u2591\u2591\u2592\u2593",
    L" \u2591 \u2591\u2591\u2592\u2591\u258C \u2591 \u2591 \u2591 \u2591 \u2591 \u2591 \u2591",
    L" \u2580 \u2580 \u2580\u2580\u2580\u2580 \u2580\u2580\u2580\u2580 \u2580\u2580\u2580\u2580 \u2580\u2580\u2580\u2580 \u2580\u2580\u2580\u2580",
    L" \u2580 "
};

static const wchar_t* TARGET_PROCESSES[] = {
    L"FiveM_GTAProcess.exe",
    L"FiveM_b2060_GTAProcess.exe",
    L"FiveM_b2189_GTAProcess.exe",
    L"FiveM_b2372_GTAProcess.exe",
    L"FiveM_b2545_GTAProcess.exe",
    L"FiveM_b2612_GTAProcess.exe",
    L"FiveM_b2699_GTAProcess.exe",
    L"FiveM_b2802_GTAProcess.exe",
    L"FiveM_b2944_GTAProcess.exe",
    L"FiveM_b3095_GTAProcess.exe",
    L"FiveM_b3179_GTAProcess.exe",
    L"FiveM_b3258_GTAProcess.exe",
    L"FiveM_b3323_GTAProcess.exe",
    L"FiveM_b3407_GTAProcess.exe",
    L"FiveM_b3411_GTAProcess.exe",
    L"FiveM_b3751_GTAProcess.exe"
};

// ============================================================================
// AOB PATCH DEFINITION
// ============================================================================
struct AobPatch {
    const wchar_t* name;
    BYTE search[16];
    int searchLen;
    BYTE patch[16];
    int patchLen;
    BYTE orig[16];
    bool hasOrig;
    std::vector<LPVOID> addrs;
};

static AobPatch g_aobs[2] = {
    {
        L"AOB_1",
        { 0xCD, 0xCC, 0x94, 0x40, 0x01, 0x00, 0x00, 0x00 },
        8,
        { 0x77, 0x77, 0x97, 0x40, 0x01, 0x00, 0x00, 0x00 },
        8,
        { 0 },
        false,
        {}
    },
    {
        L"AOB_2",
        { 0x77, 0x77, 0x97, 0x40, 0x01, 0x00, 0x00, 0x00 },
        8,
        { 0xCD, 0xCC, 0x94, 0x40, 0x01, 0x00, 0x00, 0x00 },
        8,
        { 0 },
        false,
        {}
    }
};

// ============================================================================
// GLOBALS
// ============================================================================
static HWND g_hwnd = nullptr;
static HFONT g_fLogo  = nullptr;
static HFONT g_fMenu  = nullptr;
static HFONT g_fSmall = nullptr;
static HFONT g_fTiny  = nullptr;

static AppState g_state = S_MENU;
static int g_activeMode = 0; // 1 = INSTALL, 2 = RESET
static int g_titleIdx = 0;
static bool g_blinkOn = true;
static int g_spinnerIdx = 0;

static std::mutex g_logMutex;
static std::vector<std::wstring> g_logs;
static std::thread g_worker;

// ============================================================================
// FONT & GDI HELPERS
// ============================================================================
static HFONT MakeCourierFont(int ptSize, int weight) {
    HDC hdc = GetDC(nullptr);
    int dpi = hdc ? GetDeviceCaps(hdc, LOGPIXELSY) : 96;
    if (hdc) ReleaseDC(nullptr, hdc);
    int height = -MulDiv(ptSize, dpi, 72);
    return CreateFontW(
        height,
        0,
        0,
        0,
        weight,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        FIXED_PITCH | FF_MODERN,
        L"Courier New"
    );
}

static void HLineRange(HDC dc, int x1, int x2, int y, int thickness, COLORREF color) {
    if (x2 < x1) return;
    HPEN pen = CreatePen(PS_SOLID, thickness, color);
    if (!pen) return;
    HGDIOBJ old = SelectObject(dc, pen);
    if (thickness <= 1) {
        MoveToEx(dc, x1, y, nullptr);
        LineTo(dc, x2 + 1, y);
    } else {
        for (int i = 0; i < thickness; ++i) {
            MoveToEx(dc, x1, y + i, nullptr);
            LineTo(dc, x2 + 1, y + i);
        }
    }
    SelectObject(dc, old);
    DeleteObject(pen);
}

static void DrawCenteredText(HDC dc, HFONT font, int y, const std::wstring& text, COLORREF color) {
    SetBkMode(dc, TRANSPARENT);
    HGDIOBJ oldFont = SelectObject(dc, font);
    SIZE sz = { 0, 0 };
    GetTextExtentPoint32W(dc, text.c_
