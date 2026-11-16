// 한자 후보 엔진 — 웹 버전 engine.js 를 그대로 옮긴 것 (한문 모드 기준)
// Copyright (c) 2026 세 (@segrc_) — MIT License
#pragma once
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct Candidate {
    std::wstring read;    // 이 후보의 독음 (예측이면 전체 독음)
    std::wstring text;    // 한자
    std::wstring mean;    // 훈음 (한 글자일 때)
    int len = 0;          // 친 독음 중 이 후보가 차지하는 글자 수
    bool pred = false;    // 예측 후보
    bool learned = false; // 내가 고른 적 있음
    bool ctx = false;     // 앞 글자 문맥으로 올라옴
};

class Engine {
public:
    // hanmun.dict (UTF-16으로 바꾼 전체 내용)
    void Load(const std::wstring& dict);
    bool Loaded() const { return loaded_; }

    std::vector<Candidate> Candidates(const std::wstring& run, bool exact, const std::wstring& prev);
    std::vector<Candidate> WithPredictions(std::vector<Candidate> list, const std::wstring& run, bool predict);

    // 사용자 학습 — 줄 형식 "f\t읽기\t한자" (고른 횟수 +1), "w\t읽기\t한자" (내 단어)
    void LoadUser(const std::wstring& text);
    void ClearUser();
    std::wstring Learn(const std::wstring& r, const std::wstring& z);    // 저장할 줄을 돌려준다
    std::wstring AddWord(const std::wstring& r, const std::wstring& z);  // 이미 있으면 빈 문자열

    static std::wstring Format(const std::wstring& r, const std::wstring& z, int mode);
    static std::vector<std::wstring> Chars(const std::wstring& s);   // 코드포인트 단위로 자르기

private:
    struct Source {
        std::unordered_map<std::wstring, std::vector<std::wstring>> map;
        std::vector<std::wstring> keys;                 // 가나다순
        std::unordered_map<std::wstring, int> rank;     // 원래 순서 = 빈도 순
        bool dirty = true;
        void Add(const std::wstring& k, std::vector<std::wstring> v, bool front = false);
        const std::vector<std::wstring>* Get(const std::wstring& k) const;
        void Index();
    };
    int Score(const std::wstring& r, const std::wstring& z) const;
    std::vector<std::wstring> ClassicalChars(const std::wstring& r, const std::wstring& prev) const;
    std::vector<Candidate> Predict(const std::wstring& run, size_t max);

    bool loaded_ = false;
    std::unordered_map<std::wstring, std::vector<std::wstring>> C_, P_, F_;
    std::unordered_map<std::wstring, int> U_;
    std::unordered_map<std::wstring, std::wstring> M_;
    Source W_, CW_, KW_, GW_, UW_;
    std::unordered_set<std::wstring> GSET_;
    std::unordered_map<std::wstring, int> userF_;
};
