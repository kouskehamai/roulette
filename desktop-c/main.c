// カスタムルーレット - Win32 API ネイティブ版
//
// Electron版がChromiumを同梱してWebページを表示していたのに対し、
// こちらはOSのウィンドウ機能(Win32 API)を直接呼び出してUIを組み立てている。
// 追加ライブラリなし、実行ファイルも数百KB程度になる。
//
// ボタンはBS_OWNERDRAWでカスタム描画し、角丸・配色・ホバー効果をつけて
// Web版の見た目（青いボタン、トマト色のルーレット表示）に寄せている。

#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <time.h>
#include <wchar.h>
#include "resource.h"

#pragma comment(lib, "comctl32.lib")

#define MAX_VALUES 200
#define MAX_VALUE_LEN 128
#define TIMER_ID 1
#define EDIT_SUBCLASS_ID 1
#define BUTTON_SUBCLASS_ID 2

// ---- Web版に合わせた配色 ----
#define COLOR_BG          RGB(0xf4, 0xf4, 0xf9) // ページ背景
#define COLOR_TEXT_DARK   RGB(0x33, 0x33, 0x33) // 見出し・本文
#define COLOR_ROULETTE    RGB(0xff, 0x63, 0x47) // ルーレット表示(トマト色)
#define COLOR_BTN_NORMAL  RGB(0x00, 0x7b, 0xff) // ボタン通常
#define COLOR_BTN_HOVER   RGB(0x00, 0x56, 0xb3) // ボタンホバー
#define COLOR_BTN_DISABLED RGB(0xaa, 0xaa, 0xaa) // ボタン無効
#define COLOR_WHITE       RGB(0xff, 0xff, 0xff)

static wchar_t g_values[MAX_VALUES][MAX_VALUE_LEN];
static int g_valueCount = 0;

static HWND g_hEdit, g_hListBox, g_hRoulette, g_hStartBtn, g_hStopBtn, g_hAddBtn, g_hDeleteBtn;
static HFONT g_hFontUI, g_hFontTitle, g_hFontRoulette;
static HBRUSH g_hBgBrush;
static BOOL g_running = FALSE;

// オーナードローボタンの状態管理（ホバー中かどうか）
typedef struct {
    HWND hwnd;
    int ctlId;
    BOOL hover;
} ButtonState;

#define BTN_COUNT 4
static ButtonState g_btnStates[BTN_COUNT];

static ButtonState* FindButtonState(int ctlId) {
    for (int i = 0; i < BTN_COUNT; i++) {
        if (g_btnStates[i].ctlId == ctlId) return &g_btnStates[i];
    }
    return NULL;
}

static void RefreshListBox(void) {
    SendMessageW(g_hListBox, LB_RESETCONTENT, 0, 0);
    for (int i = 0; i < g_valueCount; i++) {
        SendMessageW(g_hListBox, LB_ADDSTRING, 0, (LPARAM)g_values[i]);
    }
    EnableWindow(g_hStartBtn, (g_valueCount > 0 && !g_running));
    InvalidateRect(g_hStartBtn, NULL, TRUE);
}

static void AddValue(HWND hwnd) {
    wchar_t buf[MAX_VALUE_LEN];
    GetWindowTextW(g_hEdit, buf, MAX_VALUE_LEN);

    // 前後の空白をトリム
    wchar_t *start = buf;
    while (*start == L' ' || *start == L'\t') start++;
    wchar_t *end = start + wcslen(start);
    while (end > start && (*(end - 1) == L' ' || *(end - 1) == L'\t')) end--;
    *end = L'\0';

    if (wcslen(start) == 0) {
        MessageBoxW(hwnd, L"空の値は追加できません", L"カスタムルーレット", MB_OK | MB_ICONWARNING);
        return;
    }

    for (int i = 0; i < g_valueCount; i++) {
        if (wcscmp(g_values[i], start) == 0) {
            MessageBoxW(hwnd, L"同じ値は追加できません", L"カスタムルーレット", MB_OK | MB_ICONWARNING);
            return;
        }
    }

    if (g_valueCount >= MAX_VALUES) {
        MessageBoxW(hwnd, L"追加できる値は最大200個までです", L"カスタムルーレット", MB_OK | MB_ICONWARNING);
        return;
    }

    wcsncpy(g_values[g_valueCount], start, MAX_VALUE_LEN - 1);
    g_values[g_valueCount][MAX_VALUE_LEN - 1] = L'\0';
    g_valueCount++;

    RefreshListBox();
    SetWindowTextW(g_hEdit, L"");
    SetFocus(g_hEdit);
}

