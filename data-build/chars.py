import json, unicodedata
D='/usr/share/texlive/texmf-dist/tex/xetex/xetexko/'
def tab(f,base):
    L=open(D+f).read().split()
    return {chr(base+i):chr(int(v)) for i,v in enumerate(L) if int(v)>0}
R={}
for f,b in [('hanja_hangul.tab',0x4E00),('hanjaexa_hangul.tab',0x3400),('hanjacom_hangul.tab',0xF900)]:
    R.update(tab(f,b))
# hangul decomposition
def dec(s):
    c=ord(s)-0xAC00; return c//588, (c%588)//28, c%28
def comp(i,m,f): return chr(0xAC00+i*588+m*28+f)
# 두음 reverse: initial ㅇ(11) or ㄴ(2) -> ㄹ(5) / ㄴ(2)
IY={2,6,7,12,17,20}  # ㅑ ㅕ ㅖ ㅛ ㅠ ㅣ  (ㅒ? rarely)  vowel idx: ㅏ0 ㅐ1 ㅑ2 ㅒ3 ㅓ4 ㅔ5 ㅕ6 ㅖ7 ㅗ8 ㅘ9 ㅙ10 ㅚ11 ㅛ12 ㅜ13 ㅝ14 ㅞ15 ㅟ16 ㅠ17 ㅡ18 ㅢ19 ㅣ20
def orig_forms(s):
    i,m,f=dec(s); out={s}
    if i==11 and m in IY: out|={comp(5,m,f),comp(2,m,f)}   # 이->리,니
    if i==2: out.add(comp(5,m,f))                          # 나->라
    return out
def dueum(s):
    i,m,f=dec(s)
    if i==5: return comp(11,m,f) if m in IY else comp(2,m,f)
    if i==2 and m in IY: return comp(11,m,f)
    return s
# KS X 1001 hanja, in order, to recover original reading
ks=[]
for a in range(0xCA,0xFE):
    for b in range(0xA1,0xFF):
        try: ks.append(bytes([a,b]).decode('euc_kr'))
        except: pass
seq=[(c,R[c]) for c in ks if c in R]
cands=[sorted(orig_forms(r)) for c,r in seq]
INF=10**9
best=[{} for _ in seq]; back=[{} for _ in seq]
for i,(c,r) in enumerate(seq):
    for x in cands[i]:
        cost=0 if x==r else 1
        if i==0: best[i][x]=cost; continue
        v,y=min((v+(0 if y<=x else 100),y) for y,v in best[i-1].items()); best[i][x]=v+cost; back[i][x]=y
x=min(best[-1],key=best[-1].get); KSR={}
for i in range(len(seq)-1,-1,-1):
    KSR.setdefault(seq[i][0],[]).append(x)
    if i: x=back[i][x]
print('ks',len(ks),len(KSR))
def members(enc,ranges):
    s=set()
    for a,b0,b1 in ranges:
        pass
    return s
def codec_set(enc,hi,lo_rng):
    s=[]
    for a in hi:
        for b in lo_rng:
            try: s.append(bytes([a,b]).decode(enc))
            except: pass
    return s
big5_1=codec_set('big5',range(0xA4,0xC7),list(range(0x40,0x7F))+list(range(0xA1,0xFF)))
jis1=codec_set('euc_jp',range(0xB0,0xD0),range(0xA1,0xFF))
gb1=codec_set('gb2312',range(0xB0,0xD8),range(0xA1,0xFF))
B5={c:i for i,c in enumerate(big5_1)}; J1=set(jis1); G1=set(gb1)
print(len(big5_1),len(jis1),len(gb1))
# entries: reading -> list of (char, score)
idx={}
def add(r,c,sc):
    d=idx.setdefault(r,{})
    d[c]=max(d.get(c,-1),sc)
for c,rs in KSR.items():
    u=unicodedata.normalize('NFC',c)
    base=10+(4 if u in B5 else 0)+(3 if u in J1 else 0)+(2 if u in G1 else 0)
    for r in rs:
        add(r,u,base); add(dueum(r),u,base)
for c,r in R.items():
    u=unicodedata.normalize('NFC',c)
    if u in KSR or c in KSR: continue
    sc=(4 if u in B5 else 0)+(3 if u in J1 else 0)+(2 if u in G1 else 0)
    add(r,u,sc)
    for o in orig_forms(r): 
        if o!=r: add(o,u,sc-1 if sc else -2)
out={}
for r,d in idx.items():
    out[r]=''.join(sorted(d,key=lambda c:(-d[c],B5.get(c,99999),ord(c))))
json.dump(out,open('chars.json','w'),ensure_ascii=False,separators=(',',':'))
json.dump({k:v for k,v in KSR.items()},open('ksr.json','w'),ensure_ascii=False)
for t in '한 자 학 교 대 국 민 리 이 룡 용 락 악 년 연 수 정 인 사 일'.split(): print(t,out.get(t,'')[:14])
print(len(out), sum(len(v) for v in out.values()))
