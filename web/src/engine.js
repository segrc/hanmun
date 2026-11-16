/* HanjaEngine — 화면과 무관한 후보 엔진. 시스템 입력기로 옮길 때 이 부분만 재사용합니다.
   dict = { c: {읽기: "한자들(순위순)"}, w: {읽기: ["漢字", ...]} }
   user = { f: {"읽기\t한자": 횟수}, w: {읽기: ["漢字"]} }   */
const HanjaEngine = (() => {
  const IDX = new Map();   // 앞글자 색인: 사전별로 읽기를 가나다순 정렬해 두고 이진 탐색
  let C = {}, W = {}, X = {}, M = {}, P = {}, CW = {}, KW = {}, GW = {}, GSET = new Set(), U = {}, F = {};           // 글자, 단어, 가져온 단어, 뜻
  const isSyl = ch => ch >= '가' && ch <= '힣';
  const isJamo = ch => ch >= 'ㄱ' && ch <= 'ㅣ';
  function load(dict) { C = dict.c || {}; W = dict.w || {}; P = dict.p || {}; CW = dict.cw || {}; KW = dict.kw || {}; GW = dict.gw || {}; GSET = new Set(Object.values(GW).flat()); U = dict.u || {}; F = dict.f || {}; Object.assign(M, dict.m || {}); IDX.clear(); }
  // 외부 사전(libhangul hanja.txt 형식: 읽기:한자:뜻) 병합
  function importText(text) {
    let n = 0;
    for (const line of text.split(/\r?\n/)) {
      if (!line || line[0] === '#') continue;
      const [r, z, m] = line.split(':');
      if (!r || !z || ![...r].every(isSyl)) continue;
      if ([...z].length === 1) {
        if (!(C[r] || '').includes(z)) C[r] = (C[r] || '') + z;
      } else {
        const L = X[r] || (X[r] = []);
        if (!L.includes(z) && !(W[r] || []).includes(z)) { L.push(z); n++; }
      }
      if (m && !M[z]) M[z] = m.trim();
    }
    IDX.delete(X);
    return n;
  }
  // 커서 앞 한글 묶음(변환 대상)을 찾는다: text의 [start, end)
  function findRun(text, caret, maxLen = 16) {
    let s = caret;
    while (s > 0 && caret - s < maxLen && (isSyl(text[s - 1]) || isJamo(text[s - 1]))) s--;
    // 끝에 자모만 있는 경우(조합 중) 그 앞까지만
    let e = caret;
    while (e > s && isJamo(text[e - 1])) e--;
    return e > s ? { start: s, end: e, text: text.slice(s, e) } : null;
  }
  function score(user, r, z) { return (user && user.f && user.f[r + '\t' + z]) || 0; }
  // run의 앞부분부터 길이가 긴 단어 → 첫 글자 순서로 후보를 만든다
  function candidates(run, user, opts = {}) {
    const out = [], seen = new Set();
    const exactOnly = opts.exact, han = opts.classical;
    const uw = r => (user && user.w && user.w[r]) || [];
    for (let k = run.length; k >= 1; k--) {
      if (exactOnly && k !== run.length && k !== 1) continue;
      const r = run.slice(0, k);
      let list;
      if (k === 1) list = han ? classicalChars(r, opts.prev) : [...(C[r] || ''), ...uw(r)];
      else list = han ? [].concat(uw(r), CW[r] || [], KW[r] || [], GW[r] || [], W[r] || [], X[r] || []) : [].concat(uw(r), W[r] || [], CW[r] || [], GW[r] || [], KW[r] || [], X[r] || []);
      const base = list.filter((z, i) => list.indexOf(z) === i);
      const ranked = base.map((z, i) => ({ z, i, s: score(user, r, z) }))
        .sort((a, b) => (b.s - a.s) || (a.i - b.i));
      for (const { z, s } of ranked) {
        const key = k + z; if (seen.has(key)) continue; seen.add(key);
        out.push({ len: k, read: r, text: z, learned: s > 0, mean: M[z] || '', ctx: han && k === 1 && ctxHit(opts.prev, z) });
      }
    }
    return out;
  }
  // 한문 모드 글자 순위: 말뭉치 빈도 + 앞 글자 다음에 자주 오는 글자(문맥)
  function classicalChars(r, prev) {
    const all = [...new Set([...(P[r] || ''), ...(C[r] || '')])];
    const fol = (prev && F[prev]) || '', pr = P[r] || '';
    const sc = ch => { const fi = fol.indexOf(ch), u = U[ch] || 0;
      return Math.log(u + 1) + (fi >= 0 ? 4 + (80 - fi) / 16 : 0) + (pr.includes(ch) ? 1.5 : 0) + (u ? 0 : -50); };
    const S = new Map(all.map((ch, i) => [ch, sc(ch) - i * 1e-6]));
    return all.sort((a, b) => S.get(b) - S.get(a));
  }
  const ctxHit = (prev, ch) => !!(prev && F[prev] && F[prev].includes(ch));
  function sortedKeys(src, dyn) {
    let k = dyn ? null : IDX.get(src);
    if (!k) {
      const raw = Object.keys(src); k = raw.slice().sort();
      k.rank = new Map(raw.map((r, i) => [r, i]));   // 원래 순서 = 빈도순
      if (!dyn) IDX.set(src, k);
    }
    return k;
  }
  function lower(a, x) { let lo = 0, hi = a.length; while (lo < hi) { const m = (lo + hi) >> 1; if (a[m] < x) lo = m + 1; else hi = m; } return lo; }
  // 예측: 지금까지 친 음으로 시작하는 유명한 단어·구절
  function predict(run, user, han, max, gw) {
    const out = [], seen = new Set();
    const G = GW;
    const srcs = han ? [(user && user.w) || {}, CW, G, KW, W, X] : [(user && user.w) || {}, W, CW, G, KW, X];
    srcs.forEach((src, si) => {
      const keys = sortedKeys(src, si === 0);
      for (let i = lower(keys, run); i < keys.length && keys[i].startsWith(run); i++) {
        const r = keys[i]; if (r.length <= run.length) continue;
        src[r].forEach((z, i) => { if (GSET.has(z) && (run.length < 2 || run.length * 3 <= r.length)) return;   /* 경서 구절: 1/3 초과로 쳤을 때만 */ if (!seen.has(z)) { seen.add(z); out.push({ len: run.length, read: r, text: z, pred: true, learned: score(user, r, z) > 0, mean: M[z] || '', k: [-score(user, r, z), si, r.length > run.length + 4 ? 1 : 0, keys.rank.get(r) * 8 + i] }); } });
        if (out.length > 400) break;
      }
    });
    out.sort((a, b) => { for (let j = 0; j < 4; j++) if (a.k[j] !== b.k[j]) return a.k[j] - b.k[j]; return 0; });
    return out.slice(0, max);
  }
  function withPredictions(list, run, user, opts = {}) {
    if (opts.exact || opts.predict === false) return list;
    const n = run.length >= 2 ? 5 : 3, P2 = predict(run, user, opts.classical, n, opts.gwPredict);
    if (!P2.length) return list;
    const exactEnd = list.findIndex(c => c.len < run.length);   // 끝까지 맞는 단어 다음
    const at = run.length === 1 ? Math.min(2, list.length) : (exactEnd < 0 ? list.length : exactEnd);
    return list.slice(0, at).concat(P2, list.slice(at));
  }
  function learn(user, r, z) {
    user.f = user.f || {}; const k = r + '\t' + z;
    user.f[k] = (user.f[k] || 0) + 1;
  }
  function addWord(user, r, z) {
    user.w = user.w || {}; const L = user.w[r] || (user.w[r] = []);
    if (!L.includes(z)) L.unshift(z);
  }
  function format(r, z, mode) {
    return mode === 'zr' ? `${z}(${r})` : mode === 'rz' ? `${r}(${z})` : z;
  }
  function dueum(s) {
    const c = s.charCodeAt(0) - 0xAC00; if (c < 0 || c > 11171) return s;
    const i = Math.floor(c / 588), m = Math.floor((c % 588) / 28), f = c % 28, IY = [2, 6, 7, 12, 17, 20].includes(m);
    const mk = ni => String.fromCharCode(0xAC00 + ni * 588 + m * 28 + f);
    if (i === 5) return IY ? mk(11) : mk(2);
    if (i === 2 && IY) return mk(11);
    return s;
  }
  const stats = () => ({ readings: Object.keys(C).length, words: Object.values(W).reduce((a, b) => a + b.length, 0), imported: Object.values(X).reduce((a, b) => a + b.length, 0) });
  return { load, dueum, predict, withPredictions, importText, findRun, candidates, learn, addWord, format, stats, isSyl };
})();
if (typeof module !== 'undefined') module.exports = HanjaEngine;
