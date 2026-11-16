// 입력기 본체 (TSF 텍스트 서비스)
// Copyright (c) 2026 세 (@segrc_) — MIT License
#include "TextService.h"

#include <shlobj.h>

#include <mutex>

#include "CandidateWindow.h"
#include "DisplayAttribute.h"
#include "LangBarButton.h"

// ─────────────────────────────────────────── 공용 엔진·사용자 사전

namespace {
std::mutex g_engineLock;
FILETIME g_userStamp = {};

std::wstring UserFilePath() {
    PWSTR p = nullptr;
    std::wstring path;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &p))) path = std::wstring(p) + L"\\Hanmun";
    CoTaskMemFree(p);
    return path;
}

std::wstring Utf8ToWide(const char* s, int n) {
    if (n <= 0) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s, n, nullptr, 0);
    std::wstring w(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s, n, &w[0], len);
    return w;
}

std::string WideToUtf8(const std::wstring& w) {
    if (w.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], len, nullptr, nullptr);
    return s;
}

bool FileStamp(const std::wstring& file, FILETIME* ft) {
    WIN32_FILE_ATTRIBUTE_DATA fa;
    if (!GetFileAttributesExW(file.c_str(), GetFileExInfoStandard, &fa)) return false;
    *ft = fa.ftLastWriteTime;
    return true;
}

// 호출하는 쪽이 g_engineLock 을 잡고 있어야 한다
void LoadUserFile(Engine& e) {
    std::wstring dir = UserFilePath();
    if (dir.empty()) return;
    std::wstring file = dir + L"\\user.tsv";
    HANDLE h = CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                           OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) { e.ClearUser(); g_userStamp = {}; return; }
    LARGE_INTEGER size;
    std::string buf;
    if (GetFileSizeEx(h, &size) && size.QuadPart < (64 << 20)) {
        buf.resize((size_t)size.QuadPart);
        DWORD got = 0;
        if (!buf.empty()) ReadFile(h, &buf[0], (DWORD)buf.size(), &got, nullptr);
        buf.resize(got);
    }
    FILETIME ft;
    GetFileTime(h, nullptr, nullptr, &ft);
    CloseHandle(h);
    g_userStamp = ft;
    e.LoadUser(Utf8ToWide(buf.data(), (int)buf.size()));
}
}  // namespace

Engine& SharedEngine() {
    static Engine engine;
    static std::once_flag once;
    std::call_once(once, [] {
        HRSRC r = FindResourceW(g_hInst, MAKEINTRESOURCEW(IDR_DICT), MAKEINTRESOURCEW(10) /* RT_RCDATA */);
        if (r) {
            HGLOBAL g = LoadResource(g_hInst, r);
            const char* p = g ? (const char*)LockResource(g) : nullptr;
            if (p) engine.Load(Utf8ToWide(p, (int)SizeofResource(g_hInst, r)));
        }
        LoadUserFile(engine);
    });
    return engine;
}

void TextService::ReloadUserIfChanged() {
    std::wstring dir = UserFilePath();
    FILETIME ft = {};
    bool exists = !dir.empty() && FileStamp(dir + L"\\user.tsv", &ft);
    if (exists ? CompareFileTime(&ft, &g_userStamp) != 0 : (g_userStamp.dwLowDateTime || g_userStamp.dwHighDateTime))
        LoadUserFile(SharedEngine());
}

void TextService::SaveUserLine(const std::wstring& line) {
    if (line.empty()) return;
    std::wstring dir = UserFilePath();
    if (dir.empty()) return;
    CreateDirectoryW(dir.c_str(), nullptr);
    HANDLE h = CreateFileW((dir + L"\\user.tsv").c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return;  // 저장할 수 없는 환경(일부 앱 샌드박스)에서는 이번 실행 동안만 기억
    std::string s = WideToUtf8(line);
    DWORD put = 0;
    WriteFile(h, s.data(), (DWORD)s.size(), &put, nullptr);
    FILETIME ft;
    if (GetFileTime(h, nullptr, nullptr, &ft)) g_userStamp = ft;
    CloseHandle(h);
}

// ─────────────────────────────────────────── 편집 세션

namespace {
class EditSession : public ITfEditSession {
public:
    EditSession(IUnknown* owner, std::function<void(TfEditCookie)> fn) : owner_(owner), fn_(std::move(fn)) { owner_->AddRef(); }
    ~EditSession() { owner_->Release(); }
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) return E_INVALIDARG;
        *ppv = nullptr;
        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfEditSession)) *ppv = static_cast<ITfEditSession*>(this);
        if (!*ppv) return E_NOINTERFACE;
        AddRef();
        return S_OK;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&ref_); }
    STDMETHODIMP_(ULONG) Release() override {
        LONG r = InterlockedDecrement(&ref_);
        if (!r) delete this;
        return r;
    }
    STDMETHODIMP DoEditSession(TfEditCookie ec) override { fn_(ec); return S_OK; }

