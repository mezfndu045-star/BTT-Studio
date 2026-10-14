"""Preview of skins at rest (A and ZR held, tinted pink) with their labels, like the game draws them"""
import subprocess, pathlib, sys
from PIL import Image, ImageDraw, ImageFont

here = pathlib.Path(__file__).parent
root = here / sys.argv[1]
only = sys.argv[3:] if len(sys.argv) > 3 else None
tmp = here / 'skin_preview_tmp'
tmp.mkdir(exist_ok=True)
font_path = r'C:\Users\FlowUP\AppData\Roaming\yuzu\load\0100000000010000\Practice mod 1.0\romfs\ImGuiData\Fonts\RodinNTLG-Bold.otf'


def load(skin, name):
    out = tmp / f'{skin}_{name}.png'
    if out.exists(): out.unlink()
    subprocess.run([str(here / 'astcenc_dl/x/bin/astcenc-avx2.exe'), '-dl', str(root / skin / f'{name}.astc'), str(out), '-silent'], check=True)
    return Image.open(out).convert('RGBA')


tiles = []
for d in sorted(p for p in root.iterdir() if p.is_dir()):
    if only and d.name not in only: continue
    lines = [l.split() for l in (d / 'skin.txt').read_text().splitlines() if l.strip()]
    bg = next(l for l in lines if l[0] == 'bg')
    canvas = Image.new('RGBA', (int(bg[2]), int(bg[3])), (95, 120, 80, 255))
    canvas.alpha_composite(load(d.name, 'bg'))
    for l in lines:
        if l[0] == 'stick':
            _, which, img, x, y, w, h, xr, yr = l
            canvas.alpha_composite(load(d.name, img).resize((int(w), int(h))), (int(x), int(y)))
    for l in lines:
        if l[0] in ('btn', 'btnt') and l[1] in ('a', 'zr'):
            _, inputs, img, x, y, w, h = l
            im = load(d.name, img).resize((int(w), int(h)))
            tint = Image.new('RGBA', im.size, (235, 64, 140, 0)); tint.putalpha(im.getchannel('A'))
            canvas.alpha_composite(tint, (int(x), int(y)))
    dr = ImageDraw.Draw(canvas)
    for l in lines:
        if l[0] == 'label':
            txt, x, y, size = l[1], float(l[2]), float(l[3]), float(l[4])
            f = ImageFont.truetype(font_path, max(11, int(size)))
            bb = dr.textbbox((0, 0), txt, font=f)
            tx, ty = x - (bb[2] - bb[0]) / 2 - bb[0], y - (bb[3] - bb[1]) / 2 - bb[1]
            dr.text((tx + 1.5, ty + 1.5), txt, font=f, fill=(0, 0, 0, 200)); dr.text((tx, ty), txt, font=f, fill=(255, 255, 255, 235))
    canvas.thumbnail((400, 300)); tiles.append((d.name, canvas))
W, H = 400, 320
sheet = Image.new('RGB', (3 * W, ((len(tiles) + 2) // 3) * H), (30, 30, 30)); dr = ImageDraw.Draw(sheet)
for i, (n, t) in enumerate(tiles):
    x, y = (i % 3) * W, (i // 3) * H
    sheet.paste(t.convert('RGB'), (x, y + 16)); dr.text((x + 4, y + 2), n, fill='white')
sheet.save(here / sys.argv[2])
