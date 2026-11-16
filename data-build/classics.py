import json,re,collections,pickle,itertools
U='/mnt/user-data/uploads/왕대별/'
ly=json.load(open(U+'lunyu_wikisource.json',encoding='utf-8-sig'))
pages=sorted(ly['query']['pages'].values(),key=lambda p:p['pageid'])
def wtext(p):
    r=p['revisions'][0]; s=r.get('slots',{}).get('main',r)
    return s.get('*') or s.get('content') or ''
def clean(w):
    w=re.sub(r'<!--[\s\S]*?-->','',w); w=re.sub(r'\{\{[\s\S]*?\}\}','',w); w=re.sub(r'<ref[\s\S]*?</ref>','',w); w=re.sub(r'<[^>]+>','',w); w=re.sub(r'-\{([^}]*)\}-',r'\1',w)
    out=[]
    for s in w.split('\n'):
        s=s.strip()
        if not s or s[0] in '*|=!{}[（' or s.startswith("'''") or 'http' in s or '錄音' in s: continue
        s=re.sub(r"\[\[(?:[^\]|]*\|)?([^\]]*)\]\]",r'\1',s).replace("'''",'').replace("''",'')
        if re.search('[一-鿿]',s): out.append(s)
    return out
LY=[l for p in pages for l in clean(wtext(p))]
mz=json.load(open(U+'mengzi_wikisource.json',encoding='utf-8-sig'))
MZ=[l for p in sorted(mz['query']['pages'].values(),key=lambda p:p['pageid']) for l in clean(wtext(p))]
HJ='[㐀-鿿豈-﫿]+'
def segs(lines): return [s for l in lines for s in re.findall(HJ,l)]
ls,ms=segs(LY),segs(MZ)
lines=[re.findall(HJ,l) for l in LY+MZ]
print('論語 chars',sum(map(len,ls)),'孟子 chars',sum(map(len,ms)),'clauses',len(ls),len(ms))
print(LY[0][:60]); print(LY[-1][:60])
pickle.dump((ls,ms),open('classics.pkl','wb')); pickle.dump(lines,open('classics_lines.pkl','wb'))
