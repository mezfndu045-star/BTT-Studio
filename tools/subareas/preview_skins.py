import subprocess, pathlib, sys
from PIL import Image

here = pathlib.Path(__file__).parent
ASTCENC = here / 'astcenc_dl/x/bin/astcenc-avx2.exe'
root = here / sys.argv[1]
tmp = here / 'skin_preview_tmp'
tmp.mkdir(exist_ok=True)


def load(skin, name):
    out = tmp / f'{skin}_{name}.png'
    if not out.exists():
        subprocess.run([str(ASTCENC), '-dl', str(root / skin / f'{name}.astc'), str(out), '-silent'], check=True)
    return Image.open(out).convert('RGBA')


tiles = []
for d in sorted(p for p in root.iterdir() if p.is_dir()):
    lines = [l.split() for l in (d / 'skin.txt').read_text().splitlines() if l.strip()]
    bg = next(l for l in lines if l[0] == 'bg')
    canvas = Image.new('RGBA', (int(bg[2]), int(bg[3])), (60, 60, 70, 255))
    canvas.alpha_composite(load(d.name, 'bg'))
    for l in lines:
        if l[0] == 'stick':
            _, which, img, x, y, w, h, xr, yr = l
            im = load(d.name, img).resize((int(w), int(h)))
            canvas.alpha_composite(im, (int(x) + int(int(xr) * 0.7), int(y) - int(int(yr) * 0.7)))
    for l in lines:
        if l[0] in ('btn', 'btnt'):
            _, inputs, img, x, y, w, h = l
            im = load(d.name, img).resize((int(w), int(h)))
            canvas.alpha_composite(im, (int(x), int(y)))
    canvas.thumbnail((400, 300))
    tiles.append((d.name, canvas))

W, H = 400, 320
sheet = Image.new('RGB', (3 * W, ((len(tiles) + 2) // 3) * H), (30, 30, 30))
from PIL import ImageDraw
dr = ImageDraw.Draw(sheet)
for i, (n, t) in enumerate(tiles):
    x, y = (i % 3) * W, (i // 3) * H
    bg = Image.new('RGBA', t.size, (30, 30, 30, 255)); bg.alpha_composite(t)
    sheet.paste(bg.convert('RGB'), (x, y + 16)); dr.text((x + 4, y + 2), n, fill='white')
sheet.save(here / 'skins_preview.png')