private:
    LONG ref_ = 1;
    IUnknown* owner_;
    std::function<void(TfEditCookie)> fn_;
};

// interim: 한국어 입력기 방식(조합 중인 한 글자를 블록으로 선택). 카카오톡처럼
// 옛 IMM 방식으로 한글을 받는 앱은 이렇게 해야 자모가 갈라지지 않는다.
void SetSelectionEnd(TfEditCookie ec, ITfContext* ctx, ITfRange* range, bool interim = false) {
    ITfRange* r = nullptr;
    if (FAILED(range->Clone(&r))) return;
    if (!interim) r->Collapse(ec, TF_ANCHOR_END);
    TF_SELECTION sel = {};
    sel.range = r;
    sel.style.ase = TF_AE_NONE;
    sel.style.fInterimChar = interim ? TRUE : FALSE;
    ctx->SetSelection(ec, 1, &sel);
    r->Release();
}
}  // namespace

HRESULT TextService::RunEdit(ITfContext* ctx, DWORD flags, std::function<void(TfEditCookie)> fn) {
    if (!ctx) return E_FAIL;
    auto* es = new EditSession(static_cast<ITfTextInputProcessorEx*>(this), std::move(fn));
    HRESULT hrSession = E_FAIL;
    HRESULT hr = ctx->RequestEditSession(tid_, es, flags, &hrSession);
    es->Release();
    return SUCCEEDED(hr) ? hrSession : hr;
}

// ─────────────────────────────────────────── COM

TextService::TextService() { DllAddRef(); }
TextService::~TextService() { DllRelease(); }

STDMETHODIMP TextService::QueryInterface(REFIID riid, void** ppv) {
    if (!ppv) return E_INVALIDARG;
    *ppv = nullptr;
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfTextInputProcessor) ||
        IsEqualIID(riid, IID_ITfTextInputProcessorEx))
        *ppv = static_cast<ITfTextInputProcessorEx*>(this);
    else if (IsEqualIID(riid, IID_ITfThreadMgrEventSink)) *ppv = static_cast<ITfThreadMgrEventSink*>(this);
    else if (IsEqualIID(riid, IID_ITfKeyEventSink)) *ppv = static_cast<ITfKeyEventSink*>(this);
    else if (IsEqualIID(riid, IID_ITfCompositionSink)) *ppv = static_cast<ITfCompositionSink*>(this);
    else if (IsEqualIID(riid, IID_ITfCompartmentEventSink)) *ppv = static_cast<ITfCompartmentEventSink*>(this);
    else if (IsEqualIID(riid, IID_ITfDisplayAttributeProvider)) *ppv = static_cast<ITfDisplayAttributeProvider*>(this);
    if (!*ppv) return E_NOINTERFACE;
    AddRef();
    return S_OK;
}
STDMETHODIMP_(ULONG) TextService::AddRef() { return InterlockedIncrement(&ref_); }
STDMETHODIMP_(ULONG) TextService::Release() {
    LONG r = InterlockedDecrement(&ref_);
    if (!r) delete this;
    return r;
}

// ─────────────────────────────────────────── 켜기 / 끄기

STDMETHODIMP TextService::Activate(ITfThreadMgr* tm, TfClientId tid) { return ActivateEx(tm, tid, 0); }

