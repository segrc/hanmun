// 한자 후보 창 (세로 목록)
// Copyright (c) 2026 세 (@segrc_) — MIT License
#include "CandidateWindow.h"

#include <dwmapi.h>

#include <algorithm>

#include "TextService.h"

namespace {
const wchar_t kClass[] = L"HanmunCandidateWindow";
const COLORREF kBg = RGB(255, 255, 255), kBorder = RGB(214, 220, 226), kText = RGB(28, 32, 36),
               kDim = RGB(128, 136, 144), kAccent = RGB(138, 204, 92), kAccentText = RGB(28, 40, 12),
               kPred = RGB(62, 128, 32);

int Scale(int v, UINT dpi) { return MulDiv(v, (int)dpi, 96); }

SIZE TextSize(HDC dc, HFONT f, const std::wstring& s) {
    SIZE sz = {0, 0};
    HGDIOBJ old = SelectObject(dc, f);
    GetTextExtentPoint32W(dc, s.c_str(), (int)s.size(), &sz);
    SelectObject(dc, old);
    return sz;
}

std::wstring Extra(const Candidate& c) {
    std::wstring e;
    if (c.pred) e = c.read;                       // 예측 후보는 전체 독음
    if (!c.mean.empty() && c.text.size() <= 2) { // 한 글자면 훈음
        if (!e.empty()) e += L"  ";
        e += c.mean;
    }
    return e;
}
}  // namespace

CandidateWindow::CandidateWindow(TextService* owner) : owner_(owner) {}

CandidateWindow::~CandidateWindow() {
    if (hwnd_) DestroyWindow(hwnd_);
    if (fMain_) DeleteObject(fMain_);
    if (fSmall_) DeleteObject(fSmall_);
}

HWND CandidateWindow::Hwnd() {
    if (!hwnd_) Create();
    return hwnd_;
}

bool CandidateWindow::Create() {
    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW wc = {sizeof(wc)};
        wc.style = CS_DROPSHADOW | CS_IME;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = g_hInst;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = kClass;
        if (!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;
        registered = true;
    }
    hwnd_ = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, kClass, L"", WS_POPUP, 0, 0, 10, 10,
                            nullptr, nullptr, g_hInst, this);
    if (!hwnd_) return false;
    DWORD corner = 3;  // DWMWCP_ROUNDSMALL (Windows 11)
    DwmSetWindowAttribute(hwnd_, 33 /* DWMWA_WINDOW_CORNER_PREFERENCE */, &corner, sizeof(corner));
    return true;
}

