// DLL 진입점, COM 클래스 팩토리, 등록, 설정
// Copyright (c) 2026 세 (@segrc_) — MIT License
#include <initguid.h>

#include "Globals.h"
#include "TextService.h"

HINSTANCE g_hInst = nullptr;
LONG g_cRefDll = 0;

// {2F3C03EE-41F3-47DB-A00C-ACDACFFB961A}
const CLSID CLSID_Hanmun = {0x2f3c03ee, 0x41f3, 0x47db, {0xa0, 0x0c, 0xac, 0xda, 0xcf, 0xfb, 0x96, 0x1a}};
// {4025D9CF-B997-4984-87E3-776C603C6B92}
const GUID GUID_HanmunProfile = {0x4025d9cf, 0xb997, 0x4984, {0x87, 0xe3, 0x77, 0x6c, 0x60, 0x3c, 0x6b, 0x92}};
// {64846B4B-39F8-467C-A45C-174CCE725FA5}
const GUID GUID_HanmunDisplayAttr = {0x64846b4b, 0x39f8, 0x467c, {0xa4, 0x5c, 0x17, 0x4c, 0xce, 0x72, 0x5f, 0xa5}};

namespace {
const wchar_t kName[] = L"빠른 한문 입력기";
const wchar_t kClsidKey[] = L"CLSID\\{2F3C03EE-41F3-47DB-A00C-ACDACFFB961A}";
const LANGID kLang = MAKELANGID(LANG_KOREAN, SUBLANG_KOREAN);

const GUID* const kCategories[] = {
    &GUID_TFCAT_TIP_KEYBOARD,
    &GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER,
    &GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT,
    &GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT,
};

class ClassFactory : public IClassFactory {
public:
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) return E_INVALIDARG;
        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory)) {
            *ppv = this;
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { DllAddRef(); return 2; }
    STDMETHODIMP_(ULONG) Release() override { DllRelease(); return 1; }
    STDMETHODIMP CreateInstance(IUnknown* outer, REFIID riid, void** ppv) override {
        if (!ppv) return E_INVALIDARG;
        *ppv = nullptr;
        if (outer) return CLASS_E_NOAGGREGATION;
        auto* ts = new (std::nothrow) TextService();
        if (!ts) return E_OUTOFMEMORY;
        HRESULT hr = ts->QueryInterface(riid, ppv);
        ts->Release();
        return hr;
    }
    STDMETHODIMP LockServer(BOOL lock) override {
        if (lock) DllAddRef(); else DllRelease();
        return S_OK;
    }
};
ClassFactory g_factory;

std::wstring ModulePath() {
    wchar_t buf[MAX_PATH];
    DWORD n = GetModuleFileNameW(g_hInst, buf, MAX_PATH);
    return std::wstring(buf, n);
}

bool SetRegString(HKEY root, const std::wstring& key, const wchar_t* name, const std::wstring& value) {
    HKEY h;
    if (RegCreateKeyExW(root, key.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &h, nullptr) != ERROR_SUCCESS) return false;
    LONG r = RegSetValueExW(h, name, 0, REG_SZ, (const BYTE*)value.c_str(), DWORD((value.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(h);
    return r == ERROR_SUCCESS;
}
}  // namespace

// ─────────────────────────────────────────── 설정

namespace {
const wchar_t kSettingsKey[] = L"Software\\Hanmun";
DWORD ReadDword(const wchar_t* name, DWORD def) {
    DWORD v = def, size = sizeof(v);
    if (RegGetValueW(HKEY_CURRENT_USER, kSettingsKey, name, RRF_RT_REG_DWORD, nullptr, &v, &size) != ERROR_SUCCESS) return def;
    return v;
}
void WriteDword(const wchar_t* name, DWORD v) {
    HKEY h;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kSettingsKey, 0, nullptr, 0, KEY_WRITE, nullptr, &h, nullptr) != ERROR_SUCCESS) return;
    RegSetValueExW(h, name, 0, REG_DWORD, (const BYTE*)&v, sizeof(v));
    RegCloseKey(h);
}
}  // namespace

