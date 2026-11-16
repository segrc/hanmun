// 빠른 한문 입력기 (macOS) — 입력 처리
// Copyright (c) 2026 세 (@segrc_) — MIT License
#import <Carbon/Carbon.h>
#import <InputMethodKit/InputMethodKit.h>

#import "CandidatePanel.h"
#import "Shared.h"

#include <utility>
#include <vector>

#include "Hangul.h"

static BOOL g_hanmun = NO;   // 한문(漢) 모드: 모든 앱이 함께 쓴다

@interface HanmunController : IMKInputController
@end

@implementation HanmunController {
    Hangul hg_;
    std::wstring run_;                                           // 아직 바꾸지 않은 한글
    std::vector<std::pair<std::wstring, std::wstring>> picks_;   // 이번 조합에서 고른 (독음, 한자)
    std::wstring lastChar_;                                      // 바로 앞에 넣은 한자(문맥)
    std::vector<Candidate> cands_;
    int sel_;
}

// ─────────────────────────────── 키 → 자모 (자판 배열과 무관하게 실제 키 위치로)

static char LetterForKeyCode(unsigned short k) {
    switch (k) {
        case kVK_ANSI_A: return 'A'; case kVK_ANSI_B: return 'B'; case kVK_ANSI_C: return 'C'; case kVK_ANSI_D: return 'D';
        case kVK_ANSI_E: return 'E'; case kVK_ANSI_F: return 'F'; case kVK_ANSI_G: return 'G'; case kVK_ANSI_H: return 'H';
        case kVK_ANSI_I: return 'I'; case kVK_ANSI_J: return 'J'; case kVK_ANSI_K: return 'K'; case kVK_ANSI_L: return 'L';
        case kVK_ANSI_M: return 'M'; case kVK_ANSI_N: return 'N'; case kVK_ANSI_O: return 'O'; case kVK_ANSI_P: return 'P';
        case kVK_ANSI_Q: return 'Q'; case kVK_ANSI_R: return 'R'; case kVK_ANSI_S: return 'S'; case kVK_ANSI_T: return 'T';
        case kVK_ANSI_U: return 'U'; case kVK_ANSI_V: return 'V'; case kVK_ANSI_W: return 'W'; case kVK_ANSI_X: return 'X';
        case kVK_ANSI_Y: return 'Y'; case kVK_ANSI_Z: return 'Z';
    }
    return 0;
}

- (BOOL)composing { return !hg_.Empty() || !run_.empty() || !picks_.empty(); }

- (std::wstring)pickedText:(BOOL)formatted {
    std::wstring R, Z;
    for (auto& p : picks_) { R += p.first; Z += p.second; }
    if (Z.empty()) return L"";
    return formatted ? Engine::Format(R, Z, FormatSetting()) : Z;
}

// ─────────────────────────────── 모드

- (void)setValue:(id)value forTag:(long)tag client:(id)sender {
    if (tag == kTextServiceInputModePropertyTag && [value isKindOfClass:[NSString class]]) {
        BOOL han = [value isEqualToString:kModeHanmun];
        if (han != g_hanmun) {
            [self commitAll:sender];
            g_hanmun = han;
        }
    }
    [super setValue:value forTag:tag client:sender];
}

// 한문 ↔ 한글 바꾸기: 메뉴 막대 표시도 같이 바뀌도록 입력 소스를 고른다
- (void)toggleMode:(id)client {
    [self commitAll:client];
    NSString* target = g_hanmun ? kModeHangul : kModeHanmun;
    NSDictionary* q = @{(__bridge NSString*)kTISPropertyInputSourceID : target};
    CFArrayRef list = TISCreateInputSourceList((__bridge CFDictionaryRef)q, false);
    if (list && CFArrayGetCount(list) > 0) TISSelectInputSource((TISInputSourceRef)CFArrayGetValueAtIndex(list, 0));
    if (list) CFRelease(list);
    g_hanmun = !g_hanmun;
}

// ─────────────────────────────── 키 입력

- (NSUInteger)recognizedEvents:(id)sender { return NSEventMaskKeyDown | NSEventMaskFlagsChanged; }