STDMETHODIMP TextService::ActivateEx(ITfThreadMgr* tm, TfClientId tid, DWORD) {
    tm_ = tm;
    tm_->AddRef();
    tid_ = tid;

    ITfSource* src = nullptr;
    if (SUCCEEDED(tm_->QueryInterface(IID_ITfSource, (void**)&src))) {
        src->AdviseSink(IID_ITfThreadMgrEventSink, static_cast<ITfThreadMgrEventSink*>(this), &tmCookie_);
        src->Release();
    }
    ITfKeystrokeMgr* km = nullptr;
    if (SUCCEEDED(tm_->QueryInterface(IID_ITfKeystrokeMgr, (void**)&km))) {
        km->AdviseKeyEventSink(tid_, static_cast<ITfKeyEventSink*>(this), TRUE);
        km->Release();
    }
    ITfCompartmentMgr* cm = nullptr;
    if (SUCCEEDED(tm_->QueryInterface(IID_ITfCompartmentMgr, (void**)&cm))) {
        ITfCompartment* c = nullptr;
        if (SUCCEEDED(cm->GetCompartment(GUID_COMPARTMENT_KEYBOARD_OPENCLOSE, &c))) {
            if (SUCCEEDED(c->QueryInterface(IID_ITfSource, (void**)&src))) {
                src->AdviseSink(IID_ITfCompartmentEventSink, static_cast<ITfCompartmentEventSink*>(this), &compCookie_);
                src->Release();
            }
            c->Release();
        }
        cm->Release();
    }
    ITfCategoryMgr* cat = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr, (void**)&cat))) {
        cat->RegisterGUID(GUID_HanmunDisplayAttr, &attrAtom_);
        cat->Release();
    }

    cand_ = new CandidateWindow(this);
    button_ = new LangBarButton(this);
    ITfLangBarItemMgr* lm = nullptr;
    if (SUCCEEDED(tm_->QueryInterface(IID_ITfLangBarItemMgr, (void**)&lm))) {
        lm->AddItem(button_);
        lm->Release();
    }

    SharedEngine();  // 사전을 미리 읽어 둔다
    ApplyMode(Settings::KoreanMode());
    return S_OK;
}

STDMETHODIMP TextService::Deactivate() {
    if (comp_ && compCtx_) {
        ITfContext* ctx = compCtx_;
        ctx->AddRef();
        RunEdit(ctx, TF_ES_SYNC | TF_ES_READWRITE, [this, ctx](TfEditCookie ec) { CommitAll(ec, ctx); });
        ctx->Release();
    }
    ResetState();
    SafeRelease(comp_);
    SafeRelease(compCtx_);

    if (tm_) {
        ITfSource* src = nullptr;
        if (tmCookie_ != TF_INVALID_COOKIE && SUCCEEDED(tm_->QueryInterface(IID_ITfSource, (void**)&src))) {
            src->UnadviseSink(tmCookie_);
            src->Release();
        }
        ITfKeystrokeMgr* km = nullptr;
        if (SUCCEEDED(tm_->QueryInterface(IID_ITfKeystrokeMgr, (void**)&km))) {
            km->UnadviseKeyEventSink(tid_);
            km->Release();
        }
        ITfCompartmentMgr* cm = nullptr;
        if (compCookie_ != TF_INVALID_COOKIE && SUCCEEDED(tm_->QueryInterface(IID_ITfCompartmentMgr, (void**)&cm))) {
            ITfCompartment* c = nullptr;
            if (SUCCEEDED(cm->GetCompartment(GUID_COMPARTMENT_KEYBOARD_OPENCLOSE, &c))) {
                if (SUCCEEDED(c->QueryInterface(IID_ITfSource, (void**)&src))) {
                    src->UnadviseSink(compCookie_);
                    src->Release();
                }
                c->Release();
            }
            cm->Release();
        }
        ITfLangBarItemMgr* lm = nullptr;
        if (button_ && SUCCEEDED(tm_->QueryInterface(IID_ITfLangBarItemMgr, (void**)&lm))) {
            lm->RemoveItem(button_);
            lm->Release();
        }
    }
    tmCookie_ = compCookie_ = TF_INVALID_COOKIE;
    if (button_) { button_->Detach(); button_->Release(); button_ = nullptr; }
    delete cand_;
    cand_ = nullptr;
    SafeRelease(tm_);
    tid_ = TF_CLIENTID_NULL;
    return S_OK;
}

// ─────────────────────────────────────────── 한/영, 한자 상태

bool TextService::IsOpen() {
    bool open = false;
    ITfCompartmentMgr* cm = nullptr;
    if (tm_ && SUCCEEDED(tm_->QueryInterface(IID_ITfCompartmentMgr, (void**)&cm))) {
        ITfCompartment* c = nullptr;
        if (SUCCEEDED(cm->GetCompartment(GUID_COMPARTMENT_KEYBOARD_OPENCLOSE, &c))) {
            VARIANT v;
            VariantInit(&v);
            if (c->GetValue(&v) == S_OK && v.vt == VT_I4) open = v.lVal != 0;
            c->Release();
        }
        cm->Release();
    }
    return open;
}

void TextService::SetOpen(bool open) {
    ITfCompartmentMgr* cm = nullptr;
    if (tm_ && SUCCEEDED(tm_->QueryInterface(IID_ITfCompartmentMgr, (void**)&cm))) {
        ITfCompartment* c = nullptr;
        if (SUCCEEDED(cm->GetCompartment(GUID_COMPARTMENT_KEYBOARD_OPENCLOSE, &c))) {
            VARIANT v;
            v.vt = VT_I4;
            v.lVal = open ? 1 : 0;
            c->SetValue(tid_, &v);
            c->Release();
        }
        cm->Release();
    }
}

