// 작업 표시줄 입력 표시기 버튼 (A / 가 / 漢)
// Copyright (c) 2026 세 (@segrc_) — MIT License
#include "LangBarButton.h"

#include <vector>

#include "TextService.h"

namespace {
const wchar_t* ModeText(Mode m) { return m == Mode::English ? L"A" : m == Mode::Hangul ? L"가" : L"漢"; }

// 흑백 아이콘: 시스템이 밝은/어두운 작업 표시줄에 맞춰 색을 바꿔 그린다
HICON MakeModeIcon(const wchar_t* text) {
    int cx = GetSystemMetrics(SM_CXSMICON), cy = GetSystemMetrics(SM_CYSMICON);
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = cx;
    bi.bmiHeader.biHeight = -cy;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    void* bits = nullptr;
    HDC dc = CreateCompatibleDC(nullptr);
    HBITMAP dib = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!dib) { DeleteDC(dc); return nullptr; }
    HGDIOBJ oldBmp = SelectObject(dc, dib);
    RECT rc = {0, 0, cx, cy};
    FillRect(dc, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));
    HFONT f = CreateFontW(-(cy * (text[0] == L'A' ? 15 : 14) / 16), 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, DEFAULT_PITCH, L"Malgun Gothic");
    HGDIOBJ oldFont = SelectObject(dc, f);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(0, 0, 0));
    DrawTextW(dc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(dc, oldFont);
    DeleteObject(f);
    GdiFlush();

    // 1비트 마스크 두 장 (AND: 1=투명, XOR: 0)
    int stride = ((cx + 15) / 16) * 2;
    std::vector<BYTE> andBits(stride * cy, 0xFF), xorBits(stride * cy, 0);
    const DWORD* px = (const DWORD*)bits;
    for (int y = 0; y < cy; y++)
        for (int x = 0; x < cx; x++)
            if ((px[y * cx + x] & 0xFF) < 128) andBits[y * stride + x / 8] &= (BYTE) ~(0x80 >> (x % 8));
    SelectObject(dc, oldBmp);
    DeleteObject(dib);
    DeleteDC(dc);
    return CreateIcon(g_hInst, cx, cy, 1, 1, andBits.data(), xorBits.data());
}
}  // namespace

LangBarButton::LangBarButton(TextService* owner) : owner_(owner) { DllAddRef(); }
LangBarButton::~LangBarButton() {
    SafeRelease(sink_);
    DllRelease();
}

STDMETHODIMP LangBarButton::QueryInterface(REFIID riid, void** ppv) {
    if (!ppv) return E_INVALIDARG;
    *ppv = nullptr;
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfLangBarItem) || IsEqualIID(riid, IID_ITfLangBarItemButton))
        *ppv = static_cast<ITfLangBarItemButton*>(this);
    else if (IsEqualIID(riid, IID_ITfSource)) *ppv = static_cast<ITfSource*>(this);
    if (!*ppv) return E_NOINTERFACE;
    AddRef();
    return S_OK;
}
STDMETHODIMP_(ULONG) LangBarButton::AddRef() { return InterlockedIncrement(&ref_); }
STDMETHODIMP_(ULONG) LangBarButton::Release() {
    LONG r = InterlockedDecrement(&ref_);
    if (!r) delete this;
    return r;
}

STDMETHODIMP LangBarButton::GetInfo(TF_LANGBARITEMINFO* info) {
    if (!info) return E_INVALIDARG;
    info->clsidService = CLSID_Hanmun;
    info->guidItem = GUID_InputModeItem;
    info->dwStyle = TF_LBI_STYLE_BTN_BUTTON | TF_LBI_STYLE_SHOWNINTRAY;
    info->ulSort = 0;
    lstrcpynW(info->szDescription, L"빠른 한문 입력기 입력 상태", TF_LBI_DESC_MAXLEN);
    return S_OK;
}

STDMETHODIMP LangBarButton::GetTooltipString(BSTR* tip) {
    if (!tip) return E_INVALIDARG;
    Mode m = owner_ ? owner_->GetMode() : Mode::English;
    const wchar_t* s = m == Mode::English ? L"영문 — 한/영 키로 한글" : m == Mode::Hangul ? L"한글 — 한자 키로 한문(漢) 모드"
                                                                                  : L"한문(漢) — 한자 후보 자동, 한자 키로 한글";
    *tip = SysAllocString(s);
    return *tip ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP LangBarButton::GetText(BSTR* text) {
    if (!text) return E_INVALIDARG;
    *text = SysAllocString(ModeText(owner_ ? owner_->GetMode() : Mode::English));
    return *text ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP LangBarButton::GetIcon(HICON* icon) {
    if (!icon) return E_INVALIDARG;
    *icon = MakeModeIcon(ModeText(owner_ ? owner_->GetMode() : Mode::English));
    return *icon ? S_OK : E_FAIL;
}

STDMETHODIMP LangBarButton::OnClick(TfLBIClick click, POINT pt, const RECT*) {
    if (!owner_) return S_OK;
    if (click == TF_LBI_CLK_LEFT) {
        // 왼쪽 클릭: A → 가 → 漢 → A
        Mode m = owner_->GetMode();
        owner_->SetModeFromUser(m == Mode::English ? Mode::Hangul : m == Mode::Hangul ? Mode::Hanmun : Mode::English);
    } else {
        owner_->ShowMenu(pt);
    }
    return S_OK;
}

STDMETHODIMP LangBarButton::AdviseSink(REFIID riid, IUnknown* punk, DWORD* cookie) {
    if (!IsEqualIID(riid, IID_ITfLangBarItemSink)) return CONNECT_E_CANNOTCONNECT;
    if (sink_) return CONNECT_E_ADVISELIMIT;
    if (!punk || FAILED(punk->QueryInterface(IID_ITfLangBarItemSink, (void**)&sink_))) { sink_ = nullptr; return E_NOINTERFACE; }
    *cookie = 1;
    return S_OK;
}

STDMETHODIMP LangBarButton::UnadviseSink(DWORD cookie) {
    if (cookie != 1 || !sink_) return CONNECT_E_NOCONNECTION;
    SafeRelease(sink_);
    return S_OK;
}

void LangBarButton::Update() {
    if (sink_) sink_->OnUpdate(TF_LBI_ICON | TF_LBI_TEXT | TF_LBI_TOOLTIP);
}
