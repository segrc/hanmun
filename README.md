# 한문타이핑 — 빠른 한문 입력기

## 폴더 구성
- `web/deploy/` — Cloudflare에 그대로 올리는 완성본 (index.html, 글꼴, 라이선스, 방문 통계 코드 포함)
- `web/src/` — 웹 입력기 원본
  - `page.html` 화면, `engine.js` 후보 엔진(화면과 분리, 시스템 입력기로 옮길 부분)
  - `*.json` 사전 데이터 (글자·단어·고전 어구·실록 어구·경서 구절·훈음·통계)
  - `make.py` → `hanja-ime.html` 생성 (아티팩트용), `build_deploy.py` → `deploy/` 갱신 + zip
  - `fonts/` Pretendard(한글 부분), Pretendard JP Variable(한자)
- `data-build/` — 사전을 처음부터 다시 만드는 스크립트와 직접 작성한 단어 목록
  - `chars.py`(글자 독음표) → `build.py`(현대 한자어) → `build2.py`(고전 어구·이두)
  - `corpus*.py`(실록·승정원일기 통계), `classics*.py`(논어·맹자 구절)
  - 원자료(말뭉치 jsonl, 위키문헌 json, libhangul hanja.txt)는 `Downloads\Open-Korean-Historical-Corpus\왕대별`에 있음

## 다시 빌드하기 (web/src 안에서)
    python3 make.py          # hanja-ime.html 생성
    python3 build_deploy.py  # deploy/ 갱신 + hanmun-ime-cloudflare.zip

## Windows 11 입력기 (`windows/`)
웹 버전과 같은 엔진을 C++로 옮긴 시스템 입력기(TSF)입니다. 메모장, 한글, 워드, 브라우저 등 어디서나 씁니다.

- **A / 가 / 漢** 세 상태: 한/영 키로 A↔가, **한자 키 · 오른쪽 Ctrl(혼자 눌렀다 떼기) · 작업 표시줄 표시 클릭**으로 가↔漢 (모든 앱에 같이 적용)
- 漢(한문) 모드에서만 독음을 치면 한자 후보가 자동으로 뜹니다
  - Space·↓ 다음 후보, ↑ 이전, Tab 다음 쪽, Enter 고르기, 1~9 바로 고르기
  - Backspace 한 글자 지우기(고른 직후면 고른 것 되돌리기), Esc 한글 그대로 확정
- 작업 표시줄의 A/가/漢 표시를 왼쪽 클릭하면 상태 전환, 오른쪽 클릭하면 표기 형식·어구 예측·내 단어 초기화 메뉴
- 고른 기록·내 단어는 `%APPDATA%\Hanmun\user.tsv`

### 설치
GitHub Actions가 push할 때마다 설치 파일을 만듭니다: 저장소 **Actions** 탭 → 최근 실행 → `hanmun-windows` 받기 → `hanmun-setup-*.exe` 실행.
설치 후 **설정 → 시간 및 언어 → 언어 및 지역 → 한국어 ⋯ → 언어 옵션 → 키보드 추가 → 빠른 한문 입력기**.
(코드 서명이 없어 처음 실행할 때 SmartScreen 경고가 뜹니다: ‘추가 정보 → 실행’.)

### 구성
- `src/Hangul.*` 두벌식 오토마타, `src/Engine.*` 후보 엔진(engine.js 이식)
- `src/TextService.*` TSF 본체, `src/CandidateWindow.*` 후보 창, `src/LangBarButton.*` 상태 표시
- `tools/make_dict.py` 웹 사전 JSON → `hanmun.dict` (빌드 때 DLL 안에 들어감)
- `installer/hanmun.iss` 설치 파일(Inno Setup), `.github/workflows/windows.yml` 자동 빌드
