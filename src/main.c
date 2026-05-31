/*
 * Blazingtron 2 - Native Win32 + x64 Assembly
 * Rebuild of blazingtron for Windows 11
 * Pure C + NASM assembly, no Qt, no Python, no runtime deps
 *
 * Build with MinGW-w64 + NASM (see build.bat / Makefile)
 */

#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN

// Embed manifest for modern Win11 controls (Common Controls 6) + DPI awareness
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <windows.h>
#include <commctrl.h>
#include <wchar.h>
#include <stdio.h>

// Control IDs
#define IDC_SERVICE_EDIT     101
#define IDC_VIP_STATIC       102
#define IDC_CUT_STATIC       103
#define IDC_NOTE_EDIT        104
#define IDC_GENERATED_EDIT   105
#define IDC_COPY_BTN         106
#define IDC_EXIT_BTN         107
#define IDC_COPIED_LABEL     108

// Discount radios (0/7/10/12)
#define IDC_DISC_0           201
#define IDC_DISC_7           202
#define IDC_DISC_10          203
#define IDC_DISC_12          204

// Cut radios (30/35/40/50)
#define IDC_CUT_30           301
#define IDC_CUT_35           302
#define IDC_CUT_40           303
#define IDC_CUT_50           304

// Platform radios
#define IDC_PLAT_PC          401
#define IDC_PLAT_XBOX        402
#define IDC_PLAT_PS4         403
#define IDC_PLAT_SHERPA      404

// Assembly functions (NASM x64 SSE2)
extern double calc_vip_price(double service, int discount_percent);
extern double calc_booster_cut(double vip_price, int cut_percent);

// Globals
static HINSTANCE g_hInst;
static HWND g_hMain;
static HWND g_hServiceEdit;
static HWND g_hVipStatic;
static HWND g_hCutStatic;
static HWND g_hNoteEdit;
static HWND g_hGeneratedEdit;
static HWND g_hCopiedLabel;
static HWND g_hTitleLabel;

static HWND g_hDisc[4];   // 0,7/10/12
static HWND g_hCut[4];    // 30,35,40,50
static HWND g_hPlat[4];   // PC, XBOX, PS4, Sherpa

static HFONT g_hFontNormal;
static HFONT g_hFontBig;
static HFONT g_hFontTitle;

static UINT_PTR g_timerId = 0;

// Forward decls
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static void CreateControls(HWND hwnd);
static void ApplyFonts(void);
static void UpdateCalculations(void);
static int GetDiscountPercent(void);
static int GetCutPercent(void);
static int GetPlatformIndex(void);
static const wchar_t* GetPlatformName(int idx);
static void BuildGeneratedNote(wchar_t* out, size_t outLen);
static void DoCopyNote(void);
static void SetRadioGroupCheck(HWND* group, int count, int idx);
static void SafeSetStaticText(HWND h, const wchar_t* fmt, double val);
static double ParseServiceValue(void);
static void ClearCopiedFeedback(void);

