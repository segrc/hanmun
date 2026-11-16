# 아티팩트용 hanja-ime.html → Cloudflare 배포용 deploy/index.html (+ 방문 통계 스니펫)
import shutil, subprocess
SNIP = """<!-- Cloudflare Web Analytics --><script type='module' src='https://static.cloudflareinsights.com/beacon.min.js' data-cf-beacon='{"token": "e96f07186bd6493eafd0dffffc516750"}'></script><!-- End Cloudflare Web Analytics -->"""
h = open('hanja-ime.html').read()
assert h.count('</body>') == 1
open('../deploy/index.html', 'w').write(h.replace('</body>', SNIP + '\n</body>'))
shutil.copy('icon.svg', '../deploy/favicon.svg')
subprocess.run('rm -f ../hanmun-ime-cloudflare.zip && cd ../deploy && zip -qr ../hanmun-ime-cloudflare.zip .', shell=True, check=True)
print('ok')
