#!/bin/bash
# 빠른 한문 입력기 macOS 빌드 (macOS + Xcode 명령행 도구 필요)
# 결과: build/Hanmun.app, dist/hanmun-mac-<버전>.pkg
set -euo pipefail
cd "$(dirname "$0")"
VERSION=$(sed -n 's/.*project(hanmun VERSION \([0-9.]*\).*/\1/p' ../windows/CMakeLists.txt)
APP=build/root/Library/Input\ Methods/Hanmun.app
rm -rf build && mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources" ../dist build/icons

# 1) 프로그램 (Apple Silicon + Intel)
clang++ -std=c++17 -fobjc-arc -O2 -arch arm64 -arch x86_64 -mmacosx-version-min=12.0 \
  -I../windows/src -Isrc src/*.mm ../windows/src/Engine.cpp ../windows/src/Hangul.cpp \
  -framework Cocoa -framework InputMethodKit -framework Carbon \
  -o "$APP/Contents/MacOS/Hanmun"

# 2) 사전·설정·아이콘
python3 ../windows/tools/make_dict.py ../web/src "$APP/Contents/Resources/hanmun.dict"
sed "s/@VERSION@/$VERSION/" Info.plist > "$APP/Contents/Info.plist"
cp -R resources/*.lproj "$APP/Contents/Resources/"
python3 tools/make_icons.py build/icons
iconutil -c icns build/icons/AppIcon.iconset -o "$APP/Contents/Resources/AppIcon.icns"
for n in hangul hanmun; do
  tiffutil -cathidpicheck "build/icons/$n.png" "build/icons/$n@2x.png" -out "$APP/Contents/Resources/$n.tiff"
done
cp ../LICENSE ../THIRD_PARTY_NOTICES.txt "$APP/Contents/Resources/"

# 3) 서명(임시 서명) — 개발자 계정 없이도 Apple Silicon 에서 실행되도록
codesign --force --deep --sign - "$APP"

# 4) 설치 패키지: /Library/Input Methods 에 설치
mkdir -p build/scripts
cat > build/scripts/postinstall <<'POST'
#!/bin/bash
# 이전 버전이 떠 있으면 내려서 새 버전이 뜨게 한다
pkill -x Hanmun || true
exit 0
POST
chmod +x build/scripts/postinstall
pkgbuild --root build/root --scripts build/scripts --identifier com.segrc.inputmethod.Hanmun \
  --version "$VERSION" --install-location / "../dist/hanmun-mac-$VERSION.pkg"
echo "done: dist/hanmun-mac-$VERSION.pkg"