void TextService::ApplyMode(Mode m) {
    if (m != Mode::English) Settings::SetKoreanMode(m);
    mode_ = m;
    SetOpen(m != Mode::English);
    Refresh();
    if (cand_) cand_->Hide();
    if (button_) button_->Update();
}

STDMETHODIMP TextService::OnChange(REFGUID guid) {
    if (!IsEqualGUID(guid, GUID_COMPARTMENT_KEYBOARD_OPENCLOSE)) return S_OK;
    Mode m = IsOpen() ? Settings::KoreanMode() : Mode::English;
    if (m == mode_) return S_OK;
    if (Composing() && compCtx_) {
        ITfContext* ctx = compCtx_;
        RunEdit(ctx, TF_ES_ASYNCDONTCARE | TF_ES_READWRITE, [this](TfEditCookie ec) { if (compCtx_) CommitAll(ec, compCtx_); });
    }
    mode_ = m;
    if (cand_) cand_->Hide();
    if (button_) button_->Update();
    return S_OK;
}

void TextService::SetModeFromUser(Mode m) {
    if (Composing() && compCtx_)
        RunEdit(compCtx_, TF_ES_ASYNCDONTCARE | TF_ES_READWRITE, [this](TfEditCookie ec) { if (compCtx_) CommitAll(ec, compCtx_); });
    ApplyMode(m);
}

// ─────────────────────────────────────────── 포커스·조합 종료

STDMETHODIMP TextService::OnSetFocus(ITfDocumentMgr*, ITfDocumentMgr*) {
    if (cand_) cand_->Hide();
    if (Composing() && compCtx_)
        RunEdit(compCtx_, TF_ES_ASYNCDONTCARE | TF_ES_READWRITE, [this](TfEditCookie ec) { if (compCtx_) CommitAll(ec, compCtx_); });
    lastChar_.clear();
    SyncMode();
    if (button_) button_->Update();
    return S_OK;
}

STDMETHODIMP TextService::OnCompositionTerminated(TfEditCookie, ITfComposition* comp) {
    if (comp == comp_ || !comp) {
        SafeRelease(comp_);
        SafeRelease(compCtx_);
        ResetState();
    }
    return S_OK;
}

void TextService::ResetState() {
    hg_.Reset();
    run_.clear();
    picks_.clear();
    cands_.clear();
    sel_ = 0;
    if (cand_) cand_->Hide();
}

// ─────────────────────────────────────────── 키 입력

TextService::Action TextService::Classify(WPARAM vk) const {
    Action a;
    const bool shift = GetKeyState(VK_SHIFT) < 0, ctrl = GetKeyState(VK_CONTROL) < 0, alt = GetKeyState(VK_MENU) < 0;
    if (vk == VK_HANGUL) { a.act = Act::ToggleHangul; return a; }
    if (vk == VK_HANJA) { a.act = Act::ToggleHanja; return a; }
    if (mode_ == Mode::English) return a;
    switch (vk) {
        case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT: case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
        case VK_MENU: case VK_LMENU: case VK_RMENU: case VK_CAPITAL: case VK_LWIN: case VK_RWIN:
        case VK_NUMLOCK: case VK_SCROLL: case VK_APPS:
            return a;
    }
    const bool comp = Composing();
    if (ctrl || alt) { if (comp) a.act = Act::CommitPass; return a; }

    if (vk >= 'A' && vk <= 'Z') {
        a.act = Act::Jamo;
        a.jamo = Hangul::KeyToJamo((unsigned)vk, shift);
        return a;
    }
    const int n = (int)cands_.size();
    if (mode_ == Mode::Hanmun && n) {
        switch (vk) {
            case VK_SPACE: a.act = shift ? Act::Prev : Act::Next; return a;
            case VK_DOWN: a.act = Act::Next; return a;
            case VK_UP: a.act = Act::Prev; return a;
            case VK_TAB: a.act = shift ? Act::PagePrev : Act::PageNext; return a;
            case VK_NEXT: a.act = Act::PageNext; return a;
            case VK_PRIOR: a.act = Act::PagePrev; return a;
            case VK_RETURN: a.act = Act::Pick; a.index = sel_; return a;
        }
        int d = -1;
        if (vk >= '1' && vk <= '9' && !shift) d = (int)vk - '1';
        else if (vk >= VK_NUMPAD1 && vk <= VK_NUMPAD9) d = (int)vk - VK_NUMPAD1;
        if (d >= 0) {
            int idx = (sel_ / CandidateWindow::kPage) * CandidateWindow::kPage + d;
            if (idx < n) { a.act = Act::Pick; a.index = idx; return a; }
        }
    }
    if (!comp) return a;
    if (vk == VK_BACK) a.act = Act::Back;
    else if (vk == VK_ESCAPE) a.act = Act::CommitEat;
    else if (vk == VK_RETURN) a.act = mode_ == Mode::Hanmun ? Act::CommitEat : Act::CommitPass;
    else a.act = Act::CommitPass;
    return a;
}

