"""Preview a converted skin: background, every pressed picture and the sticks (decoded from the .astc files)."""
import sys, pathlib, subprocess, tempfile
from PIL import Image
skin = pathlib.Path(sys.argv[1]); astcenc = sys.argv[2]; out = sys.argv[3]
tmp = pathlib.Path(tempfile.mkdtemp())
def dec(name):
    png = tmp / (name + '.png')
    subprocess.run([astcenc, '-dl', str(skin / (name + '.astc')), str(png), '-silent'], check=True)
    return Image.open(png).convert('RGBA')
lines = (skin / 'skin.txt').read_text().split('\n')
w, h = map(int, lines[0].split()[2:4])
bg = dec('bg').crop((0, 0, w, h))
rest = bg.copy(); pressed = bg.copy()
for l in lines[1:]:
    p = l.split()
    if not p: continue
    if p[0] == 'btn':
        im = dec(p[2]); x, y, bw, bh = map(int, p[3:7]); im = im.crop((0, 0, bw, bh))
        pressed.alpha_composite(im, (x, y))
    if p[0] == 'stick':
        im = dec(p[2]); x, y, bw, bh = map(int, p[3:7]); im = im.crop((0, 0, bw, bh))
        rest.alpha_composite(im, (x, y)); pressed.alpha_composite(im, (x + int(p[7]), y))
canvas = Image.new('RGBA', (w * 2 + 10, h), (40, 40, 40, 255))
canvas.alpha_composite(rest, (0, 0)); canvas.alpha_composite(pressed, (w + 10, 0))
canvas.save(out)
