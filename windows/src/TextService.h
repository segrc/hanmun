// 입력기 본체 (TSF 텍스트 서비스)
// Copyright (c) 2026 세 (@segrc_) — MIT License
#pragma once
#include <functional>
#include <vector>

#include "Engine.h"
#include "Globals.h"
#include "Hangul.h"

class CandidateWindow;
class LangBarButton;

class TextService : public ITfTextInputProcessorEx,
                    public ITfThreadMgrEventSink,
                    public ITfKeyEventSink,
                    public ITfCompositionSink,
                    public ITfCompartmentEventSink,
                    public ITfDisplayAttributeProvider {
public:
    TextService();
    ~TextService();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ITfTextInputProcessor(Ex)
    STDMETHODIMP Activate(ITfThreadMgr* tm, TfClientId tid) override;
    STDMETHODIMP Deactivate() override;
    STDMETHODIMP ActivateEx(ITfThreadMgr* tm, TfClientId tid, DWORD flags) override;

    // ITfThreadMgrEventSink
    STDMETHODIMP OnInitDocumentMgr(ITfDocumentMgr*) override { return S_OK; }
    STDMETHODIMP OnUninitDocumentMgr(ITfDocumentMgr*) override { return S_OK; }
    STDMETHODIMP OnSetFocus(ITfDocumentMgr* focus, ITfDocumentMgr* prev) override;
    STDMETHODIMP OnPushContext(ITfContext*) override { return S_OK; }
    STDMETHODIMP OnPopContext(ITfContext*) override { return S_OK; }

    // ITfKeyEventSink
    STDMETHODIMP OnSetFocus(BOOL) override { return S_OK; }
    STDMETHODIMP OnTestKeyDown(ITfContext* ctx, WPARAM w, LPARAM l, BOOL* eaten) override;
    STDMETHODIMP OnKeyDown(ITfContext* ctx, WPARAM w, LPARAM l, BOOL* eaten) override;
    STDMETHODIMP OnTestKeyUp(ITfContext*, WPARAM w, LPARAM, BOOL* eaten) override;
    STDMETHODIMP OnKeyUp(ITfContext*, WPARAM w, LPARAM, BOOL* eaten) override;
    STDMETHODIMP OnPreservedKey(ITfContext*, REFGUID, BOOL* eaten) override { *eaten = FALSE; return S_OK; }

    // ITfCompositionSink
    STDMETHODIMP OnCompositionTerminated(TfEditCookie ec, ITfComposition* comp) override;

    // ITfCompartmentEventSink
    STDMETHODIMP OnChange(REFGUID guid) override;

    // ITfDisplayAttributeProvider
    STDMETHODIMP EnumDisplayAttributeInfo(IEnumTfDisplayAttributeInfo** ppEnum) override;
    STDMETHODIMP GetDisplayAttributeInfo(REFGUID guid, ITfDisplayAttributeInfo** ppInfo) override;

    // 언어 표시줄 버튼·후보 창에서 부른다
    Mode GetMode() const { return mode_; }
    void SetModeFromUser(Mode m);
    void OnCandidateClicked(int index);
    void ShowMenu(POINT pt);

private:
    enum class Act { None, ToggleHangul, ToggleHanja, Jamo, Back, Next, Prev, PageNext, PagePrev, Pick, CommitEat, CommitPass };
    struct Action { Act act = Act::None; wchar_t jamo = 0; int index = 0; };

    Action Classify(WPARAM vk) const;
    bool Composing() const { return !hg_.Empty() || !run_.empty() || !picks_.empty(); }
    void Execute(TfEditCookie ec, ITfContext* ctx, const Action& a);
    void DoPick(TfEditCookie ec, ITfContext* ctx, int index);
    void CommitAll(TfEditCookie ec, ITfContext* ctx);
    void Refresh();
    std::wstring PickedText(bool formatted) const;
    void Update(TfEditCookie ec, ITfContext* ctx, const std::wstring& commit);
    void ShowCandidates(TfEditCookie ec, ITfContext* ctx);
    HRESULT RunEdit(ITfContext* ctx, DWORD flags, std::function<void(TfEditCookie)> fn);
    void SetOpen(bool open);
    bool IsOpen();
    void ApplyMode(Mode m);
    void SyncMode();
    void ResetState();
    void SaveUserLine(const std::wstring& line);
    void ReloadUserIfChanged();

    LONG ref_ = 1;
    ITfThreadMgr* tm_ = nullptr;
    TfClientId tid_ = TF_CLIENTID_NULL;
    DWORD tmCookie_ = TF_INVALID_COOKIE, compCookie_ = TF_INVALID_COOKIE;
    ITfComposition* comp_ = nullptr;
    ITfContext* compCtx_ = nullptr;
    TfGuidAtom attrAtom_ = TF_INVALID_GUIDATOM;
    CandidateWindow* cand_ = nullptr;
    LangBarButton* button_ = nullptr;

    Mode mode_ = Mode::Hangul;
    Hangul hg_;
    std::wstring run_;                                         // 아직 바꾸지 않은 한글
    std::vector<std::pair<std::wstring, std::wstring>> picks_; // 이번 조합에서 고른 (독음, 한자)
    std::wstring lastChar_;                                    // 바로 앞에 입력한 한자 (문맥)
    std::vector<Candidate> cands_;
    int sel_ = 0;
    bool ignoreOpenChange_ = false;
    bool rctrlAlone_ = false;
};

// 엔진은 프로세스마다 한 번만 불러 쓴다
Engine& SharedEngine();
