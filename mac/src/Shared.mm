// 빠른 한문 입력기 (macOS) — 공용
// Copyright (c) 2026 세 (@segrc_) — MIT License
#import "Shared.h"

NSString* NSFromW(const std::wstring& w) {
    if (w.empty()) return @"";
    return [[NSString alloc] initWithBytes:w.data()
                                    length:w.size() * sizeof(wchar_t)
                                  encoding:(sizeof(wchar_t) == 4 ? NSUTF32LittleEndianStringEncoding
                                                                 : NSUTF16LittleEndianStringEncoding)];
}

std::wstring WFromNS(NSString* s) {
    if (!s.length) return L"";
    NSData* d = [s dataUsingEncoding:(sizeof(wchar_t) == 4 ? NSUTF32LittleEndianStringEncoding
                                                           : NSUTF16LittleEndianStringEncoding)];
    return std::wstring((const wchar_t*)d.bytes, d.length / sizeof(wchar_t));
}

static NSString* UserFile() {
    NSArray* dirs = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
    NSString* dir = [dirs.firstObject stringByAppendingPathComponent:@"Hanmun"];
    [[NSFileManager defaultManager] createDirectoryAtPath:dir withIntermediateDirectories:YES attributes:nil error:nil];
    return [dir stringByAppendingPathComponent:@"user.tsv"];
}

Engine& SharedEngine() {
    static Engine engine;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        NSString* path = [[NSBundle mainBundle] pathForResource:@"hanmun" ofType:@"dict"];
        NSString* dict = path ? [NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil] : nil;
        if (dict) engine.Load(WFromNS(dict));
        NSString* user = [NSString stringWithContentsOfFile:UserFile() encoding:NSUTF8StringEncoding error:nil];
        if (user) engine.LoadUser(WFromNS(user));
    });
    return engine;
}

void SaveUserLine(const std::wstring& line) {
    if (line.empty()) return;
    NSString* file = UserFile();
    if (![[NSFileManager defaultManager] fileExistsAtPath:file])
        [[NSFileManager defaultManager] createFileAtPath:file contents:nil attributes:nil];
    NSFileHandle* h = [NSFileHandle fileHandleForWritingAtPath:file];
    [h seekToEndOfFile];
    [h writeData:[NSFromW(line) dataUsingEncoding:NSUTF8StringEncoding]];
    [h closeFile];
}

void ClearUserData() {
    [[NSFileManager defaultManager] removeItemAtPath:UserFile() error:nil];
    SharedEngine().ClearUser();
}

int FormatSetting() {
    NSInteger f = [[NSUserDefaults standardUserDefaults] integerForKey:@"Format"];
    return (f >= 0 && f <= 2) ? (int)f : 0;
}
void SetFormatSetting(int f) { [[NSUserDefaults standardUserDefaults] setInteger:f forKey:@"Format"]; }
BOOL PredictSetting() {
    id v = [[NSUserDefaults standardUserDefaults] objectForKey:@"Predict"];
    return v ? [v boolValue] : YES;
}
void SetPredictSetting(BOOL on) { [[NSUserDefaults standardUserDefaults] setBool:on forKey:@"Predict"]; }
