// 한자 후보 창 (세로 목록)
// Copyright (c) 2026 세 (@segrc_) — MIT License
#import "CandidatePanel.h"

#import "Shared.h"

namespace {
const CGFloat kMainSize = 16, kSmallSize = 11, kPad = 4, kGap = 8, kRowExtra = 5;
NSColor* Accent() { return [NSColor colorWithSRGBRed:138 / 255.0 green:204 / 255.0 blue:92 / 255.0 alpha:1]; }
NSColor* AccentText() { return [NSColor colorWithSRGBRed:28 / 255.0 green:40 / 255.0 blue:12 / 255.0 alpha:1]; }
NSColor* PredColor() { return [NSColor colorWithSRGBRed:62 / 255.0 green:128 / 255.0 blue:32 / 255.0 alpha:1]; }
}  // namespace

@interface CandidateRow : NSObject
@property(nonatomic, copy) NSString* text;
@property(nonatomic, copy) NSString* extra;
@property(nonatomic) BOOL pred;
@end
@implementation CandidateRow
@end

@interface CandidateView : NSView
@property(nonatomic, strong) NSArray<CandidateRow*>* rows;
@property(nonatomic) int selected;   // 이 페이지 안에서
@property(nonatomic) int page, pages, base;
@property(nonatomic) CGFloat rowH, numW, textW;
@property(nonatomic, weak) CandidatePanel* owner;
@end

@interface CandidatePanel ()
@property(nonatomic, strong) NSPanel* panel;
@property(nonatomic, strong) CandidateView* view;
@end

static NSFont* MainFont() {
    NSFont* f = [NSFont fontWithName:@"AppleSDGothicNeo-Regular" size:kMainSize];
    return f ?: [NSFont systemFontOfSize:kMainSize];
}
static NSFont* SmallFont() { return [NSFont systemFontOfSize:kSmallSize]; }

@implementation CandidateView
- (BOOL)isFlipped { return YES; }
- (BOOL)acceptsFirstMouse:(NSEvent*)e { return YES; }

- (void)drawRect:(NSRect)dirty {
    NSRect b = self.bounds;
    BOOL dark = [self.effectiveAppearance bestMatchFromAppearancesWithNames:@[ NSAppearanceNameAqua, NSAppearanceNameDarkAqua ]] ==
                NSAppearanceNameDarkAqua;
    NSColor* bg = dark ? [NSColor colorWithWhite:0.16 alpha:1] : [NSColor whiteColor];
    NSColor* fg = dark ? [NSColor colorWithWhite:0.93 alpha:1] : [NSColor colorWithWhite:0.11 alpha:1];
    NSColor* dim = dark ? [NSColor colorWithWhite:0.6 alpha:1] : [NSColor colorWithWhite:0.5 alpha:1];
    NSBezierPath* frame = [NSBezierPath bezierPathWithRoundedRect:NSInsetRect(b, 0.5, 0.5) xRadius:7 yRadius:7];
    [bg setFill];
    [frame fill];
    [(dark ? [NSColor colorWithWhite:0.3 alpha:1] : [NSColor colorWithWhite:0.85 alpha:1]) setStroke];
    [frame stroke];

    for (int i = 0; i < (int)self.rows.count; i++) {
        CandidateRow* r = self.rows[i];
        BOOL on = i == self.selected;
        NSRect row = NSMakeRect(kPad, kPad + i * self.rowH, b.size.width - kPad * 2, self.rowH);
        if (on) {
            [Accent() setFill];
            [[NSBezierPath bezierPathWithRoundedRect:row xRadius:5 yRadius:5] fill];
        }
        CGFloat x = row.origin.x + 6;
        NSDictionary* numA = @{NSFontAttributeName : SmallFont(), NSForegroundColorAttributeName : on ? AccentText() : dim};
        NSString* num = [NSString stringWithFormat:@"%d", i + 1];
        NSSize ns = [num sizeWithAttributes:numA];
        [num drawAtPoint:NSMakePoint(x, NSMidY(row) - ns.height / 2) withAttributes:numA];
        x += self.numW + kGap;
        NSDictionary* mainA = @{NSFontAttributeName : MainFont(), NSForegroundColorAttributeName : on ? AccentText() : fg};
        NSSize ts = [r.text sizeWithAttributes:mainA];
        [r.text drawAtPoint:NSMakePoint(x, NSMidY(row) - ts.height / 2) withAttributes:mainA];
        x += self.textW + kGap;
        if (r.extra.length) {
            NSDictionary* exA = @{NSFontAttributeName : SmallFont(),
                                  NSForegroundColorAttributeName : on ? AccentText() : (r.pred ? PredColor() : dim)};
            NSSize es = [r.extra sizeWithAttributes:exA];
            [r.extra drawInRect:NSMakeRect(x, NSMidY(row) - es.height / 2, NSMaxX(row) - x - 4, es.height) withAttributes:exA];
        }
    }
    if (self.pages > 1) {
        NSDictionary* a = @{NSFontAttributeName : SmallFont(), NSForegroundColorAttributeName : dim};
        NSString* s = [NSString stringWithFormat:@"%d/%d  Tab ▸", self.page + 1, self.pages];
        NSSize sz = [s sizeWithAttributes:a];
        [s drawAtPoint:NSMakePoint(b.size.width - kPad - 4 - sz.width, b.size.height - kPad - sz.height) withAttributes:a];
    }
}