int Settings::Format() { DWORD f = ReadDword(L"Format", 0); return f <= 2 ? (int)f : 0; }
void Settings::SetFormat(int f) { WriteDword(L"Format", (DWORD)f); }
bool Settings::Predict() { return ReadDword(L"Predict", 1) != 0; }
void Settings::SetPredict(bool on) { WriteDword(L"Predict", on ? 1 : 0); }
Mode Settings::KoreanMode() { return ReadDword(L"KoreanMode", 1) == 2 ? Mode::Hanmun : Mode::Hangul; }
void Settings::SetKoreanMode(Mode m) { WriteDword(L"KoreanMode", m == Mode::Hanmun ? 2 : 1); }

// ─────────────────────────────────────────── DLL 내보내기

BOOL WINAPI DllMain(HINSTANCE h, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_hInst = h;
        DisableThreadLibraryCalls(h);
    }
    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void** ppv) {
    if (!ppv) return E_INVALIDARG;
    *ppv = nullptr;
    if (!IsEqualCLSID(clsid, CLSID_Hanmun)) return CLASS_E_CLASSNOTAVAILABLE;
    return g_factory.QueryInterface(riid, ppv);
}

STDAPI DllCanUnloadNow() { return g_cRefDll <= 0 ? S_OK : S_FALSE; }

STDAPI DllUnregisterServer();

STDAPI DllRegisterServer() {
    HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    HRESULT hr = S_OK;
    const std::wstring path = ModulePath();

    // COM 서버
    if (!SetRegString(HKEY_CLASSES_ROOT, kClsidKey, nullptr, kName) ||
        !SetRegString(HKEY_CLASSES_ROOT, std::wstring(kClsidKey) + L"\\InprocServer32", nullptr, path) ||
        !SetRegString(HKEY_CLASSES_ROOT, std::wstring(kClsidKey) + L"\\InprocServer32", L"ThreadingModel", L"Apartment"))
        hr = SELFREG_E_CLASS;

    // 한국어 입력 프로필
    if (SUCCEEDED(hr)) {
        ITfInputProcessorProfileMgr* pm = nullptr;
        hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER, IID_ITfInputProcessorProfileMgr,
                              (void**)&pm);
        if (SUCCEEDED(hr)) {
            hr = pm->RegisterProfile(CLSID_Hanmun, kLang, GUID_HanmunProfile, kName, (ULONG)wcslen(kName), path.c_str(),
                                     (ULONG)path.size(), (UINT)-IDI_HANMUN, nullptr, 0, TRUE, 0);
            pm->Release();
        }
    }
    // 분류(키보드 입력기, 표시 속성, 스토어 앱·작업 표시줄 지원)
    if (SUCCEEDED(hr)) {
        ITfCategoryMgr* cm = nullptr;
        hr = CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr, (void**)&cm);
        if (SUCCEEDED(hr)) {
            for (auto* cat : kCategories)
                if (FAILED(hr = cm->RegisterCategory(CLSID_Hanmun, *cat, CLSID_Hanmun))) break;
            cm->Release();
        }
    }
    if (FAILED(hr)) DllUnregisterServer();
    if (SUCCEEDED(init)) CoUninitialize();
    return FAILED(hr) ? SELFREG_E_CLASS : S_OK;
}

STDAPI DllUnregisterServer() {
    HRESULT init = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    ITfCategoryMgr* cm = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER, IID_ITfCategoryMgr, (void**)&cm))) {
        for (auto* cat : kCategories) cm->UnregisterCategory(CLSID_Hanmun, *cat, CLSID_Hanmun);
        cm->Release();
    }
    ITfInputProcessorProfileMgr* pm = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_ITfInputProcessorProfileMgr, (void**)&pm))) {
        pm->UnregisterProfile(CLSID_Hanmun, kLang, GUID_HanmunProfile, 0);
        pm->Release();
    }
    RegDeleteTreeW(HKEY_CLASSES_ROOT, kClsidKey);
    if (SUCCEEDED(init)) CoUninitialize();
    return S_OK;
}