static void DeleteSelectedValue(void) {
    int sel = (int)SendMessageW(g_hListBox, LB_GETCURSEL, 0, 0);
    if (sel == LB_ERR) return;

    for (int i = sel; i < g_valueCount - 1; i++) {
        wcscpy(g_values[i], g_values[i + 1]);
    }
    g_valueCount--;
    RefreshListBox();
}

static void StartRoulette(HWND hwnd) {
    if (g_valueCount == 0) return;
    g_running = TRUE;
    EnableWindow(g_hStartBtn, FALSE);
    EnableWindow(g_hStopBtn, TRUE);
    InvalidateRect(g_hStartBtn, NULL, TRUE);
    InvalidateRect(g_hStopBtn, NULL, TRUE);
    SetTimer(hwnd, TIMER_ID, 100, NULL); // 0.1秒ごとに更新
}

static void StopRoulette(HWND hwnd) {
    g_running = FALSE;
    KillTimer(hwnd, TIMER_ID);
    EnableWindow(g_hStartBtn, (g_valueCount > 0));
    EnableWindow(g_hStopBtn, FALSE);
    InvalidateRect(g_hStartBtn, NULL, TRUE);
    InvalidateRect(g_hStopBtn, NULL, TRUE);
}

// ---- Editコントロール: Enterキーで追加 ----
static LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                          UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    (void)uIdSubclass;
    (void)dwRefData;
    if (msg == WM_KEYDOWN && wParam == VK_RETURN) {
        AddValue(GetParent(hwnd));
        return 0;
    }
    if (msg == WM_GETDLGCODE) {
        return DefSubclassProc(hwnd, msg, wParam, lParam) | DLGC_WANTALLKEYS;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

// ---- ボタン: マウスホバー検知 ----
static LRESULT CALLBACK ButtonSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                            UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    (void)uIdSubclass;
    ButtonState* state = (ButtonState*)dwRefData;

    switch (msg) {
    case WM_MOUSEMOVE:
        if (!state->hover) {
            state->hover = TRUE;
            TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd, 0 };
            TrackMouseEvent(&tme);
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;
    case WM_MOUSELEAVE:
        state->hover = FALSE;
        InvalidateRect(hwnd, NULL, TRUE);
        break;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

// 角丸ボタンを描画する
static void DrawOwnerButton(LPDRAWITEMSTRUCT dis, ButtonState* state) {
    HDC hdc = dis->hDC;
    RECT rc = dis->rcItem;
    BOOL disabled = (dis->itemState & ODS_DISABLED) != 0;
    BOOL hover = state->hover && !disabled;

    // ボタン矩形の外側(角丸の四隅)をページ背景色で塗ってから、その上に角丸を描く
    HBRUSH pageBg = CreateSolidBrush(COLOR_BG);
    FillRect(hdc, &rc, pageBg);
    DeleteObject(pageBg);

    COLORREF btnColor = disabled ? COLOR_BTN_DISABLED : (hover ? COLOR_BTN_HOVER : COLOR_BTN_NORMAL);
    HBRUSH btnBrush = CreateSolidBrush(btnColor);
    HPEN btnPen = CreatePen(PS_SOLID, 1, btnColor);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, btnBrush);
    HPEN oldPen = (HPEN)SelectObject(hdc, btnPen);
    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 8, 8);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(btnBrush);
    DeleteObject(btnPen);

    wchar_t text[64];
    GetWindowTextW(dis->hwndItem, text, 64);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, COLOR_WHITE);
    HFONT oldFont = (HFONT)SelectObject(hdc, g_hFontUI);
    DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, oldFont);
}

