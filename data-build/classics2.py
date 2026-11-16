import json,pickle,collections,itertools
exec(open('corpus3.py').read().split("SUR=")[0])   # reuse rd, dec, comp, dueum, prim, ksr, obs
ls,ms=pickle.load(open('classics.pkl','rb'))
VM=dict(zip('為既教鄉顏眾舉吿恆產飲鄰毀弒雞尚啟衞絕強茲讎卻慎偽清汙','爲旣敎鄕顔衆擧告恒産飮隣毁弑鷄尙啓衛絶强玆讐却愼僞淸汚'))
norm=lambda s:''.join(VM.get(c,c) for c in s)
ls=[norm(s) for s in ls]; ms=[norm(s) for s in ms]
ALT={'說':['설','열','세'],'樂':['악','락','요'],'惡':['악','오'],'見':['견','현'],'易':['역','이'],'行':['행','항'],'復':['복','부'],
 '數':['수','삭'],'識':['식','지'],'度':['도','탁'],'更':['경','갱'],'率':['솔','률'],'降':['강','항'],'殺':['살','쇄'],'食':['식','사'],
 '省':['성','생'],'女':['녀','여'],'否':['부','비'],'畫':['화','획'],'宿':['숙','수'],'北':['북','배'],'參':['참','삼'],'辟':['벽','피'],'屬':['속','촉'],'塞':['색','새'],'告':['고','곡'],'厲':['려'],'冉':['염'],'滕':['등']}
def readings(w):
    opts=[]
    for i,ch in enumerate(w):
        if ch=='不' and i+1<len(w):
            nx=prim(w[i+1]) or ''
            o=['부'] if nx and dec(nx)[0] in (3,12) else ['불']
        elif ch in ALT: o=ALT[ch]
        else:
            r=prim(ch)
            if r is None: return []
            o=[r]
        opts.append(o)
    out=[]
    for combo in itertools.islice(itertools.product(*opts),8):
        h=''.join(combo); h=dueum(h[0])+h[1:]
        if h not in out: out.append(h)
    return out
lines=[[norm(x) for x in L] for L in pickle.load(open('classics_lines.pkl','rb'))]
pairs=[]
for L in lines:
    for i in range(len(L)-1):
        j=L[i]+L[i+1]
        if len(j)<=12 and min(len(L[i]),len(L[i+1]))<=2: pairs.append(j)   # 한쪽 구가 1~2자일 때만
print('pairs',len(pairs))
GW={}; order=[]; seen=set(); miss=0
for s in ls+ms+pairs:
    if len(s)<2 or len(s)>14 or s in seen: continue
    seen.add(s); rs=readings(s)
    if not rs: miss+=1; continue
    for h in rs: GW.setdefault(h,[]).append(s)
print('clauses',len(seen),'keys',len(GW),'unreadable',miss)
for t in ['학이시습지','불역열호','불역락호','왕하필왈리','역유인의이이의','인자요산','온고이지신','노인노','호연지기']: print(t,GW.get(t))
json.dump(GW,open('classics_words.json','w'),ensure_ascii=False,separators=(',',':'))
# bigram/unigram merge (classics weighted x5)
uni,ng=pickle.load(open('corpus.pkl','rb'))
cu=collections.Counter(); cb=collections.Counter()
for s in ls+ms:
    cu.update(s)
    for i in range(len(s)-1): cb[s[i:i+2]]+=1
B=collections.Counter({k:v for k,v in ng[2].items() if v>=3})
for k,v in cb.items(): B[k]+=5*v
Uall=collections.Counter(uni)
for k,v in cu.items(): Uall[k]+=5*v
FOL={}
for w,k in B.items(): FOL.setdefault(w[0],[]).append((k,w[1]))
fol={a:''.join(b for k,b in sorted(v,reverse=True)[:80]) for a,v in FOL.items() if Uall.get(a,0)>=30}
json.dump({'u':{c:n for c,n in Uall.items() if n>=3},'f':fol},open('corpus_stats.json','w'),ensure_ascii=False,separators=(',',':'))
import os; print(os.path.getsize('classics_words.json'),os.path.getsize('corpus_stats.json'))