- (BOOL)handleEvent:(NSEvent*)event client:(id)sender {
    if (event.type != NSEventTypeKeyDown) return NO;
    NSEventModifierFlags m = event.modifierFlags;
    BOOL shift = (m & NSEventModifierFlagShift) != 0, opt = (m & NSEventModifierFlagOption) != 0;
    BOOL cmd = (m & NSEventModifierFlagCommand) != 0, ctrl = (m & NSEventModifierFlagControl) != 0;
    unsigned short key = event.keyCode;
    BOOL comp = [self composing];

    // Option+Return: 한글 ↔ 한문 (맥 한국어 입력기의 한자 키와 같은 자리)
    if (opt && !cmd && !ctrl && (key == kVK_Return || key == kVK_ANSI_KeypadEnter)) {
        [self toggleMode:sender];
        return YES;
    }
    if (cmd || ctrl || opt) {
        if (comp) [self commitAll:sender];
        return NO;
    }
    if (char L = LetterForKeyCode(key)) {
        [self feedJamo:Hangul::KeyToJamo((unsigned)L, shift) client:sender];
        return YES;
    }
    int n = (int)cands_.size();
    if (g_hanmun && n) {
        switch (key) {
            case kVK_Space: [self moveBy:(shift ? -1 : 1) client:sender]; return YES;
            case kVK_DownArrow: [self moveBy:1 client:sender]; return YES;
            case kVK_UpArrow: [self moveBy:-1 client:sender]; return YES;
            case kVK_Tab: [self pageBy:(shift ? -1 : 1) client:sender]; return YES;
            case kVK_PageDown: [self pageBy:1 client:sender]; return YES;
            case kVK_PageUp: [self pageBy:-1 client:sender]; return YES;
            case kVK_Return: case kVK_ANSI_KeypadEnter: [self pick:sel_ client:sender]; return YES;
        }
        NSString* ch = event.charactersIgnoringModifiers;
        if (!shift && ch.length == 1) {
            unichar c = [ch characterAtIndex:0];
            if (c >= '1' && c <= '9') {
                int idx = (sel_ / kPage) * kPage + (c - '1');
                if (idx < n) { [self pick:idx client:sender]; return YES; }
            }
        }
    }
    if (!comp) return NO;
    if (key == kVK_Delete) { [self backspace:sender]; return YES; }
    if (key == kVK_Escape) { [self commitAll:sender]; return YES; }                       // 한글 그대로 확정
    if ((key == kVK_Return || key == kVK_ANSI_KeypadEnter) && g_hanmun) { [self commitAll:sender]; return YES; }
    [self commitAll:sender];   // 그 밖의 키: 확정한 뒤 앱이 처리
    lastChar_.clear();
    return NO;
}

- (void)feedJamo:(wchar_t)j client:(id)sender {
    if (!j) return;
    std::wstring out = hg_.Feed(j), commit;
    if (g_hanmun) {
        if (!out.empty()) {
            if (Hangul::IsSyllable(out[0])) run_ += out;
            else { commit = [self pickedText:YES] + run_ + out; run_.clear(); picks_.clear(); }
        }
        if (picks_.empty() && run_.size() > 16) {
            commit += run_.substr(0, run_.size() - 16);
            run_.erase(0, run_.size() - 16);
        }
        [self refresh];
    } else commit = out;
    [self update:sender commit:commit];
}

- (void)backspace:(id)sender {
    if (!hg_.Empty()) hg_.Backspace();
    else if (!run_.empty()) run_.pop_back();
    else if (!picks_.empty()) { run_ = picks_.back().first + run_; picks_.pop_back(); }
    [self refresh];
    [self update:sender commit:L""];
}

- (void)moveBy:(int)d client:(id)sender {
    int n = (int)cands_.size();
    if (!n) return;
    sel_ = (sel_ + d + n) % n;
    [self showCandidates:sender];
}

- (void)pageBy:(int)d client:(id)sender {
    int n = (int)cands_.size();
    if (!n) return;
    int pages = (n + kPage - 1) / kPage, p = sel_ / kPage + d;
    if (p >= pages) p = 0;
    if (p < 0) p = pages - 1;
    sel_ = p * kPage;
    [self showCandidates:sender];
}

- (void)pick:(int)index client:(id)sender {
    if (index < 0 || index >= (int)cands_.size()) return;
    if (hg_.IsFullSyllable()) run_ += hg_.Flush();
    Candidate c = cands_[index];
    if (c.len <= 0 || c.len > (int)run_.size()) return;
    picks_.push_back({c.read, c.text});
    run_.erase(0, c.len);
    Engine& e = SharedEngine();
    SaveUserLine(e.Learn(c.read, c.text));
    auto chars = Engine::Chars(c.text);
    if (!chars.empty()) lastChar_ = chars.back();
    std::wstring commit;
    if (run_.empty() && hg_.Empty()) {
        commit = [self pickedText:YES];
        if (picks_.size() >= 2) {   // 이어서 고른 것은 한 단어로 기억 (8자까지)
            std::wstring R, Z;
            for (auto& p : picks_) { R += p.first; Z += p.second; }
            size_t zc = Engine::Chars(Z).size();
            if (zc >= 2 && zc <= 8) SaveUserLine(e.AddWord(R, Z));
        }
        picks_.clear();
    }
    [self refresh];
    [self update:sender commit:commit];
}

- (void)commitAll:(id)sender {
    std::wstring commit = [self pickedText:YES] + run_ + hg_.Current();
    hg_.Reset();
    run_.clear();
    picks_.clear();
    cands_.clear();
    sel_ = 0;
    [self update:sender commit:commit];
}

- (void)refresh {
    cands_.clear();
    sel_ = 0;
    if (!g_hanmun) return;
    std::wstring r = run_ + (hg_.IsFullSyllable() ? hg_.Current() : L"");
    if (r.empty()) return;
    std::wstring prev = lastChar_;
    if (!picks_.empty()) {
        auto ch = Engine::Chars(picks_.back().second);
        if (!ch.empty()) prev = ch.back();
    }
    Engine& e = SharedEngine();
    cands_ = e.WithPredictions(e.Candidates(r, false, prev), r, PredictSetting());
}

