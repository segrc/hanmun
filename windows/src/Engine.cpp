// 한자 후보 엔진 — 웹 버전 engine.js 를 그대로 옮긴 것 (한문 모드 기준)
// Copyright (c) 2026 세 (@segrc_) — MIT License
#include "Engine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cwchar>

namespace {
std::vector<std::wstring> SplitBy(const std::wstring& s, wchar_t sep) {
    std::vector<std::wstring> out;
    size_t a = 0;
    for (;;) {
        size_t b = s.find(sep, a);
        out.push_back(s.substr(a, b == std::wstring::npos ? std::wstring::npos : b - a));
        if (b == std::wstring::npos) break;
        a = b + 1;
    }
    return out;
}
bool StartsWith(const std::wstring& s, const std::wstring& p) {
    return s.size() >= p.size() && s.compare(0, p.size(), p) == 0;
}
}  // namespace

std::vector<std::wstring> Engine::Chars(const std::wstring& s) {
    std::vector<std::wstring> out;
    for (size_t i = 0; i < s.size(); i++) {
        // Windows의 wchar_t 는 UTF-16 이라 확장 한자는 두 칸(서로게이트)을 차지한다
        if (sizeof(wchar_t) == 2 && s[i] >= 0xD800 && s[i] <= 0xDBFF && i + 1 < s.size()) {
            out.push_back(s.substr(i, 2)); i++;
        } else out.push_back(s.substr(i, 1));
    }
    return out;
}

void Engine::Source::Add(const std::wstring& k, std::vector<std::wstring> v, bool front) {
    auto it = map.find(k);
    if (it == map.end()) {
        rank.emplace(k, (int)rank.size());
        map.emplace(k, std::move(v));
    } else {
        for (auto& z : v) {
            if (std::find(it->second.begin(), it->second.end(), z) != it->second.end()) continue;
            if (front) it->second.insert(it->second.begin(), z); else it->second.push_back(z);
        }
    }
    dirty = true;
}

const std::vector<std::wstring>* Engine::Source::Get(const std::wstring& k) const {
    auto it = map.find(k);
    return it == map.end() ? nullptr : &it->second;
}

void Engine::Source::Index() {
    if (!dirty) return;
    keys.clear();
    keys.reserve(map.size());
    for (auto& e : map) keys.push_back(e.first);
    std::sort(keys.begin(), keys.end());
    dirty = false;
}

void Engine::Load(const std::wstring& dict) {
    size_t a = 0;
    while (a < dict.size()) {
        size_t b = dict.find(L'\n', a);
        if (b == std::wstring::npos) b = dict.size();
        std::wstring line = dict.substr(a, b - a);
        a = b + 1;
        size_t t1 = line.find(L'\t'), t2 = t1 == std::wstring::npos ? t1 : line.find(L'\t', t1 + 1);
        if (t2 == std::wstring::npos) continue;
        std::wstring sec = line.substr(0, t1), k = line.substr(t1 + 1, t2 - t1 - 1), v = line.substr(t2 + 1);
        if (sec == L"c") C_[k] = Chars(v);
        else if (sec == L"p") P_[k] = Chars(v);
        else if (sec == L"f") F_[k] = Chars(v);
        else if (sec == L"u") U_[k] = (int)std::wcstol(v.c_str(), nullptr, 10);
        else if (sec == L"m") M_[k] = v;
        else {
            Source* s = sec == L"w" ? &W_ : sec == L"cw" ? &CW_ : sec == L"kw" ? &KW_ : sec == L"gw" ? &GW_ : nullptr;
            if (!s) continue;
            auto list = SplitBy(v, L'|');
            if (s == &GW_) for (auto& z : list) GSET_.insert(z);
            s->Add(k, std::move(list));
        }
    }
    for (Source* s : {&W_, &CW_, &KW_, &GW_}) s->Index();
    loaded_ = true;
}

int Engine::Score(const std::wstring& r, const std::wstring& z) const {
    auto it = userF_.find(r + L"\t" + z);
    return it == userF_.end() ? 0 : it->second;
}

// 한문 모드 글자 순위: 말뭉치 빈도 + 앞 글자 다음에 자주 오는 글자(문맥) + 한문 우선 글자
std::vector<std::wstring> Engine::ClassicalChars(const std::wstring& r, const std::wstring& prev) const {
    static const std::vector<std::wstring> none;
    auto get = [](const std::unordered_map<std::wstring, std::vector<std::wstring>>& m, const std::wstring& k)
        -> const std::vector<std::wstring>& { auto it = m.find(k); return it == m.end() ? none : it->second; };
    const auto& pr = get(P_, r);
    const auto& cr = get(C_, r);
    const auto& fol = prev.empty() ? none : get(F_, prev);
    std::vector<std::wstring> all;
    std::unordered_set<std::wstring> seen;
    for (auto* L : {&pr, &cr}) for (auto& ch : *L) if (seen.insert(ch).second) all.push_back(ch);
    std::vector<std::pair<double, size_t>> sc;
    for (size_t i = 0; i < all.size(); i++) {
        const auto& ch = all[i];
        auto ui = U_.find(ch);
        int u = ui == U_.end() ? 0 : ui->second;
        auto fi = std::find(fol.begin(), fol.end(), ch);
        double s = std::log(u + 1.0);
        if (fi != fol.end()) s += 4 + (80 - double(fi - fol.begin())) / 16;
        if (std::find(pr.begin(), pr.end(), ch) != pr.end()) s += 1.5;
        if (!u) s -= 50;
        sc.push_back({s - i * 1e-6, i});
    }
    std::sort(sc.begin(), sc.end(), [](auto& a, auto& b) { return a.first > b.first; });
    std::vector<std::wstring> out;
    for (auto& e : sc) out.push_back(all[e.second]);
    return out;
}

