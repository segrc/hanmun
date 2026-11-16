// 빠른 한문 입력기 — 공용 선언
// Copyright (c) 2026 세 (@segrc_) — MIT License
#pragma once
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <ole2.h>
#include <olectl.h>
#include <msctf.h>
#include <string>

#define HANMUN_VERSION L"0.1.5"

extern HINSTANCE g_hInst;
extern LONG g_cRefDll;

extern const CLSID CLSID_Hanmun;        // 입력기(텍스트 서비스) 클래스
extern const GUID GUID_HanmunProfile;   // 한국어 입력 프로필
extern const GUID GUID_HanmunDisplayAttr; // 조합 중 글자 밑줄
extern const GUID GUID_InputModeItem;     // = GUID_LBI_INPUTMODE (작업 표시줄 입력 표시기)

inline void DllAddRef() { InterlockedIncrement(&g_cRefDll); }
inline void DllRelease() { InterlockedDecrement(&g_cRefDll); }

template <class T> inline void SafeRelease(T*& p) { if (p) { p->Release(); p = nullptr; } }

// 입력 상태
enum class Mode { English = 0, Hangul = 1, Hanmun = 2 };

// 사용자 설정 (HKCU\Software\Hanmun)
namespace Settings {
    int Format();            // 0: 漢字  1: 漢字(한자)  2: 한자(漢字)
    void SetFormat(int f);
    bool Predict();          // 어구 예측
    void SetPredict(bool on);
    Mode KoreanMode();       // 한글 상태로 돌아올 때 쓸 모드(한글/한문)
    void SetKoreanMode(Mode m);
}

#define IDI_HANMUN 101
#define IDR_DICT   201
