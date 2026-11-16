import json,pickle,math,collections
uni,ng=pickle.load(open('corpus.pkl','rb'))
def c(s): return uni[s] if len(s)==1 else ng[len(s)][s]
MINC={2:40,3:25,4:20}
cand={}
for n in (2,3,4):
    for w,k in ng[n].items():
        if k<MINC[n]: continue
        f=k/c(w[:-1]); b=k/c(w[1:])
        g=math.sqrt(f*b)
        if g<0.12 and k<400: continue
        cand[w]=k
# subsumption: drop if a longer kept-candidate (n+1) explains >=85% of count
longer=collections.defaultdict(int)
for w,k in cand.items():
    if len(w)>=3:
        longer[w[:-1]]=max(longer[w[:-1]],k); longer[w[1:]]=max(longer[w[1:]],k)
kept={w:k for w,k in cand.items() if longer.get(w,0) < 0.85*k}
print(len(cand),len(kept))
top=sorted(kept.items(),key=lambda x:-x[1])
for n in (2,3,4): print(n,[w for w,k in top if len(w)==n][:60])
pickle.dump(kept,open('phr.pkl','wb'))
