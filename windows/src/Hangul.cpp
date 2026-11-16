// 두벌식 한글 오토마타
// Copyright (c) 2026 세 (@segrc_) — MIT License
#include "Hangul.h"

namespace {
const wchar_t CHO[] = L"ㄱㄲㄴㄷㄸㄹㅁㅂㅃㅅㅆㅇㅈㅉㅊㅋㅌㅍㅎ";
const wchar_t JUNG[] = L"ㅏㅐㅑㅒㅓㅔㅕㅖㅗㅘㅙㅚㅛㅜㅝㅞㅟㅠㅡㅢㅣ";
const wchar_t JONG[] = L" ㄱㄲㄳㄴㄵㄶㄷㄹㄺㄻㄼㄽㄾㄿㅀㅁㅂㅄㅅㅆㅇㅈㅊㅋㅌㅍㅎ";

// 겹받침·겹모음: {앞, 뒤, 합친 것}
const wchar_t JONG2[][3] = {
    {L'ㄱ', L'ㅅ', L'ㄳ'}, {L'ㄴ', L'ㅈ', L'ㄵ'}, {L'ㄴ', L'ㅎ', L'ㄶ'}, {L'ㄹ', L'ㄱ', L'ㄺ'},
    {L'ㄹ', L'ㅁ', L'ㄻ'}, {L'ㄹ', L'ㅂ', L'ㄼ'}, {L'ㄹ', L'ㅅ', L'ㄽ'}, {L'ㄹ', L'ㅌ', L'ㄾ'},
    {L'ㄹ', L'ㅍ', L'ㄿ'}, {L'ㄹ', L'ㅎ', L'ㅀ'}, {L'ㅂ', L'ㅅ', L'ㅄ'}};
const wchar_t JUNG2[][3] = {
    {L'ㅗ', L'ㅏ', L'ㅘ'}, {L'ㅗ', L'ㅐ', L'ㅙ'}, {L'ㅗ', L'ㅣ', L'ㅚ'}, {L'ㅜ', L'ㅓ', L'ㅝ'},
    {L'ㅜ', L'ㅔ', L'ㅞ'}, {L'ㅜ', L'ㅣ', L'ㅟ'}, {L'ㅡ', L'ㅣ', L'ㅢ'}};

int IndexOf(const wchar_t* s, wchar_t c) {
    for (int i = 0; s[i]; i++) if (s[i] == c) return i;
    return -1;
}
bool IsVowel(wchar_t c) { return c >= 0x314F && c <= 0x3163; }

template <size_t N>
wchar_t Combine(const wchar_t (&t)[N][3], wchar_t a, wchar_t b) {
    for (auto& e : t) if (e[0] == a && e[1] == b) return e[2];
    return 0;
}
template <size_t N>
bool Split(const wchar_t (&t)[N][3], wchar_t c, wchar_t* a, wchar_t* b) {
    for (auto& e : t) if (e[2] == c) { *a = e[0]; *b = e[1]; return true; }
    return false;
}
}  // namespace

wchar_t Hangul::KeyToJamo(unsigned vk, bool shift) {
    // 두벌식 표준 배열 (QWERTY 자리)
    static const wchar_t NORMAL[] = L"ㅁㅠㅊㅇㄷㄹㅎㅗㅑㅓㅏㅣㅡㅜㅐㅔㅂㄱㄴㅅㅕㅍㅈㅌㅛㅋ";
    if (vk < 'A' || vk > 'Z') return 0;
    wchar_t j = NORMAL[vk - 'A'];
    if (shift) {
        switch (vk) {
            case 'Q': return L'ㅃ'; case 'W': return L'ㅉ'; case 'E': return L'ㄸ';
            case 'R': return L'ㄲ'; case 'T': return L'ㅆ'; case 'O': return L'ㅒ';
            case 'P': return L'ㅖ';
        }
    }
    return j;
}

std::wstring Hangul::Current() const {
    if (cho_ && jung_) {
        int c = IndexOf(CHO, cho_), v = IndexOf(JUNG, jung_), t = jong_ ? IndexOf(JONG, jong_) : 0;
        return std::wstring(1, wchar_t(0xAC00 + (c * 21 + v) * 28 + t));
    }
    if (cho_) return std::wstring(1, cho_);
    if (jung_) return std::wstring(1, jung_);
    return L"";
}

std::wstring Hangul::Feed(wchar_t j) {
    std::wstring out;
    auto restart = [&](wchar_t c, wchar_t v) {
        out = Current(); Reset();
        cho_ = c; jung_ = v;
        if (c) keys_.push_back(c);
        if (v) keys_.push_back(v);
    };
    if (!IsVowel(j)) {
        if (!cho_ && !jung_) { cho_ = j; keys_.push_back(j); }
        else if (!jung_ || !cho_) restart(j, 0);               // 자음만 / 모음만 있던 글자
        else if (!jong_) {
            if (IndexOf(JONG, j) > 0) { jong_ = j; keys_.push_back(j); }
            else restart(j, 0);                                // ㄸ ㅃ ㅉ 은 받침이 될 수 없음
        } else if (wchar_t d = Combine(JONG2, jong_, j)) { jong_ = d; keys_.push_back(j); }
        else restart(j, 0);
    } else {
        if (!jung_) { jung_ = j; keys_.push_back(j); }
        else if (!jong_) {
            if (wchar_t d = Combine(JUNG2, jung_, j)) { jung_ = d; keys_.push_back(j); }   // ㅗ+ㅏ → ㅘ
            else restart(0, j);
        } else {
            // 받침이 다음 글자 초성으로 넘어간다 (닭+ㅏ → 달가)
            wchar_t keep = 0, move = jong_;
            if (Split(JONG2, jong_, &keep, &move)) jong_ = keep; else jong_ = 0;
            restart(move, j);
        }
    }
    return out;
}

bool Hangul::Backspace() {
    if (keys_.empty()) return false;
    std::vector<wchar_t> k(keys_.begin(), keys_.end() - 1);
    Reset();
    for (wchar_t j : k) Feed(j);
    return true;
}

void Hangul::Load(wchar_t s) {
    Reset();
    if (!IsSyllable(s)) return;
    int n = s - 0xAC00, c = n / 588, v = (n % 588) / 28, t = n % 28;
    std::vector<wchar_t> k;
    k.push_back(CHO[c]);
    wchar_t a, b;
    if (Split(JUNG2, JUNG[v], &a, &b)) { k.push_back(a); k.push_back(b); } else k.push_back(JUNG[v]);
    if (t) { if (Split(JONG2, JONG[t], &a, &b)) { k.push_back(a); k.push_back(b); } else k.push_back(JONG[t]); }
    for (wchar_t j : k) Feed(j);
}
