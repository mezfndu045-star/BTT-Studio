"""Convert Periscope / RetroSpy skins into BTT-Studio image skins (romfs ImGuiData/Skins/<id>/skin.txt + .astc).

skin.txt lines (coordinates in pixels of the converted background):
  bg <image> <w> <h>
  btn <inputs> <image> <x> <y> <w> <h>        drawn while one of the inputs ("a|x") is held
  stick <L|R> <image> <x> <y> <w> <h> <xrange> <yrange>
Inputs: a b x y l r zl zr plus minus up down left right ls rs home capture, and the stick directions
lsu lsd lsl lsr rsu rsd rsl rsr.
"""
import os, re, sys, subprocess, pathlib, shutil
import xml.etree.ElementTree as ET
from PIL import Image

here = pathlib.Path(__file__).parent
ASTCENC = here / 'astcenc_dl/x/bin/astcenc-avx2.exe'
OUT = pathlib.Path(sys.argv[1])
MAX_W = 640

SKINS = {
    # id: (source dir, kind, background name or None, section types, mapping retro name -> inputs, stick mapping)
    'SwitchPro': ('retrospy_skins/switch-pro', 'retrospy', None, ['switch'], {
        'a': 'a', 'b': 'b', 'x': 'x', 'y': 'y', '+': 'plus', '-': 'minus', 'zl': 'zl', 'zr': 'zr', 'l': 'l', 'r': 'r',
        'home': 'home', 'capture': 'capture', 'ls': 'ls', 'rs': 'rs', 'up': 'up', 'down': 'down', 'left': 'left',
        'right': 'right'}, {'lstick_x': 'L', 'rstick_x': 'R'}),
    'GameCube': ('retrospy_skins/gamecube-default', 'retrospy', 'Purple', [], {
        'a': 'a', 'b': 'b', 'x': 'x', 'y': 'y', 'z': 'r', 'start': 'plus', 'l': 'l', 'r': 'r', 'trig_l': 'zl',
        'trig_r': 'zr', 'up': 'up', 'down': 'down', 'left': 'left', 'right': 'right'},
        {'lstick_x': 'L', 'cstick_x': 'R'}),
    'N64': ('retrospy_skins/n64-default', 'retrospy', None, [], {
        'a': 'a', 'b': 'b', 'z': 'zl|zr', 'start': 'plus', 'l': 'l', 'r': 'r', 'up': 'up', 'down': 'down', 'left': 'left',
        'right': 'right', 'cup': 'rsu|x', 'cdown': 'rsd', 'cleft': 'rsl|y', 'cright': 'rsr'}, {'stick_x': 'L'}),
    'NES': ('retrospy_skins/nes-dogbone', 'retrospy', None, [], {
        'a': 'a|x', 'b': 'b|y', 'select': 'minus', 'start': 'plus', 'up': 'up|lsu', 'down': 'down|lsd',
        'left': 'left|lsl', 'right': 'right|lsr'}, {}),
    'SNES': ('retrospy_skins/wii-snes', 'retrospy', None, ['switch', 'wii;snes;switch'], {
        'a': 'a', 'b': 'b', 'x': 'x', 'y': 'y', 'l': 'l|zl', 'r': 'r|zr', '+': 'plus', '-': 'minus', 'start': 'plus',
        'select': 'minus', 'up': 'up|lsu', 'down': 'down|lsd', 'left': 'left|lsl', 'right': 'right|lsr'}, {}),
    'PS4': ('retrospy_skins/playstation4-dualshock4', 'retrospy', None, [], {
        'triangle': 'x', 'circle': 'a', 'x': 'b', 'square': 'y', 'l1': 'l', 'r1': 'r', 'l_trig': 'zl', 'r_trig': 'zr',
        'share': 'minus', 'options': 'plus', 'ps': 'home', 'lstick': 'ls', 'rstick': 'rs', 'up': 'up', 'down': 'down',
        'left': 'left', 'right': 'right'}, {'lstick_x': 'L', 'rstick_x': 'R'}),
    'Xbox360': ('retrospy_skins/xbox360-controller', 'retrospy', None, [], {
        'y': 'x', 'b': 'a', 'a': 'b', 'x': 'y', 'lb': 'l', 'rb': 'r', 'trig_l': 'zl', 'trig_r': 'zr', 'start': 'plus',
        'back': 'minus', 'l3': 'ls', 'r3': 'rs', 'up': 'up', 'down': 'down', 'left': 'left', 'right': 'right'},
        {'lstick_x': 'L', 'rstick_x': 'R'}),
    'WiiClassic': ('retrospy_skins/wii-classic', 'retrospy', None, ['switch', 'wii;switch'], {
        'a': 'a', 'b': 'b', 'x': 'x', 'y': 'y', 'l': 'l', 'r': 'r', 'zl': 'zl', 'zr': 'zr', '+': 'plus', '-': 'minus',
        'start': 'plus', 'select': 'minus', 'home': 'home', 'up': 'up', 'down': 'down', 'left': 'left', 'right': 'right'},
        {'lstick_x': 'L', 'rstick_x': 'R'}),
}


