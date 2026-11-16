# 웹 버전 파비콘(청록 키보드)과 같은 모양의 .ico 생성
from PIL import Image, ImageDraw
import sys
S = 256; k = S / 64
im = Image.new('RGBA', (S, S), (0, 0, 0, 0)); d = ImageDraw.Draw(im)
R = lambda x, y, w, h: [x*k, y*k, (x+w)*k, (y+h)*k]
d.rounded_rectangle(R(0, 0, 64, 64), radius=14*k, fill='#8cc63f')
d.rounded_rectangle(R(10, 19, 44, 27), radius=5*k, outline='white', width=round(3.5*k))
for x in (16, 22.6, 29.2, 35.8, 42.4):
    d.rounded_rectangle(R(x, 25, 4.2, 4.2), radius=1.1*k, fill='white')
d.rounded_rectangle(R(21, 36, 22, 4), radius=2*k, fill='white')
im.save(sys.argv[1], sizes=[(16, 16), (20, 20), (24, 24), (32, 32), (48, 48), (64, 64), (256, 256)])