static HWND CreateOwnerButton(HWND parent, HINSTANCE hInst, const wchar_t* label, int id,
                               int x, int y, int w, int h, int stateIndex) {
    HWND btn = CreateWindowExW(0, L"BUTTON", label,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        x, y, w, h, parent, (HMENU)(INT_PTR)id, hInst, NULL);

    g_btnStates[stateIndex].hwnd = btn;
    g_btnStates[stateIndex].ctlId = id;
    g_btnStates[stateIndex].hover = FALSE;
    SetWindowSubclass(btn, ButtonSubclassProc, BUTTON_SUBCLASS_ID, (DWORD_PTR)&g_btnStates[stateIndex]);
    return btn;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        HINSTANCE hInst = ((LPCREATESTRUCTW)lParam)->hInstance;

        g_hFontTitle = CreateFontW(28, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Yu Gothic UI");

        g_hFontUI = CreateFontW(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Yu Gothic UI");

        g_hFontRoulette = CreateFontW(48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Yu Gothic UI");

        HWND hTitle = CreateWindowExW(0, L"STATIC", L"カスタムルーレット",
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            10, 10, 440, 36, hwnd, NULL, hInst, NULL);
        SendMessageW(hTitle, WM_SETFONT, (WPARAM)g_hFontTitle, TRUE);

        g_hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            10, 60, 260, 28, hwnd, (HMENU)IDC_EDIT_INPUT, hInst, NULL);
        SendMessageW(g_hEdit, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);
        SetWindowSubclass(g_hEdit, EditSubclassProc, EDIT_SUBCLASS_ID, 0);

        g_hAddBtn = CreateOwnerButton(hwnd, hInst, L"追加", IDC_BUTTON_ADD, 280, 58, 90, 32, 0);

        g_hListBox = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", NULL,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY,
            10, 100, 360, 150, hwnd, (HMENU)IDC_LISTBOX, hInst, NULL);
        SendMessageW(g_hListBox, WM_SETFONT, (WPARAM)g_hFontUI, TRUE);

        g_hDeleteBtn = CreateOwnerButton(hwnd, hInst, L"削除", IDC_BUTTON_DELETE, 10, 258, 100, 32, 1);

        g_hRoulette = CreateWindowExW(0, L"STATIC", L"-",
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            10, 310, 440, 100, hwnd, (HMENU)IDC_STATIC_ROULETTE, hInst, NULL);
        SendMessageW(g_hRoulette, WM_SETFONT, (WPARAM)g_hFontRoulette, TRUE);

        g_hStartBtn = CreateOwnerButton(hwnd, hInst, L"スタート", IDC_BUTTON_START, 100, 430, 110, 40, 2);
        EnableWindow(g_hStartBtn, FALSE);

        g_hStopBtn = CreateOwnerButton(hwnd, hInst, L"ストップ", IDC_BUTTON_STOP, 240, 430, 110, 40, 3);
        EnableWindow(g_hStopBtn, FALSE);

        return 0;
    }

    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
        if (dis->CtlType == ODT_BUTTON) {
            ButtonState* state = FindButtonState((int)dis->CtlID);
            if (state) {
                DrawOwnerButton(dis, state);
                return TRUE;
            }
        }
        return FALSE;
    }

    // 見出し・ルーレット表示の文字色とページ背景色を適用
    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        HWND hwndStatic = (HWND)lParam;
        SetBkMode(hdcStatic, TRANSPARENT);
        if (hwndStatic == g_hRoulette) {
            SetTextColor(hdcStatic, COLOR_ROULETTE);
        } else {
            SetTextColor(hdcStatic, COLOR_TEXT_DARK);
        }
        return (LRESULT)g_hBgBrush;
    }

    case WM_CTLCOLOREDIT: {
        HDC hdcEdit = (HDC)wParam;
        SetBkMode(hdcEdit, OPAQUE);
        SetBkColor(hdcEdit, COLOR_WHITE);
        SetTextColor(hdcEdit, COLOR_TEXT_DARK);
        return (LRESULT)GetStockObject(WHITE_BRUSH);
    }

    case WM_CTLCOLORLISTBOX: {
        HDC hdcList = (HDC)wParam;
        SetBkColor(hdcList, COLOR_WHITE);
        SetTextColor(hdcList, COLOR_TEXT_DARK);
        return (LRESULT)GetStockObject(WHITE_BRUSH);
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);
        if (code == BN_CLICKED) {
            switch (id) {
            case IDC_BUTTON_ADD:
                AddValue(hwnd);
                break;
            case IDC_BUTTON_DELETE:
                DeleteSelectedValue();
                break;
            case IDC_BUTTON_START:
                StartRoulette(hwnd);
                break;
            case IDC_BUTTON_STOP:
                StopRoulette(hwnd);
                break;
            }
        }
        return 0;
    }

    case WM_TIMER:
        if (wParam == TIMER_ID && g_valueCount > 0) {
            int idx = rand() % g_valueCount;
            SetWindowTextW(g_hRoulette, g_values[idx]);
        }
        return 0;

    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hwnd, &rc);
        FillRect(hdc, &rc, g_hBgBrush);
        return 1;
    }

    case WM_DESTROY:
        KillTimer(hwnd, TIMER_ID);
        DeleteObject(g_hBgBrush);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)pCmdLine;

    srand((unsigned int)time(NULL));

    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    g_hBgBrush = CreateSolidBrush(COLOR_BG);

    const wchar_t CLASS_NAME[] = L"CustomRouletteWindowClass";

    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = g_hBgBrush;
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APPICON));
    wc.hIconSm = wc.hIcon;

    RegisterClassExW(&wc);

    DWORD style = (WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX);

    RECT rect = { 0, 0, 480, 560 };
    AdjustWindowRect(&rect, style, FALSE);

    HWND hwnd = CreateWindowExW(
        0, CLASS_NAME, L"カスタムルーレット", style,
        CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