def remove_key_background(im):
    """Flood fill the outside of the controller (chroma key / black background) with transparency."""
    im = im.convert('RGBA')
    w, h = im.size
    px = im.load()
    corners = [px[0, 0], px[w - 1, 0], px[0, h - 1], px[w - 1, h - 1]]
    key = max(set(corners), key=corners.count)
    if key[3] < 10:
        return im
    tol = 60

    def close(c):
        return c[3] > 0 and abs(c[0] - key[0]) + abs(c[1] - key[1]) + abs(c[2] - key[2]) <= tol

    stack = [(x, y) for x in range(w) for y in (0, h - 1)] + [(x, y) for y in range(h) for x in (0, w - 1)]
    seen = bytearray(w * h)
    while stack:
        x, y = stack.pop()
        if x < 0 or y < 0 or x >= w or y >= h or seen[y * w + x]:
            continue
        seen[y * w + x] = 1
        if not close(px[x, y]):
            continue
        px[x, y] = (0, 0, 0, 0)
        stack.extend(((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)))
    return im


def make_tintable(im):
    """Flat single-color overlays (circles...) -> white with the same alpha, so the game can tint them"""
    px = [p for p in im.getdata() if p[3] > 128]
    if not px: return im, False
    import statistics
    sd = max(statistics.pstdev([p[c] for p in px]) for c in range(3))
    if sd > 30: return im, False
    white = Image.new('RGBA', im.size, (255, 255, 255, 0))
    white.putalpha(im.getchannel('A'))
    return white, True


def encode(img, path):
    png = path.with_suffix('.png')
    img.save(png)
    subprocess.run([str(ASTCENC), '-cl', str(png), str(path), '4x4', '-medium', '-silent'], check=True)
    png.unlink()


def section_ok(el, wanted):
    t = el.get('type')
    return t in wanted


def retrospy(skin_id, src, bg_name, sections, mapping, sticks):
    src = here / src
    root = ET.parse(src / 'skin.xml').getroot()
    bgs = root.findall('background')
    bg = next((b for b in bgs if b.get('name') == bg_name), bgs[0])
    bg_img = remove_key_background(Image.open(src / bg.get('image')))
    f = min(1.0, MAX_W / bg_img.width)
    out = OUT / skin_id
    out.mkdir(parents=True, exist_ok=True)
    bw, bh = round(bg_img.width * f), round(bg_img.height * f)
    encode(bg_img.resize((bw, bh), Image.LANCZOS), out / 'bg.astc')
    lines = [f'bg bg {bw} {bh}']
    elems = list(root)
    for sec in root.findall('section'):
        if section_ok(sec, sections):
            elems += list(sec)
    images = {}

    def image_id(name, w, h, tint_ok=True):
        key = (name, w, h, tint_ok)
        if key not in images:
            im = Image.open(src / name).convert('RGBA').resize((max(1, w), max(1, h)), Image.LANCZOS)
            tinted = False
            if tint_ok: im, tinted = make_tintable(im)
            images[key] = (f'i{len(images)}', tinted)
            encode(im, out / f'{images[key][0]}.astc')
        return images[key]

    def geom(el):
        # width / height default to the image size
        iw, ih = Image.open(src / el.get('image')).size
        w = float(el.get('width') or iw); h = float(el.get('height') or ih)
        return round(float(el.get('x')) * f), round(float(el.get('y')) * f), round(w * f), round(h * f)

    used = set()
    for el in elems:
        if el.tag in ('button', 'analog'):
            inputs = mapping.get(el.get('name'))
            if not inputs: continue
            x, y, w, h = geom(el)
            if (inputs, x, y) in used: continue
            used.add((inputs, x, y))
            iid, tinted = image_id(el.get("image"), w, h)
            lines.append(f'{"btnt" if tinted else "btn"} {inputs} {iid} {x} {y} {w} {h}')
        elif el.tag == 'stick':
            which = sticks.get(el.get('xname'))
            if not which: continue
            x, y, w, h = geom(el)
            xr, yr = round(float(el.get('xrange')) * f), round(float(el.get('yrange')) * f)
            lines.append(f'stick {which} {image_id(el.get("image"), w, h, False)[0]} {x} {y} {w} {h} {xr} {yr}')
    (out / 'skin.txt').write_bytes(('\n'.join(lines) + '\n').encode())
    return len(lines)


