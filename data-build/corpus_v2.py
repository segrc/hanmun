# 실록·승정원일기 어구 사전 v2 (corpus_words.json, corpus_stats.json)
#  - 원자료의 인명·지명·서명 표시(<ner_*>)를 써서, 인명 등은 '그 이름 그대로'만 남기고
#    이름에 왈·어조사·다른 글자가 붙은 조각(昌集曰, 書元斗杓 …)은 세지 않는다
#  - 여러 왕대에 고루 나오는 어구만 남긴다 (한 표본에만 많은 爲假注書 같은 상투구 제외)
#  - 앞뒤에 허사만 하나 더 붙은 조각은 버린다 (爲+假注書, 不可+也)
# 사용: python3 corpus_v2.py <왕대별 폴더> <web/src 폴더>
import json, re, sys, os, glob, math, collections

SRC = sys.argv[1] if len(sys.argv) > 1 else '/mnt/user-data/uploads/왕대별'
OUT = sys.argv[2] if len(sys.argv) > 2 else os.path.join(os.path.dirname(__file__), '..', 'web', 'src')
FILES = sorted(glob.glob(os.path.join(SRC, '실록_[0-9]*.jsonl')) + glob.glob(os.path.join(SRC, '승정원일기_*년.jsonl')))
HJ = re.compile(r'[㐀-鿿豈-﫿]+')
ENT = re.compile(r'<(ner_[a-z]+)\d*>(.*?)</ner_[a-z]+\d*>')
TAG = re.compile(r'<[^>]+>')

def docs(path):
    with open(path, encoding='utf-8') as f:
        for line in f:
            try: d = json.loads(line)
            except Exception: continue
            c = d.get('content') or {}
            yield c.get('body_xml') or d.get('text') or ''

def split(x):
    """본문 → (보통 글 조각들, 개체명 목록). 개체명 자리는 끊어서 n-gram 이 넘지 않게 한다."""
    ents = [(t, ''.join(HJ.findall(TAG.sub('', v)))) for t, v in ENT.findall(x)]
    plain = TAG.sub('', ENT.sub('|', x))
    return HJ.findall(plain), ents

S = len(FILES)
names = [os.path.basename(f) for f in FILES]
print('sources', S, names, flush=True)

# 1단계: 글자·2자 (출처별)
uni = collections.Counter()
ng = {n: [collections.Counter() for _ in range(S)] for n in (2, 3, 4)}
ent = collections.defaultdict(collections.Counter)   # 종류 → 이름 → 횟수
allbi = collections.Counter()                         # 개체명 포함 전체 2자 (문맥 순위용)
for si, f in enumerate(FILES):
    for x in docs(f):
        segs, es = split(x)
        for t, v in es:
            if 2 <= len(v) <= 5: ent[t][v] += 1
        full = HJ.findall(TAG.sub('', x))
        for s in full:
            uni.update(s)
            for i in range(len(s) - 1): allbi[s[i:i+2]] += 1
        for s in segs:
            for i in range(len(s) - 1): ng[2][si][s[i:i+2]] += 1
    print('pass1', names[si], flush=True)
tot = {2: sum(ng[2], collections.Counter())}
MINC = {2: 40, 3: 30, 4: 25}
# 2·3단계: 앞뒤 부분이 충분히 잦은 것만 3자·4자로 센다 (메모리 절약)
for n in (3, 4):
    prev = {w for w, k in tot[n-1].items() if k >= MINC[n-1]}
    for si, f in enumerate(FILES):
        c = ng[n][si]
        for x in docs(f):
            for s in split(x)[0]:
                for i in range(len(s) - n + 1):
                    w = s[i:i+n]
                    if w[:-1] in prev and w[1:] in prev: c[w] += 1
    tot[n] = sum(ng[n], collections.Counter())
    print('pass', n, len(tot[n]), flush=True)

def cnt(w): return uni[w] if len(w) == 1 else tot[len(w)][w]
def spread(w):
    n = len(w); per = [ng[n][si][w] for si in range(S)]
    return sum(1 for k in per if k > 0), max(per) / max(1, sum(per))

