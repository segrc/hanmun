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
# 구 묶음(한쪽이 1~2자)은 한 문장 안에서만: 문장 끝(。？！；)을 넘지 않게
def sentences(l):
    out=[]; carry=[]
    parts=re.split('([。？！；?!;])',l)
    for k in range(0,len(parts),2):
        sent=parts[k]; end=parts[k+1] if k+1<len(parts) else ''
        cl=carry+re.findall(HJ,sent); carry=[]
        if not cl: continue
        # '叟！' '王！' 같은 부름말(느낌표 앞 1~2자)은 뒤 문장에 붙인다: 王曰：「叟！不遠千里而來 → [王曰] [叟, 不遠千里而來…]
        if end in '！!' and len(cl[-1])<=2:
            carry=[cl.pop()]
            if not cl: continue
        out.append(cl)
    if carry: out.append(carry)
    return out
lines=[c for l in LY+MZ for c in sentences(l)]
print('論語 chars',sum(map(len,ls)),'孟子 chars',sum(map(len,ms)),'clauses',len(ls),len(ms))
print(LY[0][:60]); print(LY[-1][:60])
pickle.dump((ls,ms),open('classics.pkl','wb')); pickle.dump(lines,open('classics_lines.pkl','wb'))