// 친 독음의 앞부분부터: 긴 단어 → 짧은 단어 → 첫 글자
std::vector<Candidate> Engine::Candidates(const std::wstring& run, bool exact, const std::wstring& prev) {
    std::vector<Candidate> out;
    std::unordered_set<std::wstring> seen;
    const std::vector<std::wstring>* fol = nullptr;
    if (!prev.empty()) { auto it = F_.find(prev); if (it != F_.end()) fol = &it->second; }
    for (size_t k = run.size(); k >= 1; k--) {
        if (exact && k != run.size() && k != 1) continue;
        std::wstring r = run.substr(0, k);
        std::vector<std::wstring> list;
        if (k == 1) list = ClassicalChars(r, prev);
        else for (Source* s : {&UW_, &CW_, &KW_, &GW_, &W_}) if (auto* L = s->Get(r)) list.insert(list.end(), L->begin(), L->end());
        std::vector<std::wstring> base;
        std::unordered_set<std::wstring> dup;
        for (auto& z : list) if (dup.insert(z).second) base.push_back(z);
        std::vector<std::pair<int, size_t>> ranked;
        for (size_t i = 0; i < base.size(); i++) ranked.push_back({Score(r, base[i]), i});
        std::stable_sort(ranked.begin(), ranked.end(), [](auto& a, auto& b) { return a.first > b.first; });
        for (auto& e : ranked) {
            const auto& z = base[e.second];
            if (!seen.insert(std::to_wstring(k) + L":" + z).second) continue;
            Candidate c;
            c.len = (int)k; c.read = r; c.text = z; c.learned = e.first > 0;
            auto m = M_.find(z); if (m != M_.end()) c.mean = m->second;
            c.ctx = k == 1 && fol && std::find(fol->begin(), fol->end(), z) != fol->end();
            out.push_back(std::move(c));
        }
    }
    return out;
}

// 예측: 지금까지 친 음으로 시작하는 단어·어구·구절
std::vector<Candidate> Engine::Predict(const std::wstring& run, size_t max) {
    struct P { Candidate c; std::array<long long, 4> k; };
    std::vector<P> out;
    std::unordered_set<std::wstring> seen;
    Source* srcs[] = {&UW_, &CW_, &GW_, &KW_, &W_};
    for (int si = 0; si < 5; si++) {
        Source* src = srcs[si];
        src->Index();
        auto it = std::lower_bound(src->keys.begin(), src->keys.end(), run);
        for (; it != src->keys.end() && StartsWith(*it, run); ++it) {
            const std::wstring& r = *it;
            if (r.size() <= run.size()) continue;
            const auto& L = src->map[r];
            for (size_t j = 0; j < L.size(); j++) {
                const auto& z = L[j];
                // 논어·맹자 구절은 1/3 넘게 쳤을 때만
                if (GSET_.count(z) && (run.size() < 2 || run.size() * 3 <= r.size())) continue;
                if (!seen.insert(z).second) continue;
                int s = Score(r, z);
                P p;
                p.c.len = (int)run.size(); p.c.read = r; p.c.text = z; p.c.pred = true; p.c.learned = s > 0;
                auto m = M_.find(z); if (m != M_.end()) p.c.mean = m->second;
                p.k = {-s, si, r.size() > run.size() + 4 ? 1 : 0, (long long)src->rank[r] * 8 + (long long)j};
                out.push_back(std::move(p));
            }
            if (out.size() > 400) break;
        }
    }
    std::stable_sort(out.begin(), out.end(), [](const P& a, const P& b) { return a.k < b.k; });
    std::vector<Candidate> res;
    for (size_t i = 0; i < out.size() && i < max; i++) res.push_back(std::move(out[i].c));
    return res;
}

std::vector<Candidate> Engine::WithPredictions(std::vector<Candidate> list, const std::wstring& run, bool predict) {
    if (!predict || run.empty()) return list;
    auto p = Predict(run, run.size() >= 2 ? 5 : 3);
    if (p.empty()) return list;
    size_t at;
    if (run.size() == 1) at = std::min<size_t>(2, list.size());
    else {
        at = list.size();
        for (size_t i = 0; i < list.size(); i++) if (list[i].len < (int)run.size()) { at = i; break; }
    }
    list.insert(list.begin() + at, p.begin(), p.end());
    return list;
}

void Engine::LoadUser(const std::wstring& text) {
    userF_.clear();
    UW_ = Source();
    for (auto& line : SplitBy(text, L'\n')) {
        auto f = SplitBy(line, L'\t');
        if (f.size() < 3 || f[1].empty() || f[2].empty()) continue;
        std::wstring z = f[2];
        while (!z.empty() && (z.back() == L'\r')) z.pop_back();
        if (f[0] == L"f") userF_[f[1] + L"\t" + z]++;
        else if (f[0] == L"w") UW_.Add(f[1], {z}, true);
    }
}

void Engine::ClearUser() { userF_.clear(); UW_ = Source(); }

std::wstring Engine::Learn(const std::wstring& r, const std::wstring& z) {
    userF_[r + L"\t" + z]++;
    return L"f\t" + r + L"\t" + z + L"\n";
}

std::wstring Engine::AddWord(const std::wstring& r, const std::wstring& z) {
    if (auto* L = UW_.Get(r)) if (std::find(L->begin(), L->end(), z) != L->end()) return L"";
    UW_.Add(r, {z}, true);
    return L"w\t" + r + L"\t" + z + L"\n";
}

std::wstring Engine::Format(const std::wstring& r, const std::wstring& z, int mode) {
    if (mode == 1) return z + L"(" + r + L")";
    if (mode == 2) return r + L"(" + z + L")";
    return z;
}
