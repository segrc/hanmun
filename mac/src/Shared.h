// 빠른 한문 입력기 (macOS) — 공용
// Copyright (c) 2026 세 (@segrc_) — MIT License
#pragma once
#import <Cocoa/Cocoa.h>

#include <string>

#include "Engine.h"

#define kModeHangul @"com.segrc.inputmethod.Hanmun.hangul"
#define kModeHanmun @"com.segrc.inputmethod.Hanmun.hanmun"

NSString* NSFromW(const std::wstring& w);
std::wstring WFromNS(NSString* s);

// 엔진은 입력기 프로세스 하나에서 모든 앱이 함께 쓴다
Engine& SharedEngine();
void SaveUserLine(const std::wstring& line);
void ClearUserData();

// 설정 (NSUserDefaults)
int FormatSetting();          // 0 漢字  1 漢字(한자)  2 한자(漢字)
void SetFormatSetting(int f);
BOOL PredictSetting();
void SetPredictSetting(BOOL on);