- (void)mouseDown:(NSEvent*)e {
    NSPoint p = [self convertPoint:e.locationInWindow fromView:nil];
    int i = (int)floor((p.y - kPad) / self.rowH);
    if (p.y >= kPad && i >= 0 && i < (int)self.rows.count && self.owner.onPick) self.owner.onPick(self.base + i);
}
@end

@implementation CandidatePanel

+ (instancetype)shared {
    static CandidatePanel* s;
    static dispatch_once_t once;
    dispatch_once(&once, ^{ s = [CandidatePanel new]; });
    return s;
}

- (instancetype)init {
    if ((self = [super init])) {
        _panel = [[NSPanel alloc] initWithContentRect:NSMakeRect(0, 0, 100, 100)
                                            styleMask:NSWindowStyleMaskBorderless | NSWindowStyleMaskNonactivatingPanel
                                              backing:NSBackingStoreBuffered
                                                defer:YES];
        _panel.level = NSPopUpMenuWindowLevel + 1;
        _panel.opaque = NO;
        _panel.backgroundColor = [NSColor clearColor];
        _panel.hasShadow = YES;
        _panel.hidesOnDeactivate = NO;
        _panel.collectionBehavior = NSWindowCollectionBehaviorCanJoinAllSpaces | NSWindowCollectionBehaviorFullScreenAuxiliary;
        _view = [[CandidateView alloc] initWithFrame:NSMakeRect(0, 0, 100, 100)];
        _view.owner = self;
        _panel.contentView = _view;
    }
    return self;
}

- (void)showCandidates:(const std::vector<Candidate>&)cands selected:(int)sel at:(NSRect)line {
    if (cands.empty()) { [self hide]; return; }
    int page = sel / kPage, pages = ((int)cands.size() + kPage - 1) / kPage, base = page * kPage;
    NSMutableArray* rows = [NSMutableArray array];
    CGFloat textW = 0, extraW = 0;
    NSDictionary* mainA = @{NSFontAttributeName : MainFont()};
    NSDictionary* smallA = @{NSFontAttributeName : SmallFont()};
    for (int i = base; i < (int)cands.size() && i < base + kPage; i++) {
        const Candidate& c = cands[i];
        CandidateRow* r = [CandidateRow new];
        r.text = NSFromW(c.text);
        NSMutableString* ex = [NSMutableString string];
        if (c.pred) [ex appendString:NSFromW(c.read)];
        if (!c.mean.empty() && Engine::Chars(c.text).size() == 1) {
            if (ex.length) [ex appendString:@"  "];
            [ex appendString:NSFromW(c.mean)];
        }
        r.extra = ex;
        r.pred = c.pred;
        textW = MAX(textW, [r.text sizeWithAttributes:mainA].width);
        if (ex.length) extraW = MAX(extraW, [ex sizeWithAttributes:smallA].width);
        [rows addObject:r];
    }
    extraW = MIN(extraW, 190);
    CGFloat rowH = ceil([@"漢가" sizeWithAttributes:mainA].height) + kRowExtra;
    CGFloat numW = [@"9" sizeWithAttributes:smallA].width;
    CGFloat footH = pages > 1 ? [@"1/2" sizeWithAttributes:smallA].height + 4 : 0;
    CGFloat w = MAX(100, kPad * 2 + 6 + numW + kGap + textW + (extraW ? kGap + extraW : 0) + 10);
    CGFloat h = kPad * 2 + rowH * rows.count + footH;

    _view.rows = rows;
    _view.selected = sel - base;
    _view.page = page;
    _view.pages = pages;
    _view.base = base;
    _view.rowH = rowH;
    _view.numW = numW;
    _view.textW = textW;

    NSScreen* screen = [NSScreen mainScreen];
    for (NSScreen* s in [NSScreen screens])
        if (NSPointInRect(line.origin, s.frame)) { screen = s; break; }
    NSRect vis = screen.visibleFrame;
    CGFloat x = line.origin.x - kPad - 6 - numW - kGap / 2;
    CGFloat y = line.origin.y - h - 3;                            // 글자 아래
    if (y < vis.origin.y) y = NSMaxY(line) + 3;                   // 자리가 없으면 위
    x = MAX(vis.origin.x, MIN(x, NSMaxX(vis) - w));
    [_panel setFrame:NSMakeRect(x, y, w, h) display:NO];
    _view.frame = NSMakeRect(0, 0, w, h);
    [_view setNeedsDisplay:YES];
    [_panel orderFrontRegardless];
}

- (void)hide { [_panel orderOut:nil]; }
@end