// ─────────────────────────────── 조합 글자

- (void)update:(id)sender commit:(const std::wstring&)commit {
    if (!commit.empty())
        [sender insertText:NSFromW(commit) replacementRange:NSMakeRange(NSNotFound, NSNotFound)];
    std::wstring comp = [self pickedText:NO] + run_ + hg_.Current();
    NSString* s = NSFromW(comp);
    NSAttributedString* marked = [[NSAttributedString alloc]
        initWithString:s
            attributes:@{NSUnderlineStyleAttributeName : @(NSUnderlineStyleSingle), NSMarkedClauseSegmentAttributeName : @0}];
    if (s.length || commit.empty())
        [sender setMarkedText:marked selectionRange:NSMakeRange(s.length, 0) replacementRange:NSMakeRange(NSNotFound, NSNotFound)];
    [self showCandidates:sender];
}

- (void)showCandidates:(id)sender {
    CandidatePanel* p = [CandidatePanel shared];
    if (cands_.empty() || ![self composing]) { [p hide]; return; }
    NSRect line = NSZeroRect;
    [sender attributesForCharacterIndex:0 lineHeightRectangle:&line];
    __weak HanmunController* weakSelf = self;
    __weak id client = sender;
    p.onPick = ^(int index) {
        HanmunController* me = weakSelf;
        if (me && client) [me pick:index client:client];
    };
    [p showCandidates:cands_ selected:sel_ at:line];
}

// 포커스가 옮겨 가거나 앱이 조합을 끝낼 때
- (void)commitComposition:(id)sender {
    [self commitAll:sender];
    lastChar_.clear();
}

- (void)deactivateServer:(id)sender {
    [self commitComposition:sender];
    [[CandidatePanel shared] hide];
    [super deactivateServer:sender];
}

// ─────────────────────────────── 메뉴 막대의 입력기 메뉴

- (NSMenu*)menu {
    NSMenu* m = [[NSMenu alloc] initWithTitle:@"Hanmun"];
    int f = FormatSetting();
    NSArray* names = @[ @"표기: 漢字", @"표기: 漢字(한자)", @"표기: 한자(漢字)" ];
    SEL sels[3] = {@selector(format0:), @selector(format1:), @selector(format2:)};
    for (int i = 0; i < 3; i++) {
        NSMenuItem* it = [[NSMenuItem alloc] initWithTitle:names[i] action:sels[i] keyEquivalent:@""];
        it.state = f == i ? NSControlStateValueOn : NSControlStateValueOff;
        [m addItem:it];
    }
    [m addItem:[NSMenuItem separatorItem]];
    NSMenuItem* pr = [[NSMenuItem alloc] initWithTitle:@"어구 예측" action:@selector(togglePredict:) keyEquivalent:@""];
    pr.state = PredictSetting() ? NSControlStateValueOn : NSControlStateValueOff;
    [m addItem:pr];
    [m addItem:[NSMenuItem separatorItem]];
    [m addItem:[[NSMenuItem alloc] initWithTitle:@"내 단어 초기화…" action:@selector(resetWords:) keyEquivalent:@""]];
    [m addItem:[[NSMenuItem alloc] initWithTitle:@"빠른 한문 입력기 정보" action:@selector(about:) keyEquivalent:@""]];
    return m;
}

- (void)format0:(id)sender { SetFormatSetting(0); }
- (void)format1:(id)sender { SetFormatSetting(1); }
- (void)format2:(id)sender { SetFormatSetting(2); }
- (void)togglePredict:(id)sender { SetPredictSetting(!PredictSetting()); }

- (void)resetWords:(id)sender {
    NSAlert* a = [NSAlert new];
    a.messageText = @"고른 횟수와 내 단어를 모두 지울까요?";
    a.informativeText = @"되돌릴 수 없습니다.";
    [a addButtonWithTitle:@"지우기"];
    [a addButtonWithTitle:@"취소"];
    [NSApp activateIgnoringOtherApps:YES];
    if ([a runModal] == NSAlertFirstButtonReturn) ClearUserData();
}

- (void)about:(id)sender {
    NSString* v = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
    NSAlert* a = [NSAlert new];
    a.messageText = [NSString stringWithFormat:@"빠른 한문 입력기 v%@", v];
    a.informativeText = @"제작자: 세 (@segrc_)\n\n"
                        @"메뉴 막대에서 ‘한문(漢)’을 고르거나 Option+Return 으로 한글 ↔ 한문을 바꿉니다.\n"
                        @"독음을 치면 후보가 뜨고, Space로 이동, Return·숫자로 고릅니다. Esc는 한글 그대로 확정합니다.\n\n"
                        @"웹 버전: https://hanmun.j-segrc.workers.dev";
    [NSApp activateIgnoringOtherApps:YES];
    [a runModal];
}

@end
