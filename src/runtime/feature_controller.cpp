#define NOMINMAX
#include <windows.h>
#include <objbase.h>

#include "feature_controller.h"
#include "runtime_configuration.h"

#include <cstdio>
#include <conio.h>
#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstring>
#include <string>

#include "vendor/xor.h"

namespace fp_vars
{
bool LoadFromFlatGlobals(const char *(*lookup)(const char *, void *), void *user);
bool Init();
bool IsConfigured();
const char *LastLoadError();
} // namespace fp_vars
namespace ac_vars
{
bool LoadFromFlatGlobals(const char *(*lookup)(const char *, void *), void *user);
bool Init();
bool IsConfigured();
const char *LastLoadError();
} // namespace ac_vars
namespace FloatStrike
{
bool Init();
const char *LastError();
bool ApplyLow();
bool ApplyMedium();
bool ApplyHigh();
bool ApplyCustom(float val, float area);
bool SetRbp38(bool enable);
bool IsRbp38Enabled();
bool SetRunSpeed(bool enable, float speedVal = 1.07f);
bool IsRunSpeedEnabled();
bool Restore();
} // namespace FloatStrike

namespace
{

enum Color : WORD
{
    C_PURPLE = FOREGROUND_RED | FOREGROUND_BLUE,
    C_PINK = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
    C_CYAN = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
    C_YELLOW = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
    C_RED = FOREGROUND_RED,
    C_WHITE = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
    C_GRAY = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
};

struct Rgb
{
    int r, g, b;
};

// Palette: แดงอ่อน ชมพูแดงอ่อน ขาวอ่อน (Soft Light Red, Pastel Rose, White)
static constexpr Rgb kSoftRed = {255, 105, 120};       // #FF6978 Soft Light Red (แดงอ่อน)
static constexpr Rgb kSoftRedLight = {255, 135, 148};  // #FF8794 Light Pastel Red
static constexpr Rgb kSoftCoral = {255, 155, 165};     // #FF9BA5 Soft Coral Red
static constexpr Rgb kSoftRose = {255, 140, 152};      // #FF8C98 Pastel Rose Red
static constexpr Rgb kSoftRoseLight = {255, 185, 195}; // #FFB9C3 Light Rose
static constexpr Rgb kSoftRosePale = {255, 215, 222};  // #FFD7DE Pale Rose White
static constexpr Rgb kSoftWhite = {255, 248, 250};     // #FFF8FA Soft Tinted White
static constexpr Rgb kPureWhite = {255, 255, 255};     // #FFFFFF Pure White
static constexpr Rgb kSoftMuted = {205, 150, 158};     // Subtle pastel hint for borders/brackets
static constexpr Rgb kSoftPeach = {255, 165, 175};     // Soft peach/rose for reset
static constexpr Rgb kSoftError = {255, 80, 95};       // Vibrant red for errors

// Aliases for light red theme
static constexpr Rgb kSoftPurple = kSoftRed;
static constexpr Rgb kSoftPurpleLight = kSoftRedLight;
static constexpr Rgb kSoftOrchid = kSoftCoral;
static constexpr Rgb kSoftPink = kSoftRose;
static constexpr Rgb kSoftPinkLight = kSoftRoseLight;
static constexpr Rgb kSoftPinkPale = kSoftRosePale;

// Banner gradient top → bottom (แดงอ่อน → ชมพูแดงอ่อน → ขาวอ่อน)
static const Rgb kBannerGradient[] = {
    {255, 105, 120}, // Row 0: Soft Light Red
    {255, 128, 140}, // Row 1: Light Pastel Red
    {255, 155, 165}, // Row 2: Soft Coral / Rose
    {255, 185, 195}, // Row 3: Light Rose Pink
    {255, 218, 224}, // Row 4: Powder Rose
    {255, 248, 250}, // Row 5: Soft White
};

static HANDLE g_out = nullptr;
static HANDLE g_in = nullptr;

static const char *kBanner[] = {
    "██████╗  █████╗ ██╗   ██╗ ██████╗ █████╗ ███████╗████████╗",
    "██╔══██╗██╔══██╗╚██╗ ██╔╝██╔════╝██╔══██╗██╔════╝╚══██╔══╝",
    "██████╔╝███████║ ╚████╔╝ ██║     ███████║███████╗   ██║   ",
    "██╔══██╗██╔══██║  ╚██╔╝  ██║     ██╔══██║╚════██║   ██║   ",
    "██║  ██║██║  ██║   ██║   ╚██████╗██║  ██║███████║   ██║   ",
    "╚═╝  ╚═╝╚═╝  ╚═╝   ╚═╝    ╚═════╝╚═╝  ╚═╝╚══════╝   ╚═╝   ",
};
static constexpr int kBannerLines = 6;

static constexpr int kConsoleCols = 64;
static constexpr int kConsoleLines = 25;
static constexpr int kContentPad = 4;
static constexpr BYTE kConsoleAlpha = 240;

static void SetColor(WORD c)
{
    if (!g_out)
        g_out = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(g_out, c);
}

static void ResetColor()
{
    SetColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

static void PrintSuccess(const char *text)
{
    if (!g_out)
        g_out = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(g_out, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    printf("%s", text);
    SetConsoleTextAttribute(g_out, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

static void PrintError(const char *text)
{
    if (!g_out)
        g_out = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(g_out, FOREGROUND_RED | FOREGROUND_INTENSITY);
    printf("%s", text);
    SetConsoleTextAttribute(g_out, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

static int ConsoleWidth()
{
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(g_out, &info))
        return 80;
    return static_cast<int>(info.srWindow.Right - info.srWindow.Left + 1);
}

static int Utf8DisplayWidth(const char *s)
{
    int w = 0;
    for (const unsigned char *p = reinterpret_cast<const unsigned char *>(s); *p; ++p)
    {
        if ((*p & 0xC0) != 0x80)
            ++w;
    }
    return w;
}

static size_t Utf8Next(const char *s)
{
    const unsigned char c = static_cast<unsigned char>(s[0]);
    if (c < 0x80)
        return 1;
    if ((c & 0xE0) == 0xC0)
        return 2;
    if ((c & 0xF0) == 0xE0)
        return 3;
    if ((c & 0xF8) == 0xF0)
        return 4;
    return 1;
}

static void PrintSpaces(int n)
{
    for (int i = 0; i < n; ++i)
        putchar(' ');
}

static void PrintRgb(int r, int g, int b, const char *text)
{
    printf("\033[38;2;%d;%d;%dm%s\033[0m", r, g, b, text);
}

static void PrintRgbColor(const Rgb &c, const char *text)
{
    PrintRgb(c.r, c.g, c.b, text);
}

static void PrintRgbF(const Rgb &c, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char buf[512];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    PrintRgbColor(c, buf);
}

static const Rgb &BannerColorRgb(int row)
{
    if (row <= 0)
        return kBannerGradient[0];
    if (row >= kBannerLines)
        return kBannerGradient[kBannerLines - 1];
    return kBannerGradient[row];
}

static inline Rgb Lerp(const Rgb &a, const Rgb &b, float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    return {static_cast<int>(a.r + (b.r - a.r) * t + 0.5f), static_cast<int>(a.g + (b.g - a.g) * t + 0.5f),
            static_cast<int>(a.b + (b.b - a.b) * t + 0.5f)};
}

static inline Rgb Grad3(const Rgb &c1, const Rgb &c2, const Rgb &c3, float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    if (t < 0.5f)
    {
        return Lerp(c1, c2, t * 2.0f);
    }
    else
    {
        return Lerp(c2, c3, (t - 0.5f) * 2.0f);
    }
}

static inline Rgb GradSym(const Rgb &edge, const Rgb &mid, const Rgb &center, float t)
{
    t = std::clamp(t, 0.0f, 1.0f);
    if (t <= 0.5f)
    {
        return Grad3(edge, mid, center, t * 2.0f);
    }
    else
    {
        return Grad3(center, mid, edge, (t - 0.5f) * 2.0f);
    }
}

static void PrintGradientText(const char *text, const Rgb &c1, const Rgb &c2, const Rgb &c3)
{
    const int totalLen = Utf8DisplayWidth(text);
    if (totalLen <= 0)
        return;
    const float denom = (totalLen > 1) ? static_cast<float>(totalLen - 1) : 1.0f;

    int idx = 0;
    for (const char *p = text; *p;)
    {
        const size_t n = Utf8Next(p);
        char glyph[8] = {};
        memcpy(glyph, p, n);

        const float t = static_cast<float>(idx) / denom;
        const Rgb c = Grad3(c1, c2, c3, t);
        printf("\033[38;2;%d;%d;%dm%s\033[0m", c.r, c.g, c.b, glyph);

        p += n;
        ++idx;
    }
}

static void PrintSymGradientText(const char *text, const Rgb &edge, const Rgb &mid, const Rgb &center)
{
    const int totalLen = Utf8DisplayWidth(text);
    if (totalLen <= 0)
        return;
    const float denom = (totalLen > 1) ? static_cast<float>(totalLen - 1) : 1.0f;

    int idx = 0;
    for (const char *p = text; *p;)
    {
        const size_t n = Utf8Next(p);
        char glyph[8] = {};
        memcpy(glyph, p, n);

        const float t = static_cast<float>(idx) / denom;
        const Rgb c = GradSym(edge, mid, center, t);
        printf("\033[38;2;%d;%d;%dm%s\033[0m", c.r, c.g, c.b, glyph);

        p += n;
        ++idx;
    }
}

static void PrintCenteredGradient(const char *text, const Rgb &c1, const Rgb &c2, const Rgb &c3)
{
    const int pad = std::max(0, (ConsoleWidth() - Utf8DisplayWidth(text)) / 2);
    PrintSpaces(pad);
    PrintGradientText(text, c1, c2, c3);
    putchar('\n');
}

static void PrintCenteredSymGradient(const char *text, const Rgb &edge, const Rgb &mid, const Rgb &center)
{
    const int pad = std::max(0, (ConsoleWidth() - Utf8DisplayWidth(text)) / 2);
    PrintSpaces(pad);
    PrintSymGradientText(text, edge, mid, center);
    putchar('\n');
}

static void PrintBanner()
{
    const int cw = ConsoleWidth();

    for (int row = 0; row < kBannerLines; ++row)
    {
        const char *line = kBanner[row];
        const int lineW = Utf8DisplayWidth(line);
        PrintSpaces(std::max(0, (cw - lineW) / 2));

        const Rgb &color = BannerColorRgb(row);

        for (const char *p = line; *p;)
        {
            const size_t n = Utf8Next(p);
            char glyph[8] = {};
            memcpy(glyph, p, n);
            printf("\033[38;2;%d;%d;%dm%s\033[0m", color.r, color.g, color.b, glyph);
            p += n;
        }
        putchar('\n');
    }
}

static void ApplyConsoleFont()
{
    if (!g_out)
        return;

    CONSOLE_FONT_INFOEX cfi{};
    cfi.cbSize = sizeof(cfi);
    cfi.dwFontSize.Y = 16;
    cfi.FontFamily = FF_MODERN;
    cfi.FontWeight = FW_NORMAL;
    wcscpy_s(cfi.FaceName, L"Consolas");
    if (!SetCurrentConsoleFontEx(g_out, FALSE, &cfi))
    {
        wcscpy_s(cfi.FaceName, L"Lucida Console");
        SetCurrentConsoleFontEx(g_out, FALSE, &cfi);
    }
}

static void ApplyConsoleWindowStyle()
{
    HWND hwnd = GetConsoleWindow();
    if (!hwnd)
        return;

    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    style &= ~(WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME);
    SetWindowLong(hwnd, GWL_STYLE, style);

    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);
    SetLayeredWindowAttributes(hwnd, 0, kConsoleAlpha, LWA_ALPHA);

    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

static void ApplyConsoleSize()
{
    if (!g_out || g_out == INVALID_HANDLE_VALUE)
        return;

    SMALL_RECT minWin = {0, 0, 1, 1};
    SetConsoleWindowInfo(g_out, TRUE, &minWin);

    COORD buf{static_cast<SHORT>(kConsoleCols), static_cast<SHORT>(kConsoleLines)};
    SetConsoleScreenBufferSize(g_out, buf);

    SMALL_RECT win{0, 0, static_cast<SHORT>(kConsoleCols - 1), static_cast<SHORT>(kConsoleLines - 1)};
    SetConsoleWindowInfo(g_out, TRUE, &win);
}

static void SetConsoleCursorVisible(bool visible)
{
    if (!g_out || g_out == INVALID_HANDLE_VALUE)
        return;
    CONSOLE_CURSOR_INFO ci{};
    if (!GetConsoleCursorInfo(g_out, &ci))
        return;
    ci.bVisible = visible ? TRUE : FALSE;
    SetConsoleCursorInfo(g_out, &ci);
}

static void ClearScreen()
{
    if (!g_out || g_out == INVALID_HANDLE_VALUE)
        return;
    CONSOLE_SCREEN_BUFFER_INFO csbi{};
    if (!GetConsoleScreenBufferInfo(g_out, &csbi))
        return;
    DWORD cellCount = csbi.dwSize.X * csbi.dwSize.Y;
    DWORD count = 0;
    COORD homeCoords = {0, 0};
    FillConsoleOutputCharacterA(g_out, ' ', cellCount, homeCoords, &count);
    FillConsoleOutputAttribute(g_out, csbi.wAttributes, cellCount, homeCoords, &count);
    SetConsoleCursorPosition(g_out, homeCoords);
}

static void DrawFrameHeader()
{
    putchar('\n');
    PrintBanner();
    putchar('\n');
    PrintCenteredGradient("[+] CMD RAYCAST [+]", kSoftPurple, kSoftPink, kSoftWhite);
    PrintCenteredSymGradient("FOURTYSTORE | CHAYENSHOP", kSoftPurpleLight, kSoftPink, kSoftWhite);
    PrintCenteredSymGradient("[------------------------------------]", kSoftPurple, kSoftPink, kSoftWhite);
}

static void SetupConsole()
{
    AllocConsole();

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // Create a private active screen buffer specifically for Upcri UI rendering
    HANDLE hPrivateBuf =
        CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                  CONSOLE_TEXTMODE_BUFFER, nullptr);

    if (hPrivateBuf && hPrivateBuf != INVALID_HANDLE_VALUE)
    {
        g_out = hPrivateBuf;
        SetConsoleActiveScreenBuffer(g_out);
    }
    else
    {
        g_out = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                            nullptr, OPEN_EXISTING, 0, nullptr);
    }

    // Redirect process-wide STD handles to NUL so FiveM / Node.js logs go to NUL
    HANDLE hNull = CreateFileW(L"NUL", GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                               nullptr, OPEN_EXISTING, 0, nullptr);
    if (hNull != INVALID_HANDLE_VALUE)
    {
        SetStdHandle(STD_OUTPUT_HANDLE, hNull);
        SetStdHandle(STD_ERROR_HANDLE, hNull);
    }

    g_in = GetStdHandle(STD_INPUT_HANDLE);

    FILE *f = nullptr;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);
    freopen_s(&f, "CONIN$", "r", stdin);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    SetConsoleTitleW(xorstr_(L" Upcri - FOURTYSTORE"));

    if (g_out && g_out != INVALID_HANDLE_VALUE)
    {
        DWORD outMode = 0;
        if (GetConsoleMode(g_out, &outMode))
        {
            SetConsoleMode(g_out, outMode | ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT |
                                      0x0004 /* ENABLE_VIRTUAL_TERMINAL_PROCESSING */);
        }
    }

    if (g_in)
    {
        DWORD inMode = 0;
        if (GetConsoleMode(g_in, &inMode))
        {
            inMode |= ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT;
            inMode &= ~ENABLE_MOUSE_INPUT;
            SetConsoleMode(g_in, inMode);
        }
    }

    ApplyConsoleFont();
    ApplyConsoleSize();
    ApplyConsoleWindowStyle();

    SetConsoleCursorVisible(true);
}

static bool StrikeConfigReady()
{
    // FloatStrike does not depend on the separate Hard Head configuration.
    return fp_vars::IsConfigured();
}

static void RunMode(bool (*fn)(), const char *name)
{
    if (!StrikeConfigReady())
    {
        ClearScreen();
        PrintBanner();
        printf("\n\n");
        PrintSpaces(kContentPad);
        printf("\033[38;2;%d;%d;%dm[!] Local configuration unavailable\033[0m\n", kSoftError.r, kSoftError.g,
               kSoftError.b);
        Sleep(2000);
        return;
    }

    ClearScreen();
    DrawFrameHeader();
    printf("\n\n");
    PrintSpaces(kContentPad);
    printf("\033[38;2;%d;%d;%dmApplying \033[0m", kSoftPurpleLight.r, kSoftPurpleLight.g, kSoftPurpleLight.b);
    PrintGradientText(name, kSoftPink, kSoftPinkLight, kPureWhite);
    printf("...\n");

    PrintSpaces(kContentPad);
    if (fn())
    {
        PrintGradientText("OK\n", kSoftPink, kSoftPinkLight, kPureWhite);
    }
    else
    {
        printf("\033[38;2;%d;%d;%dmError: %s\033[0m\n", kSoftError.r, kSoftError.g, kSoftError.b,
               FloatStrike::LastError());
    }
    Sleep(1500);
}

static void RunReset()
{
    ClearScreen();
    DrawFrameHeader();
    printf("\n\n");
    PrintSpaces(kContentPad);
    PrintGradientText("Resetting...\n", kSoftPeach, kSoftPinkPale, kPureWhite);

    PrintSpaces(kContentPad);
    if (FloatStrike::Restore())
    {
        PrintGradientText("OK\n", kSoftPink, kSoftPinkLight, kPureWhite);
    }
    else
    {
        printf("\033[38;2;%d;%d;%dmError: %s\033[0m\n", kSoftError.r, kSoftError.g, kSoftError.b,
               FloatStrike::LastError());
    }
    Sleep(1500);
}

static bool g_uiRunning = true;

static void CloseConsole()
{
    g_uiRunning = false;
    HWND hwnd = GetConsoleWindow();
    if (hwnd)
    {
        ShowWindow(hwnd, SW_HIDE);
        FreeConsole();
        PostMessage(hwnd, WM_CLOSE, 0, 0);
    }
}

static bool GenerateRandomGuidTmp(wchar_t *outName, size_t outSize)
{
    GUID guid{};
    if (FAILED(CoCreateGuid(&guid)))
    {
        return false;
    }
    swprintf_s(outName, outSize, L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x.tmp", guid.Data1,
               guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
               guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
    return true;
}

static void RunCleanFile()
{
    CloseConsole();
}

struct MenuEntry
{
    const char *action;
    WORD color;
    void (*handler)();
};

static int g_menuBaseRow = 0;

static void PrintMenuLineContent(int num, const char *action, WORD actionColor, bool selected)
{
    (void)actionColor;
    PrintSpaces(kContentPad);

    if (selected)
    {
        printf("\033[38;2;%d;%d;%dm> \033[0m", kSoftPink.r, kSoftPink.g, kSoftPink.b);
        printf("\033[38;2;%d;%d;%dm[\033[38;2;%d;%d;%dm%d\033[38;2;%d;%d;%dm] \033[0m", kSoftPurpleLight.r,
               kSoftPurpleLight.g, kSoftPurpleLight.b, kPureWhite.r, kPureWhite.g, kPureWhite.b, num,
               kSoftPurpleLight.r, kSoftPurpleLight.g, kSoftPurpleLight.b);
        PrintGradientText(action, kSoftPink, kSoftPinkPale, kPureWhite);
    }
    else
    {
        printf("  ");
        printf("\033[38;2;%d;%d;%dm[\033[38;2;%d;%d;%dm%d\033[38;2;%d;%d;%dm] \033[0m", kSoftMuted.r,
               kSoftMuted.g, kSoftMuted.b, kSoftWhite.r, kSoftWhite.g, kSoftWhite.b, num, kSoftMuted.r,
               kSoftMuted.g, kSoftMuted.b);
        if (strcmp(action, "Reset") == 0 || strcmp(action, "Clean file") == 0)
        {
            PrintGradientText(action, kSoftPeach, kSoftPinkPale, kSoftWhite);
        }
        else
        {
            PrintGradientText(action, kSoftPurple, kSoftPink, kSoftWhite);
        }
    }
}

static int MenuItemScreenRow(int index)
{
    return g_menuBaseRow + index + (index > 2 ? 1 : 0);
}

static void PrintMenuLineAt(int row, int num, const char *action, WORD actionColor, bool selected)
{
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(g_out, &info))
        return;

    COORD pos{0, static_cast<SHORT>(row)};
    SetConsoleCursorPosition(g_out, pos);
    fputs("\033[2K", stdout);
    PrintMenuLineContent(num, action, actionColor, selected);
}

static void DrawMenuFull(const MenuEntry *items, int count, int selected)
{
    ClearScreen();
    DrawFrameHeader();
    putchar('\n');

    CONSOLE_SCREEN_BUFFER_INFO info{};
    GetConsoleScreenBufferInfo(g_out, &info);
    g_menuBaseRow = info.dwCursorPosition.Y;

    for (int i = 0; i < count; ++i)
    {
        PrintMenuLineContent(i + 1, items[i].action, items[i].color, i == selected);
        putchar('\n');
        if (i == 2)
            putchar('\n');
    }

    putchar('\n');
    PrintCenteredSymGradient("Scroll wheel to navigate, Enter to select", kSoftMuted, kSoftPinkPale,
                             kSoftWhite);

    GetConsoleScreenBufferInfo(g_out, &info);
    COORD bottom{0, static_cast<SHORT>(info.dwCursorPosition.Y)};
    SetConsoleCursorPosition(g_out, bottom);
    SetConsoleCursorVisible(false);
    fflush(stdout);
}

static void UpdateMenuSelection(const MenuEntry *items, int prev, int curr)
{
    if (prev == curr)
        return;
    PrintMenuLineAt(MenuItemScreenRow(prev), prev + 1, items[prev].action, items[prev].color, false);
    PrintMenuLineAt(MenuItemScreenRow(curr), curr + 1, items[curr].action, items[curr].color, true);
    SetConsoleCursorVisible(false);
    fflush(stdout);
}

enum class MenuNavResult
{
    Continue,
    Redraw,
    Activate
};

static MenuNavResult ReadMenuNav(int &selected, int count)
{
    INPUT_RECORD ev{};
    DWORD read = 0;

    if (!ReadConsoleInputA(g_in, &ev, 1, &read) || read == 0)
        return MenuNavResult::Continue;

    if (ev.EventType == KEY_EVENT)
    {
        if (!ev.Event.KeyEvent.bKeyDown)
            return MenuNavResult::Continue;

        const WORD vk = ev.Event.KeyEvent.wVirtualKeyCode;
        if (vk == VK_UP)
        {
            selected = (selected - 1 + count) % count;
            return MenuNavResult::Redraw;
        }
        if (vk == VK_DOWN)
        {
            selected = (selected + 1) % count;
            return MenuNavResult::Redraw;
        }
        if (vk == VK_RETURN)
            return MenuNavResult::Activate;
        if (vk >= '1' && vk <= '9')
        {
            const int idx = vk - '1';
            if (idx < count)
                selected = idx;
            return MenuNavResult::Activate;
        }
        return MenuNavResult::Continue;
    }

    if (ev.EventType == MOUSE_EVENT)
    {
        if (ev.Event.MouseEvent.dwEventFlags & MOUSE_WHEELED)
        {
            const SHORT delta = static_cast<SHORT>(HIWORD(ev.Event.MouseEvent.dwButtonState));
            if (delta > 0)
                selected = (selected - 1 + count) % count;
            else if (delta < 0)
                selected = (selected + 1) % count;
            return MenuNavResult::Redraw;
        }
        return MenuNavResult::Continue;
    }

    return MenuNavResult::Continue;
}

static void MenuLoop()
{
    static const MenuEntry items[] = {
        {"Low", C_PINK, [] { RunMode(FloatStrike::ApplyLow, "LOW"); }},
        {"Medium", C_PINK, [] { RunMode(FloatStrike::ApplyMedium, "MEDIUM"); }},
        {"High", C_PINK, [] { RunMode(FloatStrike::ApplyHigh, "HIGH"); }},
        {"Reset", C_YELLOW, [] { RunReset(); }},
        {"Clean file", C_YELLOW, [] { RunCleanFile(); }},
    };
    static constexpr int kCount = static_cast<int>(sizeof(items) / sizeof(items[0]));

    int selected = 0;
    int prevSelected = 0;
    while (g_uiRunning)
    {
        DrawMenuFull(items, kCount, selected);
        prevSelected = selected;

        for (;;)
        {
            if (!g_uiRunning)
                return;
            switch (ReadMenuNav(selected, kCount))
            {
            case MenuNavResult::Redraw:
                UpdateMenuSelection(items, prevSelected, selected);
                prevSelected = selected;
                break;
            case MenuNavResult::Activate:
                goto menu_activate;
            case MenuNavResult::Continue:
            default:
                break;
            }
        }
    menu_activate:
        SetConsoleCursorVisible(false);
        if (items[selected].handler)
            items[selected].handler();
        if (!g_uiRunning)
            return;
    }
}

} // namespace

bool StrikeUiRun()
{
    SetupConsole();
    Strike_Initialize();

    if (!StrikeConfigReady())
    {
        PrintError(xorstr_("Error: local strike configuration is unavailable\r\n"));
        Sleep(3000);
        return false;
    }

    if (!FloatStrike::Init())
    {
        char errBuf[512];
        snprintf(errBuf, sizeof(errBuf), "Error: %s\r\n", FloatStrike::LastError());
        PrintError(errBuf);
        Sleep(3000);
        return false;
    }

    MenuLoop();
    return true;
}

static std::string g_lastStatus = "Ready";
void Strike_Initialize()
{
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;
    const bool fpReady = fp_vars::LoadFromFlatGlobals(local_config::Lookup, nullptr);
    const bool hhReady = ac_vars::LoadFromFlatGlobals(local_config::Lookup, nullptr);
    if (!fpReady)
    {
        g_lastStatus = std::string("Local configuration: ") + fp_vars::LastLoadError();
        return;
    }
    if (!FloatStrike::Init())
    {
        g_lastStatus = FloatStrike::LastError();
        return;
    }
    g_lastStatus = hhReady ? "Ready"
                           : std::string("FloatStrike ready; Hard Head configuration incomplete: ") +
                                 ac_vars::LastLoadError();
}

bool Strike_ApplyLow(std::string &outMsg)
{
    if (!StrikeConfigReady())
    {
        outMsg = "Local configuration unavailable";
        g_lastStatus = outMsg;
        return false;
    }
    if (FloatStrike::ApplyLow())
    {
        outMsg = "Applied LOW successfully";
        g_lastStatus = outMsg;
        return true;
    }
    else
    {
        outMsg = FloatStrike::LastError() ? FloatStrike::LastError() : "Failed to apply LOW";
        g_lastStatus = outMsg;
        return false;
    }
}

bool Strike_ApplyMedium(std::string &outMsg)
{
    if (!StrikeConfigReady())
    {
        outMsg = "Local configuration unavailable";
        g_lastStatus = outMsg;
        return false;
    }
    if (FloatStrike::ApplyMedium())
    {
        outMsg = "Applied MEDIUM successfully";
        g_lastStatus = outMsg;
        return true;
    }
    else
    {
        outMsg = FloatStrike::LastError() ? FloatStrike::LastError() : "Failed to apply MEDIUM";
        g_lastStatus = outMsg;
        return false;
    }
}

bool Strike_ApplyHigh(std::string &outMsg)
{
    if (!StrikeConfigReady())
    {
        outMsg = "Local configuration unavailable";
        g_lastStatus = outMsg;
        return false;
    }
    if (FloatStrike::ApplyHigh())
    {
        outMsg = "Applied HIGH successfully";
        g_lastStatus = outMsg;
        return true;
    }
    else
    {
        outMsg = FloatStrike::LastError() ? FloatStrike::LastError() : "Failed to apply HIGH";
        g_lastStatus = outMsg;
        return false;
    }
}

bool Strike_ApplyCustom(float val, std::string &outMsg, float area)
{
    if (!StrikeConfigReady())
    {
        outMsg = "การเข้าถึงถูกปฏิเสธ";
        g_lastStatus = outMsg;
        return false;
    }
    if (FloatStrike::ApplyCustom(val, area))
    {
        outMsg = "ใช้งานสำเร็จ";
        g_lastStatus = outMsg;
        return true;
    }
    else
    {
        outMsg = FloatStrike::LastError() ? FloatStrike::LastError() : "ไม่สามารถใช้งานได้";
        g_lastStatus = outMsg;
        return false;
    }
}

bool Strike_SetRbp38(bool enable, std::string &outMsg)
{
    if (!StrikeConfigReady())
    {
        outMsg = "การเข้าถึงถูกปฏิเสธ";
        g_lastStatus = outMsg;
        return false;
    }
    if (FloatStrike::SetRbp38(enable))
    {
        outMsg = enable ? "เปิดใช้งาน Prop แล้ว" : "ปิดใช้งาน Prop แล้ว";
        g_lastStatus = outMsg;
        return true;
    }
    else
    {
        outMsg = FloatStrike::LastError() ? FloatStrike::LastError() : "ไม่สามารถสลับ Prop ได้";
        g_lastStatus = outMsg;
        return false;
    }
}

bool Strike_GetRbp38()
{
    return FloatStrike::IsRbp38Enabled();
}

bool Strike_SetRunSpeed(bool enable, std::string &outMsg, float speedVal)
{
    if (!StrikeConfigReady())
    {
        outMsg = "การเข้าถึงถูกปฏิเสธ";
        g_lastStatus = outMsg;
        return false;
    }
    if (FloatStrike::SetRunSpeed(enable, speedVal))
    {
        outMsg = enable ? "เปิดวิ่งไวแล้ว (1.07)" : "ปิดวิ่งไวแล้ว";
        g_lastStatus = outMsg;
        return true;
    }
    else
    {
        outMsg = FloatStrike::LastError() ? FloatStrike::LastError() : "ไม่สามารถเปิดวิ่งไวได้";
        g_lastStatus = outMsg;
        return false;
    }
}

bool Strike_GetRunSpeed()
{
    return FloatStrike::IsRunSpeedEnabled();
}

bool Strike_Reset(std::string &outMsg)
{
    if (!StrikeConfigReady())
    {
        outMsg = "Local configuration unavailable";
        g_lastStatus = outMsg;
        return false;
    }
    if (FloatStrike::Restore())
    {
        outMsg = "รีเซ็ตสำเร็จ";
        g_lastStatus = outMsg;
        return true;
    }
    else
    {
        outMsg = FloatStrike::LastError() ? FloatStrike::LastError() : "ไม่สามารถรีเซ็ตได้";
        g_lastStatus = outMsg;
        return false;
    }
}

static int g_menuToggleVk = VK_HOME;

int Strike_GetMenuToggleVk()
{
    return g_menuToggleVk;
}
void Strike_SetMenuToggleVk(int vk)
{
    g_menuToggleVk = (vk > 0 ? vk : VK_HOME);
}

const char *Strike_GetLastStatus()
{
    return g_lastStatus.c_str();
}

void Strike_Shutdown()
{
    if (StrikeConfigReady())
        FloatStrike::Restore();
}
