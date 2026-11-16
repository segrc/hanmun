import json,re,math,unicodedata
chars=json.load(open('chars.json'))
rd={}  # char -> set of readings
for r,s in chars.items():
    for c in s: rd.setdefault(c,set()).add(r)
H=re.compile(r'^([가-힣]+)([㐀-鿿豈-﫿]+)$')
words={}; bad=[]
for f in ['words1.txt','words2.txt','words3.txt','words4.txt','words5.txt']:
    for t in open(f).read().split():
        m=H.match(t)
        if not m: continue
        h,z=m.groups()
        if len(h)!=len(z): bad.append(t); continue
        ok=all(h[i] in rd.get(z[i],()) for i in range(len(h)))
        if not ok: bad.append(t+' '+','.join(f'{z[i]}:{"/".join(sorted(rd.get(z[i],[])))}' for i in range(len(h)) if h[i] not in rd.get(z[i],()))); continue
        L=words.setdefault(h,[])
        if z not in L: L.append(z)
print('bad',len(bad)); print('\n'.join(bad))
print('words',len(words),sum(len(v) for v in words.values()))
json.dump(words,open('words.json','w'),ensure_ascii=False,separators=(',',':'))
EXTRA={'則':['즉'],'便':['편'],'六':['유'],'十':['시'],'數':['숫']}
for c,rs in EXTRA.items():
    for r in rs:
        rd.setdefault(c,set()).add(r)
        if c not in chars.get(r,''): chars[r]=chars.get(r,'')+c
for t in bad:
    t=t.split()[0]; m=H.match(t)
    if not m: continue
    h,z=m.groups()
    if len(h)==len(z) and all(h[i] in rd.get(z[i],()) for i in range(len(h))):
        words.setdefault(h,[]).append(z)
# char frequency from words (by reading-specific usage)
cnt={}
for h,zs in words.items():
    for z in zs:
        for i in range(len(h)):
            k=(h[i],z[i]); cnt[k]=cnt.get(k,0)+1
            # also count 두음 forms: first-syllable reading counts for orig too
from collections import defaultdict
tot={}
for (rr,cc),v in cnt.items(): tot[cc]=tot.get(cc,0)+v
out={}
for r,s in chars.items():
    pos={c:i for i,c in enumerate(s)}
    # aggregate count across readings for char (commonness) + reading-specific
    out[r]=''.join(sorted(s,key=lambda c:(-(cnt.get((r,c),0)*3+tot.get(c,0)), pos[c])))
json.dump(out,open('chars_ranked.json','w'),ensure_ascii=False,separators=(',',':'))
json.dump(words,open('words.json','w'),ensure_ascii=False,separators=(',',':'))
for t in '한 자 학 교 대 국 민 리 이 룡 용 락 악 년 연 수 정 인 사 일 편 변 시 기 고 전'.split(): print(t,out.get(t,'')[:12])