cand = {}
for n in (2, 3, 4):
    for w, k in tot[n].items():
        if k < MINC[n]: continue
        g = math.sqrt(k / cnt(w[:-1]) * k / cnt(w[1:]))
        if g < 0.12 and k < 1500: continue
        nsrc, top = spread(w)
        if nsrc < max(3, S * 6 // 10) or top > 0.5: continue   # 여러 왕대에 고루
        cand[w] = k
longer = collections.defaultdict(int)
for w, k in cand.items():
    if len(w) >= 3:
        longer[w[:-1]] = max(longer[w[:-1]], k); longer[w[1:]] = max(longer[w[1:]], k)
kept = {w: k for w, k in cand.items() if longer.get(w, 0) < 0.85 * k}
HEAD, TAIL = set('爲以而之於與則乃亦且又及其'), set('曰也矣之者乎焉哉而以於')
kept = {w: k for w, k in kept.items()
        if not (len(w) >= 3 and ((w[0] in HEAD and w[1:] in kept) or (w[-1] in TAIL and w[:-1] in kept)))}
# 허사로 시작·끝나는 3자 이상 조각(之勢矣, 而終無, 嬪御之), 숫자만 있는 것(七百七十) 제외
HEAD2, TAIL2 = set('之而於以則乃亦且又及其與所'), set('之而以於其與所也矣焉乎哉耳')
NUM = set('一二三四五六七八九十百千萬')
kept = {w: k for w, k in kept.items()
        if not (len(w) >= 3 and (w[0] in HEAD2 or w[-1] in TAIL2)) and not (w[0] == '之') and not set(w) <= NUM}
LIMIT = int(os.environ.get('KW_LIMIT', '4600'))
kept = dict(sorted(kept.items(), key=lambda x: -x[1])[:LIMIT])
print('phrases', len(cand), '->', len(kept), flush=True)

# 개체명: 이름 그대로만, 자주 나오는 것만
names_kept = {}
for t, lim, mn in (('ner_person', 100, 3), ('ner_place', 30, 2), ('ner_book', 15, 2)):   # 인명은 성까지 갖춘 3자 이상
    for v, k in ent[t].items():
        if k >= lim and len(v) >= mn: names_kept[v] = max(names_kept.get(v, 0), k)
print('entities', len(names_kept), flush=True)

# 독음 (corpus3.py 와 같은 방법)
H = OUT
chars = json.load(open(os.path.join(H, 'chars_ranked.json'))); words = json.load(open(os.path.join(H, 'words.json')))
cl = json.load(open(os.path.join(H, 'classical.json'))); ksr = json.load(open(os.path.join(H, 'ksr.json')))
rd = collections.defaultdict(set)
for r, s in chars.items():
    for ch in s: rd[ch].add(r)
def dec(s): c = ord(s) - 0xAC00; return c // 588, (c % 588) // 28, c % 28
def comp(i, m, f): return chr(0xAC00 + i * 588 + m * 28 + f)
IY = {2, 6, 7, 12, 17, 20}
def dueum(s):
    i, m, f = dec(s)
    if i == 5: return comp(11, m, f) if m in IY else comp(2, m, f)
    if i == 2 and m in IY: return comp(11, m, f)
    return s
obs = collections.defaultdict(collections.Counter)
for src in (words, cl['cw']):
    for h, zs in src.items():
        for z in zs:
            for i, ch in enumerate(z):
                if i < len(h) and (i > 0 or h[i] == dueum(h[i])): obs[ch][h[i]] += 1
def prim(ch):
    if obs[ch]: return obs[ch].most_common(1)[0][0]
    if ch in ksr: return ksr[ch][0]
    rs = sorted(rd.get(ch, []))
    return rs[0] if rs else None
def reading(w):
    rs = []
    for i, ch in enumerate(w):
        r = prim(ch)
        if r is None: return None
        if ch == '不' and i + 1 < len(w):
            nx = prim(w[i+1]) or ''
            r = '부' if nx and dec(nx)[0] in (3, 12) else '불'
        if i and r in ('렬', '률') and dec(rs[-1])[2] in (0, 4):   # 時烈 → 시열, 規律 → 규율
            r = '열' if r == '렬' else '율'
        rs.append(r)
    h = ''.join(rs)
    return dueum(h[0]) + h[1:]

out = {}
for w, k in list(kept.items()) + [(v, k) for v, k in names_kept.items() if v not in kept]:
    h = reading(w)
    if h: out.setdefault(h, []).append((k, w))
res = {h: [w for k, w in sorted(v, reverse=True)] for h, v in sorted(out.items(), key=lambda x: -max(k for k, w in x[1]))}
res.setdefault('실록', []).append('實錄') if '實錄' not in res.get('실록', []) else None
json.dump(res, open(os.path.join(H, 'corpus_words.json'), 'w'), ensure_ascii=False, separators=(',', ':'))
print('entries', sum(len(v) for v in res.values()), 'keys', len(res))
for h in ['승지', '상왈', '계왈', '대왈', '창집왈', '위가주서', '가주서', '불가', '부득', '이위', '도승지', '송시열', '전교왈', '실록']:
    print(h, res.get(h))

# 글자 빈도·뒤 글자 — 논어·맹자(classics.py 가 만든 classics.pkl)는 5배로 더한다
import pickle
cu, cb = collections.Counter(), collections.Counter()
cp = os.environ.get('CLASSICS_PKL', os.path.join(os.path.dirname(__file__), 'classics.pkl'))
if os.path.exists(cp):
    VM = dict(zip('為既教鄉顏眾舉吿恆產飲鄰毀弒雞尚啟衞絕強茲讎卻慎偽清汙', '爲旣敎鄕顔衆擧告恒産飮隣毁弑鷄尙啓衛絶强玆讐却愼僞淸汚'))
    ls, ms = pickle.load(open(cp, 'rb'))
    for t in ls + ms:
        t = ''.join(VM.get(c, c) for c in t)
        cu.update(t)
        for i in range(len(t) - 1): cb[t[i:i+2]] += 1
U = collections.Counter(uni)
for k, v in cu.items(): U[k] += 5 * v
B = collections.Counter({k: v for k, v in allbi.items() if v >= 3})
for k, v in cb.items(): B[k] += 5 * v
F_TOP, F_MIN = int(os.environ.get('F_TOP', '30')), int(os.environ.get('F_MIN', '200'))
fol = collections.defaultdict(list)
for w, k in B.items(): fol[w[0]].append((k, w[1]))
json.dump({'u': {c: n for c, n in U.items() if n >= 8},
           'f': {a: ''.join(b for k, b in sorted(v, reverse=True)[:F_TOP]) for a, v in fol.items() if U.get(a, 0) >= F_MIN}},
          open(os.path.join(H, 'corpus_stats.json'), 'w'), ensure_ascii=False, separators=(',', ':'))
print('done')