// 한글/한문 구분은 모든 앱이 함께 쓰는 설정이다. 작업 표시줄에서 바꾸면 그 순간 포커스가
// 작업 표시줄(탐색기)에 있으므로, 각 앱은 키를 받을 때 설정을 다시 읽어 맞춘다.
void TextService::SyncMode() {
    if (mode_ == Mode::English) return;
    Mode k = Settings::KoreanMode();
    if (k == mode_) return;
    mode_ = k;
    Refresh();
    if (cand_ && cands_.empty()) cand_->Hide();
    if (button_) button_->Update();
}

namespace {
bool IsRightCtrl(WPARAM w, LPARAM l) { return (w == VK_CONTROL || w == VK_RCONTROL) && (l & (1 << 24)); }
}

STDMETHODIMP TextService::OnTestKeyDown(ITfContext* ctx, WPARAM w, LPARAM l, BOOL* eaten) {
    // 오른쪽 Ctrl 을 혼자 눌렀다 떼면 한자 키와 같게 (노트북에서 한자 키 = 오른쪽 Ctrl 인 경우)
    if (IsRightCtrl(w, l)) { if (!(l & (1 << 30))) rctrlAlone_ = true; }
    else rctrlAlone_ = false;
    SyncMode();
    *eaten = ctx && Classify(w).act != Act::None;
    return S_OK;
}

STDMETHODIMP TextService::OnKeyDown(ITfContext* ctx, WPARAM w, LPARAM, BOOL* eaten) {
    *eaten = FALSE;
    if (!ctx) return S_OK;
    SyncMode();
    Action a = Classify(w);
    if (a.act == Act::None) return S_OK;
    RunEdit(ctx, TF_ES_SYNC | TF_ES_READWRITE, [this, ctx, a](TfEditCookie ec) { Execute(ec, ctx, a); });
    *eaten = a.act != Act::CommitPass;
    return S_OK;
}

STDMETHODIMP TextService::OnTestKeyUp(ITfContext* ctx, WPARAM w, LPARAM l, BOOL* eaten) {
    // 오른쪽 Ctrl 떼기는 앱에도 그대로 보낸다 (OnKeyUp 에서 eaten = FALSE)
    *eaten = w == VK_HANGUL || w == VK_HANJA || (ctx && IsRightCtrl(w, l) && rctrlAlone_);
    return S_OK;
}

STDMETHODIMP TextService::OnKeyUp(ITfContext* ctx, WPARAM w, LPARAM l, BOOL* eaten) {
    *eaten = w == VK_HANGUL || w == VK_HANJA;
    if (ctx && IsRightCtrl(w, l) && rctrlAlone_) {
        rctrlAlone_ = false;
        Action a;
        a.act = Act::ToggleHanja;
        RunEdit(ctx, TF_ES_SYNC | TF_ES_READWRITE, [this, ctx, a](TfEditCookie ec) { Execute(ec, ctx, a); });
    }
    return S_OK;
}

