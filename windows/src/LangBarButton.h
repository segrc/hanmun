// 작업 표시줄 입력 표시기 버튼 (A / 가 / 漢)
// Copyright (c) 2026 세 (@segrc_) — MIT License
#pragma once
#include "Globals.h"

class TextService;

class LangBarButton : public ITfLangBarItemButton, public ITfSource {
public:
    explicit LangBarButton(TextService* owner);
    ~LangBarButton();
    void Detach() { owner_ = nullptr; }
    void Update();   // 모드가 바뀌면 아이콘을 다시 그리게 한다

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;
    // ITfLangBarItem
    STDMETHODIMP GetInfo(TF_LANGBARITEMINFO* info) override;
    STDMETHODIMP GetStatus(DWORD* status) override { *status = 0; return S_OK; }
    STDMETHODIMP Show(BOOL) override { return E_NOTIMPL; }
    STDMETHODIMP GetTooltipString(BSTR* tip) override;
    // ITfLangBarItemButton
    STDMETHODIMP OnClick(TfLBIClick click, POINT pt, const RECT* area) override;
    STDMETHODIMP InitMenu(ITfMenu*) override { return E_NOTIMPL; }
    STDMETHODIMP OnMenuSelect(UINT) override { return E_NOTIMPL; }
    STDMETHODIMP GetIcon(HICON* icon) override;
    STDMETHODIMP GetText(BSTR* text) override;
    // ITfSource
    STDMETHODIMP AdviseSink(REFIID riid, IUnknown* punk, DWORD* cookie) override;
    STDMETHODIMP UnadviseSink(DWORD cookie) override;

private:
    LONG ref_ = 1;
    TextService* owner_;
    ITfLangBarItemSink* sink_ = nullptr;
};
