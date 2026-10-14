"""Crop the game picture out of the yuzu window captures and make the 384x216 menu pictures."""
import sys, pathlib
from PIL import Image

src = pathlib.Path(sys.argv[1])
dst = pathlib.Path(sys.argv[2])
dst.mkdir(exist_ok=True)


def game_box(im):
    # yuzu window: title bar + menu bar on top, status bar at the bottom, game picture fitted (16:9) in between
    w, h = im.size
    gray = im.convert('L')
    # first row under the menu bar where the dark game background starts (menu bar is light, ~240)
    top = 0
    for y in range(10, min(120, h)):
        row = [gray.getpixel((x, y)) for x in range(0, w, max(1, w // 64))]
        if sum(1 for v in row if v > 200) < len(row) // 4 and y > 30:
            top = y
            break
    bottom = h - 22
    gh = bottom - top
    gw = gh * 16 / 9
    if gw > w:
        gw = w
        gh = gw * 9 / 16
        top = top + ((bottom - top) - gh) / 2
    left = (w - gw) / 2
    return (round(left), round(top), round(left + gw), round(top + gh))


for p in sorted(src.glob('*.png')):
    im = Image.open(p).convert('RGB')
    box = game_box(im)
    game = im.crop(box).resize((384, 216), Image.LANCZOS)
    game.save(dst / p.name)
print('cropped', len(list(dst.glob('*.png'))))