void CandidateWindow::MakeFonts(UINT dpi) {
    if (dpi == dpi_ && fMain_) return;
    if (fMain_) DeleteObject(fMain_);
    if (fSmall_) DeleteObject(fSmall_);
    dpi_ = dpi;
    auto mk = [&](int px, int weight) {
        return CreateFontW(-Scale(px, dpi), 0, 0, 0, weight, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Malgun Gothic");
    };
    fMain_ = mk(19, FW_NORMAL);
    fSmall_ = mk(12, FW_NORMAL);
}

SIZE CandidateWindow::Measure() {
    HDC dc = GetDC(hwnd_);
    pad_ = Scale(5, dpi_);
    gap_ = Scale(10, dpi_);
    SIZE m = TextSize(dc, fMain_, L"漢가");
    rowH_ = m.cy + Scale(7, dpi_);
    numW_ = TextSize(dc, fSmall_, L"9").cx;
    textW_ = extraW_ = 0;
    for (auto& c : items_) {
        textW_ = std::max<int>(textW_, TextSize(dc, fMain_, c.text).cx);
        std::wstring e = Extra(c);
        if (!e.empty()) extraW_ = std::max<int>(extraW_, TextSize(dc, fSmall_, e).cx);
    }
    extraW_ = std::min<int>(extraW_, Scale(220, dpi_));
    footH_ = pages_ > 1 ? TextSize(dc, fSmall_, L"1/2").cy + Scale(4, dpi_) : 0;
    ReleaseDC(hwnd_, dc);
    int w = pad_ * 2 + Scale(8, dpi_) + numW_ + gap_ + textW_ + (extraW_ ? gap_ + extraW_ : 0) + Scale(10, dpi_);
    w = std::max<int>(w, Scale(120, dpi_));
    int h = pad_ * 2 + rowH_ * (int)items_.size() + footH_;
    return {w, h};
}

void CandidateWindow::Show(const RECT& anchor, const std::vector<Candidate>& cands, int sel) {
    if (cands.empty()) { Hide(); return; }
    if (!hwnd_ && !Create()) return;
    sel_ = sel;
    page_ = sel / kPage;
    pages_ = ((int)cands.size() + kPage - 1) / kPage;
    base_ = page_ * kPage;
    items_.assign(cands.begin() + base_, cands.begin() + std::min<size_t>(cands.size(), base_ + kPage));

    POINT pt = {anchor.left, anchor.bottom};
    HMONITOR mon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    UINT dpi = GetDpiForWindow(hwnd_);
    MakeFonts(dpi ? dpi : 96);
    SIZE sz = Measure();

    MONITORINFO mi = {sizeof(mi)};
    GetMonitorInfoW(mon, &mi);
    const RECT& wa = mi.rcWork;
    int x = anchor.left - pad_ - Scale(8, dpi_) - numW_ - gap_ / 2, y = anchor.bottom + Scale(3, dpi_);
    if (y + sz.cy > wa.bottom) y = anchor.top - sz.cy - Scale(3, dpi_);   // 아래 자리가 없으면 위로
    x = std::max<int>(wa.left, std::min<int>(x, wa.right - sz.cx));
    y = std::max<int>(wa.top, y);
    SetWindowPos(hwnd_, HWND_TOPMOST, x, y, sz.cx, sz.cy, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void CandidateWindow::Hide() {
    if (hwnd_) ShowWindow(hwnd_, SW_HIDE);
}

void CandidateWindow::Paint(HDC dc) {
    RECT rc;
    GetClientRect(hwnd_, &rc);
    HBRUSH bg = CreateSolidBrush(kBg);
    FillRect(dc, &rc, bg);
    DeleteObject(bg);
    HBRUSH border = CreateSolidBrush(kBorder);
    FrameRect(dc, &rc, border);
    DeleteObject(border);
    SetBkMode(dc, TRANSPARENT);

    for (int i = 0; i < (int)items_.size(); i++) {
        const Candidate& c = items_[i];
        const bool on = base_ + i == sel_;
        RECT row = {pad_, pad_ + i * rowH_, rc.right - pad_, pad_ + (i + 1) * rowH_};
        if (on) {
            HBRUSH b = CreateSolidBrush(kAccent);
            HPEN p = CreatePen(PS_SOLID, 1, kAccent);
            HGDIOBJ ob = SelectObject(dc, b), op = SelectObject(dc, p);
            RoundRect(dc, row.left, row.top, row.right, row.bottom, Scale(8, dpi_), Scale(8, dpi_));
            SelectObject(dc, ob);
            SelectObject(dc, op);
            DeleteObject(b);
            DeleteObject(p);
        }
        int x = row.left + Scale(8, dpi_);
        // 번호
        SelectObject(dc, fSmall_);
        SetTextColor(dc, on ? kAccentText : kDim);
        wchar_t num[4] = {wchar_t(L'1' + i), 0};
        RECT r = {x, row.top, x + numW_, row.bottom};
        DrawTextW(dc, num, -1, &r, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOPREFIX);
        x += numW_ + gap_;
        // 한자
        SelectObject(dc, fMain_);
        SetTextColor(dc, on ? kAccentText : kText);
        r = {x, row.top, x + textW_ + 2, row.bottom};
        DrawTextW(dc, c.text.c_str(), (int)c.text.size(), &r, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOPREFIX);
        x += textW_ + gap_;
        // 독음·훈음
        std::wstring e = Extra(c);
        if (!e.empty()) {
            SelectObject(dc, fSmall_);
            SetTextColor(dc, on ? kAccentText : (c.pred ? kPred : kDim));
            r = {x, row.top, row.right - Scale(6, dpi_), row.bottom};
            DrawTextW(dc, e.c_str(), (int)e.size(), &r, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOPREFIX | DT_END_ELLIPSIS);
        }
    }
    if (pages_ > 1) {
        SelectObject(dc, fSmall_);
        SetTextColor(dc, kDim);
        wchar_t buf[32];
        wsprintfW(buf, L"%d/%d  Tab ▸", page_ + 1, pages_);
        RECT r = {pad_, rc.bottom - pad_ - footH_, rc.right - pad_ - Scale(4, dpi_), rc.bottom - pad_};
        DrawTextW(dc, buf, -1, &r, DT_SINGLELINE | DT_VCENTER | DT_RIGHT | DT_NOPREFIX);
    }
}

LRESULT CALLBACK CandidateWindow::WndProc(HWND h, UINT msg, WPARAM w, LPARAM l) {
    auto* self = (CandidateWindow*)GetWindowLongPtrW(h, GWLP_USERDATA);
    switch (msg) {
        case WM_NCCREATE: {
            auto* cs = (CREATESTRUCTW*)l;
            SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
            break;
        }
        case WM_MOUSEACTIVATE: return MA_NOACTIVATE;
        case WM_ERASEBKGND: return 1;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(h, &ps);
            if (self) {
                RECT rc;
                GetClientRect(h, &rc);
                HDC mem = CreateCompatibleDC(dc);
                HBITMAP bmp = CreateCompatibleBitmap(dc, rc.right, rc.bottom);
                HGDIOBJ old = SelectObject(mem, bmp);
                self->Paint(mem);
                BitBlt(dc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
                SelectObject(mem, old);
                DeleteObject(bmp);
                DeleteDC(mem);
            }
            EndPaint(h, &ps);
            return 0;
        }
        case WM_LBUTTONUP:
            if (self && self->rowH_) {
                int y = (short)HIWORD(l) - self->pad_;
                int i = y / self->rowH_;
                if (y >= 0 && i < (int)self->items_.size()) self->owner_->OnCandidateClicked(self->base_ + i);
            }
            return 0;
        case WM_DPICHANGED: return 0;
    }
    return DefWindowProcW(h, msg, w, l);
}