void TextService::Execute(TfEditCookie ec, ITfContext* ctx, const Action& a) {
    const int n = (int)cands_.size();
    const int page = CandidateWindow::kPage;
    switch (a.act) {
        case Act::None: break;
        case Act::ToggleHangul:
            CommitAll(ec, ctx);
            ApplyMode(mode_ == Mode::English ? Settings::KoreanMode() : Mode::English);
            break;
        case Act::ToggleHanja:
            CommitAll(ec, ctx);
            ApplyMode(mode_ == Mode::Hanmun ? Mode::Hangul : Mode::Hanmun);
            break;
        case Act::Jamo: {
            if (!a.jamo) break;
            std::wstring out = hg_.Feed(a.jamo), commit;
            if (mode_ == Mode::Hanmun) {
                if (!out.empty()) {
                    if (Hangul::IsSyllable(out[0])) run_ += out;
                    else { commit = PickedText(true) + run_ + out; run_.clear(); picks_.clear(); }   // 낱자모는 그대로 내보낸다
                }
                if (picks_.empty() && run_.size() > 16) {   // 너무 길면 앞부분은 한글로 확정
                    commit += run_.substr(0, run_.size() - 16);
                    run_.erase(0, run_.size() - 16);
                }
                Refresh();
            } else commit = out;
            Update(ec, ctx, commit);
            break;
        }
        case Act::Back:
            if (!hg_.Empty()) hg_.Backspace();
            else if (!run_.empty()) run_.pop_back();
            else if (!picks_.empty()) { run_ = picks_.back().first + run_; picks_.pop_back(); }   // 고른 것 되돌리기
            Refresh();
            Update(ec, ctx, L"");
            break;
        case Act::Next: if (n) { sel_ = (sel_ + 1) % n; ShowCandidates(ec, ctx); } break;
        case Act::Prev: if (n) { sel_ = (sel_ + n - 1) % n; ShowCandidates(ec, ctx); } break;
        case Act::PageNext: if (n) { sel_ = (sel_ / page + 1) * page; if (sel_ >= n) sel_ = 0; ShowCandidates(ec, ctx); } break;
        case Act::PagePrev:
            if (n) {
                int p = sel_ / page - 1;
                if (p < 0) p = (n - 1) / page;
                sel_ = p * page;
                ShowCandidates(ec, ctx);
            }
            break;
        case Act::Pick: DoPick(ec, ctx, a.index); break;
        case Act::CommitEat:
        case Act::CommitPass: CommitAll(ec, ctx); lastChar_.clear(); break;
    }
}

void TextService::DoPick(TfEditCookie ec, ITfContext* ctx, int index) {
    if (index < 0 || index >= (int)cands_.size()) return;
    if (hg_.IsFullSyllable()) run_ += hg_.Flush();
    Candidate c = cands_[index];
    if (c.len <= 0 || c.len > (int)run_.size()) return;
    picks_.push_back({c.read, c.text});
    run_.erase(0, c.len);
    Engine& e = SharedEngine();
    {
        std::lock_guard<std::mutex> lock(g_engineLock);
        SaveUserLine(e.Learn(c.read, c.text));
    }
    auto chars = Engine::Chars(c.text);
    if (!chars.empty()) lastChar_ = chars.back();

    std::wstring commit;
    if (run_.empty() && hg_.Empty()) {
        commit = PickedText(true);
        if (picks_.size() >= 2) {   // 이어서 고른 것은 한 단어로 기억 (8자까지)
            std::wstring R, Z;
            for (auto& p : picks_) { R += p.first; Z += p.second; }
            size_t zc = Engine::Chars(Z).size();
            if (zc >= 2 && zc <= 8) {
                std::lock_guard<std::mutex> lock(g_engineLock);
                SaveUserLine(e.AddWord(R, Z));
            }
        }
        picks_.clear();
    }
    Refresh();
    Update(ec, ctx, commit);
}

void TextService::CommitAll(TfEditCookie ec, ITfContext* ctx) {
    std::wstring commit = PickedText(true) + run_ + hg_.Current();
    hg_.Reset();
    run_.clear();
    picks_.clear();
    cands_.clear();
    sel_ = 0;
    Update(ec, ctx, commit);
}

std::wstring TextService::PickedText(bool formatted) const {
    std::wstring R, Z;
    for (auto& p : picks_) { R += p.first; Z += p.second; }
    if (Z.empty()) return L"";
    return formatted ? Engine::Format(R, Z, Settings::Format()) : Z;
}

void TextService::Refresh() {
    cands_.clear();
    sel_ = 0;
    if (mode_ != Mode::Hanmun) return;
    std::wstring r = run_ + (hg_.IsFullSyllable() ? hg_.Current() : L"");
    if (r.empty()) return;
    std::wstring prev = lastChar_;
    if (!picks_.empty()) {
        auto ch = Engine::Chars(picks_.back().second);
        if (!ch.empty()) prev = ch.back();
    }
    Engine& e = SharedEngine();
    std::lock_guard<std::mutex> lock(g_engineLock);
    ReloadUserIfChanged();
    cands_ = e.WithPredictions(e.Candidates(r, false, prev), r, Settings::Predict());
}

// ─────────────────────────────────────────── 조합 문자열 갱신

