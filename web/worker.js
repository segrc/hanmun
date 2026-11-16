// 빠른 한문 입력기 — GitHub 저장소(segrc/hanmun)의 web/deploy 를 그대로 내보낸다.
// main 에 push 하면 몇 분 안에 사이트에 반영된다. GitHub 이 응답하지 않으면 jsDelivr 사본을 쓴다.
// (Cloudflare Worker "hanmun" 에 배포된 코드와 같다)
const SOURCES = [
  'https://raw.githubusercontent.com/segrc/hanmun/main/web/deploy',
  'https://cdn.jsdelivr.net/gh/segrc/hanmun@main/web/deploy',
];
const TYPES = { html: 'text/html; charset=utf-8', svg: 'image/svg+xml', woff2: 'font/woff2', txt: 'text/plain; charset=utf-8' };
export default {
  async fetch(request) {
    if (request.method !== 'GET' && request.method !== 'HEAD') return new Response('Method Not Allowed', { status: 405 });
    const url = new URL(request.url);
    let p = decodeURIComponent(url.pathname);
    if (p.endsWith('/')) p += 'index.html';
    const ext = (p.split('.').pop() || '').toLowerCase();
    if (p.includes('..') || !TYPES[ext]) return new Response('Not found', { status: 404 });
    const font = ext === 'woff2';
    for (const base of SOURCES) {
      try {
        const r = await fetch(base + p, { cf: { cacheTtl: font ? 86400 : 120, cacheEverything: true } });
        if (r.status === 404) return new Response('Not found', { status: 404 });
        if (!r.ok) continue;
        const h = new Headers({
          'content-type': TYPES[ext],
          'cache-control': font ? 'public, max-age=31536000, immutable' : 'public, max-age=0, must-revalidate',
          'x-content-type-options': 'nosniff',
          'referrer-policy': 'strict-origin-when-cross-origin',
        });
        if (font) h.set('access-control-allow-origin', '*');
        return new Response(request.method === 'HEAD' ? null : r.body, { status: 200, headers: h });
      } catch (e) {}
    }
    return new Response('잠시 후 다시 시도해 주세요.', { status: 503, headers: { 'content-type': 'text/plain; charset=utf-8' } });
  },
};