def periscope(skin_id, src):
    import tomllib
    src = here / src
    cfg = tomllib.loads((src / 'skin.toml').read_text())
    out = OUT / skin_id
    out.mkdir(parents=True, exist_ok=True)
    bg = remove_key_background(Image.open(src / cfg['background']))
    f = min(1.0, MAX_W / bg.width)
    bw, bh = round(bg.width * f), round(bg.height * f)
    encode(bg.resize((bw, bh), Image.LANCZOS), out / 'bg.astc')
    lines = [f'bg bg {bw} {bh}']
    names = {'a': 'a', 'b': 'b', 'x': 'x', 'y': 'y', 'plus': 'plus', 'minus': 'minus', 'zl': 'zl', 'zr': 'zr', 'l': 'l',
             'r': 'r', 'up': 'up', 'down': 'down', 'left': 'left', 'right': 'right', 'ls': 'ls', 'rs': 'rs'}
    images = {}

    def image_id(name, tint_ok=True):
        if name not in images:
            im = Image.open(src / name).convert('RGBA')
            w, h = max(1, round(im.width * f)), max(1, round(im.height * f))
            im = im.resize((w, h), Image.LANCZOS)
            tinted = False
            if tint_ok: im, tinted = make_tintable(im)
            images[name] = (f'i{len(images)}', w, h, tinted)
            encode(im, out / f'{images[name][0]}.astc')
        return images[name]

    p1 = cfg['player1']
    for btn, v in p1['buttons'].items():
        if btn not in names: continue
        iid, w, h, tinted = image_id(v['image'])
        lines.append(f'{"btnt" if tinted else "btn"} {names[btn]} {iid} {round(v["pos"]["x"] * f)} {round(v["pos"]["y"] * f)} {w} {h}')
    for key, which in (('ls', 'L'), ('rs', 'R')):
        v = p1[key]
        iid, w, h, _ = image_id(v['image'], False)
        r = round(v['range'] * f)
        lines.append(f'stick {which} {iid} {round(v["pos"]["x"] * f)} {round(v["pos"]["y"] * f)} {w} {h} {r} {r}')
    (out / 'skin.txt').write_bytes(('\n'.join(lines) + '\n').encode())
    return len(lines)


# Button names written on the skins whose pictures do not show them (input of the button -> text)
LABELS = {
    'Periscope': {'a': 'A', 'b': 'B', 'x': 'X', 'y': 'Y', 'l': 'L', 'r': 'R', 'zl': 'ZL', 'zr': 'ZR', 'plus': '+',
                  'minus': '-', 'up': '^', 'down': 'v', 'left': '<', 'right': '>'},
    'GameCube': {'a': 'A', 'b': 'B', 'x': 'X', 'y': 'Y', 'r': 'Z', 'plus': 'START'},
    'N64': {'a': 'A', 'b': 'B', 'plus': 'S'},
    'NES': {'a|x': 'A', 'b|y': 'B'},
}


def add_labels(skin_id):
    path = OUT / skin_id / 'skin.txt'
    lines = path.read_text().splitlines()
    labels = LABELS.get(skin_id, {})
    done = set()
    extra = []
    for l in lines:
        parts = l.split()
        if parts[0] in ('btn', 'btnt') and parts[1] in labels and parts[1] not in done:
            done.add(parts[1])
            x, y, w, h = (float(v) for v in parts[3:7])
            size = max(10, min(h * 0.55, w * 0.45, 30))
            extra.append(f'label {labels[parts[1]]} {x + w / 2:.0f} {y + h / 2:.0f} {size:.0f}')
    path.write_text(chr(10).join(lines + extra) + chr(10))
    return len(extra)


if OUT.exists():
    shutil.rmtree(OUT)
print('Periscope', periscope('Periscope', 'periscope_skin/skin/default'), add_labels('Periscope'))
for sid, (src, kind, bg, secs, mapping, sticks) in SKINS.items():
    print(sid, retrospy(sid, src, bg, secs, mapping, sticks), add_labels(sid))