void TextService::Update(TfEditCookie ec, ITfContext* ctx, const std::wstring& commit) {
    const std::wstring compText = PickedText(false) + run_ + hg_.Current();
    if (!comp_ && commit.empty() && compText.empty()) { if (cand_) cand_->Hide(); return; }

    if (!comp_) {
        ITfInsertAtSelection* ias = nullptr;
        ITfContextComposition* cc = nullptr;
        ITfRange* range = nullptr;
        if (SUCCEEDED(ctx->QueryInterface(IID_ITfInsertAtSelection, (void**)&ias))) {
            ias->InsertTextAtSelection(ec, TF_IAS_QUERYONLY, nullptr, 0, &range);
            ias->Release();
        }
        if (range && SUCCEEDED(ctx->QueryInterface(IID_ITfContextComposition, (void**)&cc))) {
            cc->StartComposition(ec, range, static_cast<ITfCompositionSink*>(this), &comp_);
            cc->Release();
        }
        SafeRelease(range);
        if (!comp_) {   // 조합을 못 여는 곳: 확정 글자만 넣는다
            if (!commit.empty() && SUCCEEDED(ctx->QueryInterface(IID_ITfInsertAtSelection, (void**)&ias))) {
                ias->InsertTextAtSelection(ec, 0, commit.c_str(), (LONG)commit.size(), &range);
                ias->Release();
                SafeRelease(range);
            }
            ResetState();
            return;
        }
        compCtx_ = ctx;
        compCtx_->AddRef();
    }

    ITfRange* r = nullptr;
    if (FAILED(comp_->GetRange(&r))) return;
    const std::wstring all = commit + compText;
    r->SetText(ec, 0, all.c_str(), (LONG)all.size());

    ITfProperty* prop = nullptr;
    ctx->GetProperty(GUID_PROP_ATTRIBUTE, &prop);

    if (compText.empty()) {   // 다 확정 → 조합 끝
        if (prop) prop->Clear(ec, r);
        SetSelectionEnd(ec, ctx, r);
        comp_->EndComposition(ec);
        SafeRelease(r);
        SafeRelease(prop);
        SafeRelease(comp_);
        SafeRelease(compCtx_);
        if (cand_) cand_->Hide();
        return;
    }
    if (!commit.empty()) {   // 앞부분만 확정: 조합 시작점을 뒤로 민다
        ITfRange* nr = nullptr;
        if (SUCCEEDED(r->Clone(&nr))) {
            LONG moved = 0;
            nr->ShiftStart(ec, (LONG)commit.size(), &moved, nullptr);
            if (prop) {
                ITfRange* done = nullptr;
                if (SUCCEEDED(r->Clone(&done))) {
                    done->ShiftEndToRange(ec, nr, TF_ANCHOR_START);
                    prop->Clear(ec, done);
                    done->Release();
                }
            }
            comp_->ShiftStart(ec, nr);
            nr->Release();
        }
        SafeRelease(r);
        comp_->GetRange(&r);
    }
    if (r) {
        if (prop && attrAtom_ != TF_INVALID_GUIDATOM) {
            VARIANT v;
            v.vt = VT_I4;
            v.lVal = (LONG)attrAtom_;
            prop->SetValue(ec, r, &v);
        }
        // 한국어 입력기 방식: 조합 문자열의 마지막 글자를 블록(interim)으로 잡는다.
        // 한문 모드의 여러 글자 조합도 이렇게 해야 카카오톡 같은 옛 방식 앱에서 자모가 갈라지지 않는다.
        ITfRange* last = nullptr;
        if (SUCCEEDED(r->Clone(&last))) {
            LONG lastLen = (compText.size() >= 2 && compText.back() >= 0xDC00 && compText.back() <= 0xDFFF) ? 2 : 1;
            LONG moved = 0;
            last->Collapse(ec, TF_ANCHOR_END);
            last->ShiftStart(ec, -lastLen, &moved, nullptr);
            SetSelectionEnd(ec, ctx, last, true);
            last->Release();
        } else SetSelectionEnd(ec, ctx, r);
        r->Release();
    }
    SafeRelease(prop);
    ShowCandidates(ec, ctx);
}

void TextService::ShowCandidates(TfEditCookie ec, ITfContext* ctx) {
    if (!cand_) return;
    if (cands_.empty() || !comp_) { cand_->Hide(); return; }
    RECT rc = {};
    bool ok = false;
    ITfContextView* view = nullptr;
    ITfRange* r = nullptr;
    if (SUCCEEDED(ctx->GetActiveView(&view)) && SUCCEEDED(comp_->GetRange(&r))) {
        BOOL clipped = FALSE;
        ok = view->GetTextExt(ec, r, &rc, &clipped) == S_OK && (rc.bottom > rc.top || rc.right > rc.left);
    }
    SafeRelease(r);
    SafeRelease(view);
    if (!ok) {   // 위치를 못 알려 주는 앱: 캐럿 위치로
        GUITHREADINFO gi = {sizeof(gi)};
        if (GetGUIThreadInfo(GetCurrentThreadId(), &gi) && gi.hwndCaret) {
            rc = gi.rcCaret;
            MapWindowPoints(gi.hwndCaret, nullptr, (POINT*)&rc, 2);
        } else {
            POINT pt;
            GetCursorPos(&pt);
            rc = {pt.x, pt.y, pt.x, pt.y + 20};
        }
    }
    cand_->Show(rc, cands_, sel_);
}

