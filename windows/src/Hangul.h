// 두벌식 한글 오토마타
// Copyright (c) 2026 세 (@segrc_) — MIT License
#pragma once
#include <string>
#include <vector>

class Hangul {
public:
    // 가상 키(A~Z)와 Shift 상태 → 호환 자모. 해당 없으면 0
    static wchar_t KeyToJamo(unsigned vk, bool shift);
    static bool IsSyllable(wchar_t c) { return c >= 0xAC00 && c <= 0xD7A3; }

    // 자모 하나를 넣는다. 앞 글자가 끝났으면 그 글자를 돌려준다.
    std::wstring Feed(wchar_t jamo);
    // 마지막 입력 자모를 지운다. 지울 게 없으면 false
    bool Backspace();
    // 완성 글자 하나를 다시 조합 상태로 풀어 넣는다(지우기로 앞 글자를 고칠 때)
    void Load(wchar_t syllable);
    std::wstring Current() const;
    std::wstring Flush() { std::wstring s = Current(); Reset(); return s; }
    bool Empty() const { return keys_.empty(); }
    bool IsFullSyllable() const { return cho_ && jung_; }
    void Reset() { cho_ = jung_ = jong_ = 0; keys_.clear(); }

private:
    wchar_t cho_ = 0, jung_ = 0, jong_ = 0;   // 호환 자모(0 = 없음)
    std::vector<wchar_t> keys_;               // 지금 글자를 만든 입력 순서
};