// Entry point
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPWSTR lpCmdLine, int nCmdShow)
{
    g_hInst = hInstance;

    // Init common controls (for visual styles via manifest)
    INITCOMMONCONTROLSEX icex = { sizeof(icex), ICC_STANDARD_CLASSES | ICC_WIN95_CLASSES };
    InitCommonControlsEx(&icex);

    // Register window class
    WNDCLASSEXW wc = {0};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"Blazingtron2Class";
    wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(101)); // from rc if present

    if (!RegisterClassExW(&wc)) {
        MessageBoxW(NULL, L"Failed to register window class", L"Error", MB_ICONERROR);
        return 1;
    }

    // Create main window (sized similar to original ~543x557)
    g_hMain = CreateWindowExW(
        0,
        L"Blazingtron2Class",
        L"Blazingtron 2",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        560, 560,
        NULL, NULL, hInstance, NULL
    );

    if (!g_hMain) {
        MessageBoxW(NULL, L"Failed to create window", L"Error", MB_ICONERROR);
        return 1;
    }

    ShowWindow(g_hMain, nCmdShow);
    UpdateWindow(g_hMain);

    // Message loop
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE:
        CreateControls(hwnd);
        ApplyFonts();
        // Defaults: 10% discount, 30% cut, PC platform
        SetRadioGroupCheck(g_hDisc, 4, 2); // 10%
        SetRadioGroupCheck(g_hCut,  4, 0); // 30%
        SetRadioGroupCheck(g_hPlat, 4, 0); // PC
        SetWindowTextW(g_hServiceEdit, L"100.00");
        SetWindowTextW(g_hNoteEdit, L"Weekly raid + dungeon");
        UpdateCalculations();
        return 0;

    case WM_COMMAND:
        {
            int id = LOWORD(wParam);
            int code = HIWORD(wParam);

            // Radio buttons - any discount/cut/platform change
            if (code == BN_CLICKED) {
                if (id >= IDC_DISC_0 && id <= IDC_DISC_12) {
                    int idx = id - IDC_DISC_0;
                    SetRadioGroupCheck(g_hDisc, 4, idx);
                    UpdateCalculations();
                }
                else if (id >= IDC_CUT_30 && id <= IDC_CUT_50) {
                    int idx = id - IDC_CUT_30;
                    SetRadioGroupCheck(g_hCut, 4, idx);
                    UpdateCalculations();
                }
                else if (id >= IDC_PLAT_PC && id <= IDC_PLAT_SHERPA) {
                    int idx = id - IDC_PLAT_PC;
                    SetRadioGroupCheck(g_hPlat, 4, idx);
                    UpdateCalculations();
                }
                else if (id == IDC_COPY_BTN) {
                    DoCopyNote();
                }
                else if (id == IDC_EXIT_BTN) {
                    PostQuitMessage(0);
                }
            }

            // Live update on service value or note text change
            if (code == EN_CHANGE) {
                if (id == IDC_SERVICE_EDIT || id == IDC_NOTE_EDIT) {
                    UpdateCalculations();
                }
            }
        }
        return 0;

    case WM_TIMER:
        if (wParam == 1) {
            ClearCopiedFeedback();
            KillTimer(hwnd, 1);
            g_timerId = 0;
        }
        return 0;

    case WM_DESTROY:
        if (g_timerId) KillTimer(hwnd, g_timerId);
        if (g_hFontNormal) DeleteObject(g_hFontNormal);
        if (g_hFontBig)    DeleteObject(g_hFontBig);
        if (g_hFontTitle)  DeleteObject(g_hFontTitle);
        PostQuitMessage(0);
        return 0;

    case WM_CTLCOLORSTATIC:
        // Make read-only generated edit look nice (white bg)
        if ((HWND)lParam == g_hGeneratedEdit) {
            SetBkColor((HDC)wParam, RGB(255,255,255));
            return (LRESULT)GetStockObject(WHITE_BRUSH);
        }
        break;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static void CreateControls(HWND hwnd)
{
    // Title (stored so we can set big font)
    g_hTitleLabel = CreateWindowW(L"STATIC", L"VIP Blazingtron 2", WS_VISIBLE | WS_CHILD | SS_LEFT,
                  20, 12, 340, 38, hwnd, NULL, g_hInst, NULL);

    // Service Value section
    CreateWindowW(L"STATIC", L"Service Value:", WS_VISIBLE | WS_CHILD,
                  20, 58, 140, 22, hwnd, NULL, g_hInst, NULL);

    CreateWindowW(L"STATIC", L"€", WS_VISIBLE | WS_CHILD | SS_CENTER,
                  20, 82, 24, 26, hwnd, NULL, g_hInst, NULL);

    g_hServiceEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL,
        46, 82, 160, 26, hwnd, (HMENU)IDC_SERVICE_EDIT, g_hInst, NULL);

    CreateWindowW(L"STATIC", L"VIP price:", WS_VISIBLE | WS_CHILD,
                  240, 58, 100, 22, hwnd, NULL, g_hInst, NULL);

    g_hVipStatic = CreateWindowW(L"STATIC", L"€0.00", WS_VISIBLE | WS_CHILD | SS_LEFT,
                  240, 80, 140, 32, hwnd, (HMENU)IDC_VIP_STATIC, g_hInst, NULL);

    // Discount group (right side)
    CreateWindowW(L"STATIC", L"Discount:", WS_VISIBLE | WS_CHILD,
                  410, 55, 100, 20, hwnd, NULL, g_hInst, NULL);

    const wchar_t* discLabels[4] = { L"0% discount", L"7% discount", L"10% discount", L"12% discount" };
    int discIds[4] = { IDC_DISC_0, IDC_DISC_7, IDC_DISC_10, IDC_DISC_12 };
    for (int i = 0; i < 4; ++i) {
        DWORD style = WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_RADIOBUTTON;
        if (i == 0) style |= WS_GROUP;
        g_hDisc[i] = CreateWindowW(L"BUTTON", discLabels[i],
            style, 410, 78 + i*23, 120, 22, hwnd, (HMENU)discIds[i], g_hInst, NULL);
    }

    // Booster's Cut
    CreateWindowW(L"STATIC", L"Booster's Cut:", WS_VISIBLE | WS_CHILD,
                  20, 130, 140, 22, hwnd, NULL, g_hInst, NULL);

    g_hCutStatic = CreateWindowW(L"STATIC", L"€0.00", WS_VISIBLE | WS_CHILD | SS_LEFT,
                  20, 152, 160, 36, hwnd, (HMENU)IDC_CUT_STATIC, g_hInst, NULL);

    // Booster cut % (right)
    CreateWindowW(L"STATIC", L"Booster cut %:", WS_VISIBLE | WS_CHILD,
                  410, 175, 120, 20, hwnd, NULL, g_hInst, NULL);

    const wchar_t* cutLabels[4] = { L"30% Cut", L"35% Cut", L"40% Cut", L"50% Cut" };
    int cutIds[4] = { IDC_CUT_30, IDC_CUT_35, IDC_CUT_40, IDC_CUT_50 };
    for (int i = 0; i < 4; ++i) {
        DWORD style = WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_RADIOBUTTON;
        if (i == 0) style |= WS_GROUP;
        g_hCut[i] = CreateWindowW(L"BUTTON", cutLabels[i],
            style, 410, 198 + i*23, 120, 22, hwnd, (HMENU)cutIds[i], g_hInst, NULL);
    }

    // Platform selection
    CreateWindowW(L"STATIC", L"Platform:", WS_VISIBLE | WS_CHILD,
                  20, 210, 100, 20, hwnd, NULL, g_hInst, NULL);

    const wchar_t* platLabels[4] = { L"PC", L"XBOX", L"PS4", L"Sherpa" };
    int platIds[4] = { IDC_PLAT_PC, IDC_PLAT_XBOX, IDC_PLAT_PS4, IDC_PLAT_SHERPA };
    for (int i = 0; i < 4; ++i) {
        DWORD style = WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_RADIOBUTTON;
        if (i == 0) style |= WS_GROUP;
        g_hPlat[i] = CreateWindowW(L"BUTTON", platLabels[i],
            style, 20 + i*110, 232, 100, 22, hwnd, (HMENU)platIds[i], g_hInst, NULL);
    }

    // Note input
    CreateWindowW(L"STATIC", L"Note / Order info:", WS_VISIBLE | WS_CHILD,
                  20, 268, 200, 20, hwnd, NULL, g_hInst, NULL);

    g_hNoteEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL,
        20, 290, 510, 26, hwnd, (HMENU)IDC_NOTE_EDIT, g_hInst, NULL);

    // Generated note (read-only)
    CreateWindowW(L"STATIC", L"Generated note (clipboard):", WS_VISIBLE | WS_CHILD,
                  20, 328, 250, 20, hwnd, NULL, g_hInst, NULL);

    g_hGeneratedEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_VISIBLE | WS_CHILD | ES_LEFT | ES_READONLY,
        20, 350, 510, 26, hwnd, (HMENU)IDC_GENERATED_EDIT, g_hInst, NULL);

    // Buttons
    CreateWindowW(L"BUTTON", L"Copy Note",
        WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
        320, 400, 100, 32, hwnd, (HMENU)IDC_COPY_BTN, g_hInst, NULL);

    CreateWindowW(L"BUTTON", L"Exit",
        WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
        440, 400, 90, 32, hwnd, (HMENU)IDC_EXIT_BTN, g_hInst, NULL);

    // Copied feedback
    g_hCopiedLabel = CreateWindowW(L"STATIC", L"",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        320, 438, 100, 20, hwnd, (HMENU)IDC_COPIED_LABEL, g_hInst, NULL);

    // Footer
    CreateWindowW(L"STATIC", L"Blazingtron 2  •  Win32 + x64 Assembly  •  Native Windows 11",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        20, 490, 510, 18, hwnd, NULL, g_hInst, NULL);
}

