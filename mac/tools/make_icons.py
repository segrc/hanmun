# 맥용 아이콘: 앱 아이콘(iconset)과 메뉴 막대 입력 모드 아이콘(가 / 漢, 흑백 템플릿)
import os, sys
from PIL import Image, ImageDraw, ImageFont
out = sys.argv[1]
os.makedirs(out, exist_ok=True)

def app_icon(S):
    k = S / 64
    im = Image.new('RGBA', (S, S), (0, 0, 0, 0)); d = ImageDraw.Draw(im)
    R = lambda x, y, w, h: [x*k, y*k, (x+w)*k, (y+h)*k]
    d.rounded_rectangle(R(0, 0, 64, 64), radius=14*k, fill='#6ab83a')
    d.rounded_rectangle(R(10, 19, 44, 27), radius=5*k, outline='white', width=max(1, round(3.5*k)))
    for x in (16, 22.6, 29.2, 35.8, 42.4):
        d.rounded_rectangle(R(x, 25, 4.2, 4.2), radius=1.1*k, fill='white')
    d.rounded_rectangle(R(21, 36, 22, 4), radius=2*k, fill='white')
    return im

iconset = os.path.join(out, 'AppIcon.iconset'); os.makedirs(iconset, exist_ok=True)
for s in (16, 32, 128, 256, 512):
    app_icon(s).save(f'{iconset}/icon_{s}x{s}.png'); app_icon(s*2).save(f'{iconset}/icon_{s}x{s}@2x.png')

FONT = '/System/Library/Fonts/AppleSDGothicNeo.ttc'
def glyph(ch, S):
    im = Image.new('RGBA', (S, S), (0, 0, 0, 0)); d = ImageDraw.Draw(im)
    f = ImageFont.truetype(FONT, int(S * 0.86), index=6 if os.path.exists(FONT) else 0)   # Bold
    l, t, r, b = d.textbbox((0, 0), ch, font=f)
    d.text(((S - (r - l)) / 2 - l, (S - (b - t)) / 2 - t), ch, font=f, fill=(0, 0, 0, 255))
    return im
for name, ch in (('hangul', '가'), ('hanmun', '漢')):
    glyph(ch, 16).save(f'{out}/{name}.png'); glyph(ch, 32).save(f'{out}/{name}@2x.png')
print('icons ok')
