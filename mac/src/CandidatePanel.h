// 한자 후보 창 (세로 목록)
// Copyright (c) 2026 세 (@segrc_) — MIT License
#pragma once
#import <Cocoa/Cocoa.h>

#include <vector>

#include "Engine.h"

static const int kPage = 9;

@interface CandidatePanel : NSObject
+ (instancetype)shared;
// lineRect: 조합 글자의 화면 위치(아래-왼쪽 원점). 그 아래(자리가 없으면 위)에 띄운다
- (void)showCandidates:(const std::vector<Candidate>&)cands selected:(int)sel at:(NSRect)lineRect;
- (void)hide;
@property(nonatomic, copy) void (^onPick)(int index);
@end
