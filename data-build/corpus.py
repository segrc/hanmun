import json,re,glob,collections,time
t=time.time()
HJ=re.compile(r'[㐀-鿿豈-﫿]+')
uni=collections.Counter(); bi=collections.Counter(); ng={2:collections.Counter(),3:collections.Counter(),4:collections.Counter()}
docs=0; total=0
for f in glob.glob('/mnt/user-data/uploads/왕대별/*.jsonl'):
    for line in open(f,encoding='utf-8'):
        try: d=json.loads(line)
        except: continue
        txt=d.get('text') or ''
        docs+=1
        for seg in HJ.findall(txt):
            total+=len(seg); uni.update(seg)
            for n in (2,3,4):
                for i in range(len(seg)-n+1): ng[n][seg[i:i+n]]+=1
print('docs',docs,'chars',total,'uniq',len(uni),'time',round(time.time()-t,1))
import pickle; pickle.dump((uni,ng),open('/home/claude/ime/corpus.pkl','wb'))
print(uni.most_common(40))
for n in (2,3,4): print(n,len(ng[n]),ng[n].most_common(25))
