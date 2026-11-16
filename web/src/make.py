import json
c=json.load(open('chars_ranked.json')); w=json.load(open('words.json'))
cl=json.load(open('classical.json')); cs=json.load(open('corpus_stats.json'))
cl['kw']=json.load(open('corpus_words.json')); cl['u']=cs['u']; cl['f']=cs['f']
cl['gw']=json.load(open('classics_words.json')); cl['m']=json.load(open('meanings.json'))
d=json.dumps({'c':c,'w':w,**cl},ensure_ascii=False,separators=(',',':')).replace('</','<\\/')
s=open('page.html').read().replace('/*DICT*/',d).replace('/*ENGINE*/',open('engine.js').read().replace("if (typeof module !== 'undefined') module.exports = HanjaEngine;",''))
open('hanja-ime.html','w').write(s); print(len(s.encode()))
