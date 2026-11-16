# 빠른 한문 입력기

한글로 음을 치면 한자 후보가 바로 뜨는 웹 한문 입력기입니다. 서버 없이 정적 파일만으로 동작합니다.

## Cloudflare Pages에 올리기 (가장 쉬운 방법)
1. https://dash.cloudflare.com 에 로그인 → 왼쪽 메뉴 **Workers & Pages** → **Create** → **Pages** → **Upload assets**(직접 업로드)
2. 프로젝트 이름 입력 (예: `hanmun-ime`) → 이 폴더 전체(또는 zip)를 끌어다 놓기 → **Deploy**
3. `https://프로젝트이름.pages.dev` 주소로 열립니다. 고칠 때는 같은 프로젝트에 새로 업로드하면 됩니다.

## 명령어로 올리기 (선택)
```
npx wrangler pages deploy . --project-name hanmun-ime
```

## 구성
- `index.html` — 입력기 전체 (엔진·사전 데이터 포함)
- `fonts/` — Pretendard(한글·로마자 부분 글꼴), Pretendard JP Variable(한자), `OFL.txt`
- `_headers` — 글꼴 파일 장기 캐시 설정 (Cloudflare Pages 전용)
- `LICENSE` — 입력기 코드 MIT 라이선스
- `THIRD_PARTY_NOTICES.txt` — 사전 데이터·글꼴 출처와 라이선스