void TextService::OnCandidateClicked(int index) {
    if (!compCtx_) return;
    RunEdit(compCtx_, TF_ES_ASYNCDONTCARE | TF_ES_READWRITE, [this, index](TfEditCookie ec) {
        if (compCtx_) DoPick(ec, compCtx_, index);
    });
}

// ─────────────────────────────────────────── 오른쪽 클릭 메뉴

void TextService::ShowMenu(POINT pt) {
    HMENU m = CreatePopupMenu();
    auto add = [&](UINT id, const wchar_t* text, bool checked, bool radio = true) {
        MENUITEMINFOW mi = {sizeof(mi)};
        mi.fMask = MIIM_ID | MIIM_STRING | MIIM_STATE | MIIM_FTYPE;
        mi.fType = MFT_STRING | (radio ? MFT_RADIOCHECK : 0);
        mi.fState = checked ? MFS_CHECKED : 0;
        mi.wID = id;
        mi.dwTypeData = const_cast<wchar_t*>(text);
        InsertMenuItemW(m, GetMenuItemCount(m), TRUE, &mi);
    };
    auto sep = [&] { AppendMenuW(m, MF_SEPARATOR, 0, nullptr); };
    add(1, L"영문 (A)", mode_ == Mode::English);
    add(2, L"한글 (가)", mode_ == Mode::Hangul);
    add(3, L"한문 (漢) — 한자 후보 자동", mode_ == Mode::Hanmun);
    sep();
    int f = Settings::Format();
    add(10, L"표기: 漢字", f == 0);
    add(11, L"표기: 漢字(한자)", f == 1);
    add(12, L"표기: 한자(漢字)", f == 2);
    sep();
    add(20, L"어구 예측", Settings::Predict(), false);
    sep();
    add(30, L"내 단어 초기화…", false, false);
    add(40, L"빠른 한문 입력기 정보", false, false);

    HWND owner = cand_ ? cand_->Hwnd() : nullptr;
    UINT cmd = TrackPopupMenuEx(m, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, owner, nullptr);
    DestroyMenu(m);
    switch (cmd) {
        case 1: SetModeFromUser(Mode::English); break;
        case 2: SetModeFromUser(Mode::Hangul); break;
        case 3: SetModeFromUser(Mode::Hanmun); break;
        case 10: case 11: case 12: Settings::SetFormat((int)cmd - 10); break;
        case 20: Settings::SetPredict(!Settings::Predict()); break;
        case 30:
            if (MessageBoxW(nullptr, L"고른 횟수와 내 단어를 모두 지울까요?\n되돌릴 수 없습니다.", L"빠른 한문 입력기",
                            MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDYES) {
                std::wstring dir = UserFilePath();
                if (!dir.empty()) DeleteFileW((dir + L"\\user.tsv").c_str());
                std::lock_guard<std::mutex> lock(g_engineLock);
                SharedEngine().ClearUser();
                g_userStamp = {};
            }
            break;
        case 40:
            MessageBoxW(nullptr,
                        L"빠른 한문 입력기 v" HANMUN_VERSION L"\n제작자: 세 (@segrc_)\n\n"
                        L"한자 키나 오른쪽 Ctrl(혼자 눌렀다 떼기), 작업 표시줄 클릭으로\n한글(가) ↔ 한문(漢)을 바꿉니다.\n"
                        L"독음을 치면 후보가 뜨고, Space로 이동, Enter·숫자로 고릅니다.\n"
                        L"Esc는 한글 그대로 확정합니다.\n\n"
                        L"웹 버전: https://hanmun.j-segrc.workers.dev",
                        L"빠른 한문 입력기", MB_OK | MB_ICONINFORMATION);
            break;
    }
}

// ─────────────────────────────────────────── 밑줄 표시 속성

STDMETHODIMP TextService::EnumDisplayAttributeInfo(IEnumTfDisplayAttributeInfo** ppEnum) {
    if (!ppEnum) return E_INVALIDARG;
    *ppEnum = new DisplayAttributeEnum();
    return S_OK;
}

STDMETHODIMP TextService::GetDisplayAttributeInfo(REFGUID guid, ITfDisplayAttributeInfo** ppInfo) {
    if (!ppInfo) return E_INVALIDARG;
    *ppInfo = nullptr;
    if (!IsEqualGUID(guid, GUID_HanmunDisplayAttr)) return E_INVALIDARG;
    *ppInfo = new DisplayAttributeInfo();
    return S_OK;
}
