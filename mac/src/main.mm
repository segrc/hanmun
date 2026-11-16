// 빠른 한문 입력기 (macOS) — 입력기 서버
// Copyright (c) 2026 세 (@segrc_) — MIT License
#import <Cocoa/Cocoa.h>
#import <InputMethodKit/InputMethodKit.h>

#import "Shared.h"

static IMKServer* g_server;

int main(int argc, const char* argv[]) {
    @autoreleasepool {
        NSBundle* b = [NSBundle mainBundle];
        NSString* name = [b objectForInfoDictionaryKey:@"InputMethodConnectionName"];
        g_server = [[IMKServer alloc] initWithName:name bundleIdentifier:b.bundleIdentifier];
        SharedEngine();   // 사전을 미리 읽어 둔다
        [NSApplication sharedApplication];
        [NSApp run];
    }
    return 0;
}
