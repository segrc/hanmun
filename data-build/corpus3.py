import json,pickle,collections
uni,ng=pickle.load(open('corpus.pkl','rb')); kept=pickle.load(open('phr.pkl','rb'))
chars=json.load(open('chars_ranked.json')); words=json.load(open('words.json')); cl=json.load(open('classical.json')); ksr=json.load(open('ksr.json'))
rd=collections.defaultdict(set)
for r,s in chars.items():
    for ch in s: rd[ch].add(r)
def dec(s): c=ord(s)-0xAC00; return c//588,(c%588)//28,c%28
def comp(i,m,f): return chr(0xAC00+i*588+m*28+f)
IY={2,6,7,12,17,20}
def dueum(s):
    i,m,f=dec(s)
    if i==5: return comp(11,m,f) if m in IY else comp(2,m,f)
    if i==2 and m in IY: return comp(11,m,f)
    return s
# observed non-initial readings from my dictionaries
obs=collections.defaultdict(collections.Counter)
for src in (words,cl['cw']):
    for h,zs in src.items():
        for z in zs:
            for i,ch in enumerate(z):
                if i>0 or h[i]==dueum(h[i]) and h[i] not in ('이','여','연','영','예','요','용','유','육','윤','율','융','익','인','일','임','입','나','낙','난','남','납','낭','내','냉','노','녹','논','농','뇌','누','능'): obs[ch][h[i]]+=1
def prim(ch):
    if obs[ch]: return obs[ch].most_common(1)[0][0]
    for k in (ch,):
        if k in ksr: return ksr[k][0]
    rs=sorted(rd.get(ch,[]))
    return rs[0] if rs else None
SUR=set('李金朴鄭崔尹趙申洪權閔沈兪柳韓宋吳徐黃安張林任姜具南元成許睿蔡呂盧趙睦宋')
out={}; drop=0
for w,k in kept.items():
    if w[-1] in SUR and (w[:-1] in kept or len(w)>=3): drop+=1; continue
    if w[0] in SUR and len(w)==4 and w[1:] in kept: drop+=1; continue
    if len(w)>=3 and any(w[i] in SUR and w[:i] in kept for i in range(1,len(w))): drop+=1; continue
    rs=[]
    for i,ch in enumerate(w):
        r=prim(ch)
        if r is None: break
        if ch=='不' and i+1<len(w):
            nx=prim(w[i+1]) or ''
            r='부' if nx and dec(nx)[0] in (3,12) else '불'
        rs.append(r)
    else:
        h=''.join(rs); h=dueum(h[0])+h[1:]
        out.setdefault(h,[]).append((k,w))
res={h:[w for k,w in sorted(v,reverse=True)] for h,v in sorted(out.items(),key=lambda x:-max(k for k,w in x[1]))}
print('dropped',drop,'entries',sum(len(v) for v in res.values()))
for h in ['승지','영의정','불가','부득','하여','이위','지도','상왈','계왈','도승지','이조판서','이차진복','불가불','악','전교']:
    print(h,res.get(h))
json.dump(res,open('corpus_words.json','w'),ensure_ascii=False,separators=(',',':'))
# unigram freq rank + bigram followers
U={ch:n for ch,n in uni.items()}
FOL={}
for w,k in ng[2].items():
    if k>=3: FOL.setdefault(w[0],[]).append((k,w[1]))
fol={a:''.join(b for k,b in sorted(v,reverse=True)[:80]) for a,v in FOL.items() if U.get(a,0)>=30}
json.dump({'u':{ch:n for ch,n in U.items() if n>=3},'f':fol},open('corpus_stats.json','w'),ensure_ascii=False,separators=(',',':'))
import os; print(os.path.getsize('corpus_words.json'),os.path.getsize('corpus_stats.json'))
