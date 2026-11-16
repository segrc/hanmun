# web/src 의 JSON 사전들을 입력기 DLL에 넣을 한 개의 텍스트 사전(hanmun.dict, UTF-8)으로 합친다.
# 줄 형식:  구역<TAB>키<TAB>값      (목록 값은 '|'로 구분, 원래 순서 = 빈도 순서)
#   c  읽기 -> 글자들            p  읽기 -> 한문 우선 글자들      f  글자 -> 뒤에 자주 오는 글자들
#   u  글자 -> 말뭉치 빈도        m  글자 -> 훈음
#   w  현대 한자어   cw 고전 어구·이두   kw 실록·승정원일기 어구   gw 논어·맹자 구절
import json, os, sys

src = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..', '..', 'web', 'src')
out = sys.argv[2] if len(sys.argv) > 2 else 'hanmun.dict'
J = lambda n: json.load(open(os.path.join(src, n), encoding='utf-8'))

c = J('chars_ranked.json'); w = J('words.json'); cl = J('classical.json'); cs = J('corpus_stats.json')
kw = J('corpus_words.json'); gw = J('classics_words.json'); m = J('meanings.json')

lines = []
def put(sec, d, lst=False):
    for k, v in d.items():
        if lst:
            v = [x for x in v if '|' not in x]
            if not v: continue
            v = '|'.join(v)
        v = str(v)
        assert '\t' not in k + v and '\n' not in k + v
        lines.append(f'{sec}\t{k}\t{v}')

put('c', c); put('p', cl.get('p', {})); put('f', cs['f']); put('u', cs['u']); put('m', m)
put('w', w, True); put('cw', cl.get('cw', {}), True); put('kw', kw, True); put('gw', gw, True)
data = ('\n'.join(lines) + '\n').encode('utf-8')
open(out, 'wb').write(data)
print(f'{out}: {len(lines)} lines, {len(data)} bytes')
