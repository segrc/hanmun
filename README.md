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

## 다음 단계
Windows 11 시스템 입력기(TSF, C++) — GitHub 저장소를 만들면 자동 빌드로 진행 예정.
