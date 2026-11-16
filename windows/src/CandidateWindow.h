// 한자 후보 창 (세로 목록)
// Copyright (c) 2026 세 (@segrc_) — MIT License
#pragma once
#include <vector>

#include "Engine.h"
#include "Globals.h"

class TextService;

class CandidateWindow {
public:
    static const int kPage = 9;
    explicit CandidateWindow(TextService* owner);
    ~CandidateWindow();
    // anchor: 조합 글자의 화면 위치. 그 아래(자리가 없으면 위)에 띄운다
    void Show(const RECT& anchor, const std::vector<Candidate>& cands, int sel);
    void Hide();
    HWND Hwnd();

private:
    static LRESULT CALLBACK WndProc(HWND h, UINT msg, WPARAM w, LPARAM l);
    bool Create();
    void Paint(HDC dc);
    void MakeFonts(UINT dpi);
    SIZE Measure();

    TextService* owner_;
    HWND hwnd_ = nullptr;
    HFONT fMain_ = nullptr, fSmall_ = nullptr;
    UINT dpi_ = 0;
    std::vector<Candidate> items_;   // 지금 페이지
    int sel_ = 0, page_ = 0, pages_ = 1, base_ = 0;
    int rowH_ = 0, numW_ = 0, textW_ = 0, extraW_ = 0, pad_ = 0, gap_ = 0, footH_ = 0;
};
