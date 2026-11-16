import json,re
c=json.load(open('chars_ranked.json'))
rd={}
for r,s in c.items():
    for ch in s: rd.setdefault(ch,set()).add(r)
P={}; badp=[]
for line in open('classical_prio.txt'):
    line=line.strip()
    if ':' not in line: continue
    r,s=line.split(':')
    for ch in s:
        if r not in rd.get(ch,()): badp.append(f'{r}:{ch}({"/".join(sorted(rd.get(ch,[])))})'); continue
        if ch not in P.get(r,''): P[r]=P.get(r,'')+ch
H=re.compile(r'^([가-힣]+)([㐀-鿿豈-﫿]+)$')
CW={}; bad=[]
for t in open('classical_words.txt').read().split():
    m=H.match(t)
    if not m: continue
    h,z=m.groups()
    if len(h)!=len(z) or not all(h[i] in rd.get(z[i],()) for i in range(len(h))):
        bad.append(t+' '+' '.join(f'{z[i]}:{"/".join(sorted(rd.get(z[i],[])))}' for i in range(min(len(h),len(z))) if h[i] not in rd.get(z[i],()))); continue
    L=CW.setdefault(h,[]); 
    if z not in L: L.append(z)
print('badp',badp); print('bad',bad); print(len(P),len(CW))
json.dump({'p':P,'cw':CW},open('classical.json','w'),ensure_ascii=False,separators=(',',':'))