static void ApplyFonts(void)
{
    // Title font (larger)
    g_hFontTitle = CreateFontW(26, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                               CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    // Normal UI font
    g_hFontNormal = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    // Big price font
    g_hFontBig = CreateFontW(24, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    // Apply to controls
    HWND children[] = {
        g_hServiceEdit, g_hNoteEdit, g_hGeneratedEdit,
        g_hVipStatic, g_hCutStatic
    };
    for (size_t i = 0; i < sizeof(children)/sizeof(children[0]); ++i) {
        if (children[i]) SendMessageW(children[i], WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
    }

    if (g_hVipStatic) SendMessageW(g_hVipStatic, WM_SETFONT, (WPARAM)g_hFontBig, TRUE);
    if (g_hCutStatic) SendMessageW(g_hCutStatic, WM_SETFONT, (WPARAM)g_hFontBig, TRUE);
    if (g_hTitleLabel) SendMessageW(g_hTitleLabel, WM_SETFONT, (WPARAM)g_hFontTitle, TRUE);
}

static void SetRadioGroupCheck(HWND* group, int count, int idx)
{
    for (int i = 0; i < count; ++i) {
        SendMessageW(group[i], BM_SETCHECK, (i == idx) ? BST_CHECKED : BST_UNCHECKED, 0);
    }
}

static int GetDiscountPercent(void)
{
    const int percents[4] = {0, 7, 10, 12};
    for (int i = 0; i < 4; ++i) {
        if (SendMessageW(g_hDisc[i], BM_GETCHECK, 0, 0) == BST_CHECKED)
            return percents[i];
    }
    return 10; // default
}

static int GetCutPercent(void)
{
    const int percents[4] = {30, 35, 40, 50};
    for (int i = 0; i < 4; ++i) {
        if (SendMessageW(g_hCut[i], BM_GETCHECK, 0, 0) == BST_CHECKED)
            return percents[i];
    }
    return 30;
}

static int GetPlatformIndex(void)
{
    for (int i = 0; i < 4; ++i) {
        if (SendMessageW(g_hPlat[i], BM_GETCHECK, 0, 0) == BST_CHECKED)
            return i;
    }
    return 0;
}

static const wchar_t* GetPlatformName(int idx)
{
    static const wchar_t* names[4] = { L"PC", L"XBOX", L"PS4", L"Sherpa" };
    return names[idx];
}

static double ParseServiceValue(void)
{
    wchar_t buf[64] = {0};
    GetWindowTextW(g_hServiceEdit, buf, 63);
    wchar_t* end = NULL;
    double v = wcstod(buf, &end);
    if (v < 0.0) v = 0.0;
    if (v > 100000.0) v = 100000.0; // sanity
    return v;
}

static void SafeSetStaticText(HWND h, const wchar_t* fmt, double val)
{
    wchar_t buf[64];
    swprintf_s(buf, 64, fmt, val);
    SetWindowTextW(h, buf);
}

static void BuildGeneratedNote(wchar_t* out, size_t outLen)
{
    double vip = 0.0;
    wchar_t serviceBuf[64];
    GetWindowTextW(g_hServiceEdit, serviceBuf, 63);
    double service = ParseServiceValue();

    int d = GetDiscountPercent();
    vip = calc_vip_price(service, d);

    int c = GetCutPercent();
    double cut = calc_booster_cut(vip, c);

    int platIdx = GetPlatformIndex();
    const wchar_t* plat = GetPlatformName(platIdx);

    wchar_t note[256] = {0};
    GetWindowTextW(g_hNoteEdit, note, 255);

    // Format: "PC - €12.50 - some note here"
    swprintf_s(out, outLen, L"%s - €%.2f - %s", plat, cut, note);
}

static void UpdateCalculations(void)
{
    double service = ParseServiceValue();
    int d = GetDiscountPercent();
    int c = GetCutPercent();

    double vip = calc_vip_price(service, d);
    double cut = calc_booster_cut(vip, c);

    SafeSetStaticText(g_hVipStatic, L"€%.2f", vip);
    SafeSetStaticText(g_hCutStatic, L"€%.2f", cut);

    // Update generated note
    wchar_t note[512];
    BuildGeneratedNote(note, 512);
    SetWindowTextW(g_hGeneratedEdit, note);
}

static void DoCopyNote(void)
{
    wchar_t note[512];
    GetWindowTextW(g_hGeneratedEdit, note, 511);

    if (wcslen(note) == 0) {
        wcscpy_s(note, 512, L"Nothing to copy");
    }

    if (OpenClipboard(g_hMain)) {
        EmptyClipboard();
        size_t bytes = (wcslen(note) + 1) * sizeof(wchar_t);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
        if (hMem) {
            wchar_t* p = (wchar_t*)GlobalLock(hMem);
            wcscpy_s(p, wcslen(note) + 1, note);
            GlobalUnlock(hMem);
            SetClipboardData(CF_UNICODETEXT, hMem);
        }
        CloseClipboard();
    }

    // Feedback like original
    SetWindowTextW(g_hCopiedLabel, L"copied!");
    if (g_timerId) KillTimer(g_hMain, g_timerId);
    g_timerId = SetTimer(g_hMain, 1, 1600, NULL);
}

static void ClearCopiedFeedback(void)
{
    if (g_hCopiedLabel) {
        SetWindowTextW(g_hCopiedLabel, L"");
    }
}
