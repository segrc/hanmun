// 조합 중인 글자에 밑줄을 긋는 표시 속성
// Copyright (c) 2026 세 (@segrc_) — MIT License
#pragma once
#include "Globals.h"

class DisplayAttributeInfo : public ITfDisplayAttributeInfo {
public:
    DisplayAttributeInfo() { DllAddRef(); }
    ~DisplayAttributeInfo() { DllRelease(); }
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) return E_INVALIDARG;
        *ppv = nullptr;
        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfDisplayAttributeInfo)) *ppv = this;
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
    STDMETHODIMP GetGUID(GUID* g) override { if (!g) return E_INVALIDARG; *g = GUID_HanmunDisplayAttr; return S_OK; }
    STDMETHODIMP GetDescription(BSTR* b) override { if (!b) return E_INVALIDARG; *b = SysAllocString(L"빠른 한문 입력기 조합 중"); return *b ? S_OK : E_OUTOFMEMORY; }
    STDMETHODIMP GetAttributeInfo(TF_DISPLAYATTRIBUTE* a) override {
        if (!a) return E_INVALIDARG;
        *a = {};
        a->crText.type = TF_CT_NONE;
        a->crBk.type = TF_CT_NONE;
        a->lsStyle = TF_LS_SOLID;
        a->fBoldLine = FALSE;
        a->crLine.type = TF_CT_NONE;
        a->bAttr = TF_ATTR_INPUT;
        return S_OK;
    }
    STDMETHODIMP SetAttributeInfo(const TF_DISPLAYATTRIBUTE*) override { return E_NOTIMPL; }
    STDMETHODIMP Reset() override { return S_OK; }

private:
    LONG ref_ = 1;
};

class DisplayAttributeEnum : public IEnumTfDisplayAttributeInfo {
public:
    DisplayAttributeEnum() { DllAddRef(); }
    ~DisplayAttributeEnum() { DllRelease(); }
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) return E_INVALIDARG;
        *ppv = nullptr;
        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IEnumTfDisplayAttributeInfo)) *ppv = this;
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
    STDMETHODIMP Clone(IEnumTfDisplayAttributeInfo** pp) override {
        if (!pp) return E_INVALIDARG;
        auto* e = new DisplayAttributeEnum();
        e->pos_ = pos_;
        *pp = e;
        return S_OK;
    }
    STDMETHODIMP Next(ULONG n, ITfDisplayAttributeInfo** out, ULONG* fetched) override {
        ULONG got = 0;
        if (!out) return E_INVALIDARG;
        while (got < n && pos_ < 1) { out[got++] = new DisplayAttributeInfo(); pos_++; }
        if (fetched) *fetched = got;
        return got == n ? S_OK : S_FALSE;
    }
    STDMETHODIMP Reset() override { pos_ = 0; return S_OK; }
    STDMETHODIMP Skip(ULONG n) override { pos_ += n; return pos_ <= 1 ? S_OK : S_FALSE; }

private:
    LONG ref_ = 1;
    ULONG pos_ = 0;
};
