# 아티팩트용 hanja-ime.html → Cloudflare 배포용 deploy/index.html (+ 방문 통계 스니펫)
import shutil, subprocess
SNIP = """<!-- Cloudflare Web Analytics --><script type='module' src='https://static.cloudflareinsights.com/beacon.min.js' data-cf-beacon='{"token": "e96f07186bd6493eafd0dffffc516750"}'></script><!-- End Cloudflare Web Analytics -->"""
h = open('hanja-ime.html').read()
assert h.count('</body>') == 1
open('../deploy/index.html', 'w').write(h.replace('</body>', SNIP + '\n</body>'))
shutil.copy('icon.svg', '../deploy/favicon.svg')
# 다른 사이트용 사전 한 덩어리 (deploy/hanmun-dict.json, 설명은 hanmun-dict.md)
import json
J = lambda n: json.load(open(n, encoding='utf-8'))
cl, cs = J('classical.json'), J('corpus_stats.json')
d = {'version': '0.1', 'c': J('chars_ranked.json'), 'w': J('words.json'), 'p': cl['p'], 'cw': cl['cw'],
     'kw': J('corpus_words.json'), 'gw': J('classics_words.json'), 'u': cs['u'], 'f': cs['f'], 'm': J('meanings.json')}
open('../deploy/hanmun-dict.json', 'w', encoding='utf-8').write(json.dumps(d, ensure_ascii=False, separators=(',', ':')))
subprocess.run('rm -f ../hanmun-ime-cloudflare.zip && cd ../deploy && zip -qr ../hanmun-ime-cloudflare.zip .', shell=True, check=True)
print('ok')
