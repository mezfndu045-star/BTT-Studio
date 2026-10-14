"""Convert every Open Joystick Display 2.8.0 theme style into BTT-Studio picture skins.

The themes (app/themes/<theme>/ in the app.asar of OJD, BSD 4-Clause Attribution, (c) Anthony Mattera (KernelZechs),
Open Joystick Display) are HTML + CSS + inline SVG: OJD adds the class "active" to the elements of a held button
(ojd-button="A") and moves the stick elements (ojd-directional="0") inside their wrapper with top/left percentages.
Each style is rendered in a headless Edge (Playwright): once at rest without its sticks (background), once per
button held (the changed area becomes the "pressed" picture of that button) and once per stick alone.

usage: gen_ojd_skins.py <ojd app dir> <romfs Skins dir> <scratch dir>
Writes Skins/OJD<nnn>/skin.txt + .astc, include/menu/OjdSkins.inc (id, name pairs) and the credits.
"""
import json, re, sys, pathlib, subprocess, shutil
from PIL import Image, ImageChops
from playwright.sync_api import sync_playwright

here = pathlib.Path(__file__).parent
app = pathlib.Path(sys.argv[1])
out_root = pathlib.Path(sys.argv[2])
scratch = pathlib.Path(sys.argv[3])
ASTCENC = pathlib.Path(sys.argv[4]) if len(sys.argv) > 4 else None
TARGET_W, TARGET_H = 360, 300

# Button names of the themes -> inputs of the practice skins
COMMON = {
    'L': 'l', 'R': 'r', 'L2': 'zl', 'R2': 'zr', 'L3': 'ls', 'R3': 'rs', 'UP': 'up', 'DOWN': 'down', 'LEFT': 'left',
    'RIGHT': 'right', 'START': 'plus', 'PLUS': 'plus', 'SELECT': 'minus', 'MINUS': 'minus', 'RUN': 'plus',
    'MODE': 'minus', 'HOME': 'home', 'SCREENSHOT': 'capture',
}
BY_NAME = {'A': 'a', 'B': 'b', 'X': 'x', 'Y': 'y'}
# Xbox / PlayStation: same place on the controller as on a Switch (bottom = B, right = A, left = Y, top = X)
BY_PLACE = {'A': 'b', 'B': 'a', 'X': 'y', 'Y': 'x', 'CROSS': 'b', 'CIRCLE': 'a', 'SQUARE': 'y', 'TRIANGLE': 'x',
            'TRIANGE': 'x'}
PLACE_THEMES = ('ojd-microsoft-xbox', 'ojd-scuf-ps4', 'ojd-sony-playstation')
EXTRA = {
    'ojd-nintendo-gamecube': {'Z': 'r'},
    'ojd-nintendo-64': {'Z': 'zl|zr'},
    'ojd-nec-pc-engine': {'1': 'a', '2': 'b'},
    'ojd-sega-master-system': {'1': 'b', '2': 'a'},
    # 3 / 6 buttons: bottom row A B C like Y B A, top row X Y Z like X L R
    'ojd-sega-mega-drive': {'A': 'y', 'B': 'b', 'C': 'a', 'X': 'x', 'Y': 'l', 'Z': 'r'},
    'ojd-sega-saturn': {'A': 'y', 'B': 'b', 'C': 'a', 'X': 'x', 'Y': 'l', 'Z': 'r'},
}


def mapping(theme):
    m = dict(COMMON)
    m.update(BY_PLACE if theme in PLACE_THEMES else BY_NAME)
    m.update(EXTRA.get(theme, {}))
    return m


def svg_inline(html, theme_dir):
    def repl(mo):
        path = pathlib.Path(mo.group(2))
        try:
            svg = path.read_text(encoding='utf-8', errors='replace')
        except OSError:
            return mo.group(0)
        svg = re.sub(r'<\?xml[^>]*\?>', '', svg)
        svg = re.sub(r'<!DOCTYPE[^>]*>', '', svg)
        svg = re.sub(r'<script.*?</script>', '', svg, flags=re.S | re.I)
        return mo.group(1) + '>' + svg + '</div>'
    return re.sub(r"(<div[^>]*ojd-svg=['\"]([^'\"]+)['\"][^>]*)>\s*</div>", repl, html)


def page_for(theme_dir, theme, style):
    css_root = style.get('cssroot', '')
    base = theme_dir / css_root if css_root else theme_dir
    css = [base / (style['mastercss'] if style.get('mastercss') else 'theme.css')]
    extra = base / f"theme-{style['id']}.css"
    if extra.exists():
        css.append(extra)
    html_file = theme_dir / style['file'] if style.get('file') and (theme_dir / style['file']).exists() \
        else theme_dir / 'theme.html'
    html = html_file.read_text(encoding='utf-8', errors='replace')
    html = re.sub(r'<script.*?</script>', '', html, flags=re.S | re.I)
    html = html.replace('%DIRECTORY%', theme_dir.as_posix() + '/')
    html = svg_inline(html, theme_dir)
    links = ''.join(f'<link rel="stylesheet" href="{c.as_uri()}">' for c in css)
    return (f'<!doctype html><html><head><meta charset="utf-8">{links}'
            '<style>html,body{margin:0;padding:0;background:transparent}#c{display:inline-block;position:relative}'
            '</style><style id="state"></style></head>'
            f'<body><div id="c">{html}</div></body></html>')


HIDE_STICKS = '[ojd-directional],[ojd-arcade-directional]{visibility:hidden !important}'


def only(sel):
    return f'#c *{{visibility:hidden !important}} #c {sel},#c {sel} *{{visibility:visible !important}}'


def encode(img, path, block):
    png = path.with_suffix('.png')
    img.save(png)
    subprocess.run([str(ASTCENC), '-cl', str(png), str(path), block, '-medium', '-silent'], check=True)
    png.unlink()


def shoot(pg, state_css, js=''):
    pg.evaluate('''([css, js]) => {
        document.getElementById('state').textContent = css;
        document.querySelectorAll('.active,.trigger-active').forEach(e => e.classList.remove('active','trigger-active'));
        document.querySelectorAll('[ojd-trigger-scale],[ojd-trigger-move]').forEach(e => {e.style.height='';e.style.top='';});
        if (js) eval(js);
    }''', [state_css, js])
    return pg.locator('#c').screenshot(omit_background=True, animations='disabled')


def press_js(names, triggers):
    sel = ','.join(f'[ojd-button="{n}"]' for n in names)
    js = f"document.querySelectorAll('{sel}').forEach(e=>e.classList.add('active'));" if names else ''
    for t in triggers:
        js += (f"document.querySelectorAll('[ojd-trigger=\"{t}\"]').forEach(e=>e.classList.add('trigger-active'));"
               f"document.querySelectorAll('[ojd-trigger-scale=\"{t}\"]').forEach(e=>e.style.height='100%');"
               f"document.querySelectorAll('[ojd-trigger-move=\"{t}\"]').forEach(e=>e.style.top='100%');")
    return js


def load_png(data):
    import io
    return Image.open(io.BytesIO(data)).convert('RGBA')


def changed_box(a, b):
    diff = ImageChops.difference(a, b).convert('L').point(lambda v: 255 if v > 24 else 0)
    return diff.getbbox()


def convert_style(pg, theme_dir, theme, style, skin_dir):
    # Loaded from a file: a page set from a string (about:blank) may not load the file:// stylesheets
    page = scratch / 'page.html'
    page.write_text(page_for(theme_dir, theme, style), encoding='utf-8')
    pg.goto(page.as_uri(), wait_until='load')
    pg.wait_for_timeout(50)
    base = load_png(shoot(pg, HIDE_STICKS))
    box = base.getbbox()
    if not box:
        return None
    # Crop to the drawn area, scale to the target size
    f = min(1.0, TARGET_W / (box[2] - box[0]), TARGET_H / (box[3] - box[1]))
    ox, oy = box[0], box[1]

    def fit(im, b):
        im = im.crop(b)
        w, h = max(1, round(im.width * f)), max(1, round(im.height * f))
        return im.resize((w, h), Image.LANCZOS)

    skin_dir.mkdir(parents=True, exist_ok=True)
    bg = fit(base, box)
    encode(bg, skin_dir / 'bg.astc', '6x6')
    lines = [f'bg bg {bg.width} {bg.height}']
    img_n = 0
    m = mapping(theme)
    present = set(pg.evaluate("[...document.querySelectorAll('[ojd-button]')].map(e=>e.getAttribute('ojd-button'))"))
    groups = {}
    for name in sorted(present):
        if name in m:
            groups.setdefault(m[name], []).append(name)
    triggers = {'zl': '0', 'zr': '1'}
    has_trigger = set(pg.evaluate("[...document.querySelectorAll('[ojd-trigger]')].map(e=>e.getAttribute('ojd-trigger'))"))
    for inp in sorted(set(groups) | {k for k, t in triggers.items() if t in has_trigger}):
        names = groups.get(inp, [])
        trig = [triggers[inp]] if inp in triggers and triggers[inp] in has_trigger else []
        pressed = load_png(shoot(pg, HIDE_STICKS, press_js(names, trig)))
        b = changed_box(base, pressed)
        if not b:
            continue
        b = (max(b[0], box[0]), max(b[1], box[1]), min(b[2], box[2]), min(b[3], box[3]))
        if b[2] <= b[0] or b[3] <= b[1]:
            continue
        im = fit(pressed, b)
        encode(im, skin_dir / f'i{img_n}.astc', '4x4')
        x, y = round((b[0] - ox) * f), round((b[1] - oy) * f)
        lines.append(f'btn {inp} i{img_n} {x} {y} {im.width} {im.height}')
        img_n += 1
    # Sticks: the stick alone, centered in its wrapper, moving by half the wrapper size
    sticks = pg.evaluate('''() => [...document.querySelectorAll('[ojd-directional],[ojd-arcade-directional]')].map(e => {
        const r = e.getBoundingClientRect(), p = e.parentElement.getBoundingClientRect(),
              c = document.getElementById('c').getBoundingClientRect();
        return {d: e.getAttribute('ojd-directional') ?? 'arcade', x: r.left - c.left, y: r.top - c.top, w: r.width,
                h: r.height, pw: p.width, ph: p.height};
    })''')
    seen = set()
    for s in sticks:
        side = 'R' if s['d'] == '1' else 'L'
        if side in seen or s['w'] < 2 or s['h'] < 2:
            continue
        seen.add(side)
        sel = f"[ojd-directional='{s['d']}']" if s['d'] != 'arcade' else '[ojd-arcade-directional]'
        alone = load_png(shoot(pg, only(sel)))
        sb = (int(s['x']), int(s['y']), int(s['x'] + s['w'] + 0.999), int(s['y'] + s['h'] + 0.999))
        sb = (max(sb[0], 0), max(sb[1], 0), min(sb[2], alone.width), min(sb[3], alone.height))
        if sb[2] <= sb[0] or sb[3] <= sb[1] or not alone.crop(sb).getbbox():
            continue
        im = fit(alone, sb)
        encode(im, skin_dir / f'i{img_n}.astc', '4x4')
        x, y = round((sb[0] - ox) * f), round((sb[1] - oy) * f)
        lines.append(f'stick {side} i{img_n} {x} {y} {im.width} {im.height} '
                     f'{round(s["pw"] / 2 * f)} {round(s["ph"] / 2 * f)}')
        img_n += 1
    (skin_dir / 'skin.txt').write_bytes(('\n'.join(lines) + '\n').encode())
    return len(lines) - 1


def main():
    themes_dir = app / 'themes'
    entries = []
    scratch.mkdir(parents=True, exist_ok=True)
    for old in out_root.glob('OJD*'):
        if old.is_dir():
            shutil.rmtree(old)
    with sync_playwright() as p:
        browser = p.chromium.launch(channel='msedge')
        pg = browser.new_page(viewport={'width': 1200, 'height': 1000})
        n = 0
        for theme_dir in sorted(themes_dir.iterdir()):
            meta = json.loads((theme_dir / 'theme.json').read_text(encoding='utf-8-sig'))
            for style in meta['styles']:
                skin_id = f'OJD{n:03d}'
                try:
                    count = convert_style(pg, theme_dir, theme_dir.name, style, out_root / skin_id)
                except Exception as e:
                    print('FAIL', theme_dir.name, style['id'], e)
                    count = None
                if count:
                    entries.append((skin_id, style['name'], theme_dir.name, count))
                    n += 1
                    print(skin_id, theme_dir.name, style['name'], count, flush=True)
                else:
                    shutil.rmtree(out_root / skin_id, ignore_errors=True)
        browser.close()
    inc = here.parent.parent / 'include/menu/OjdSkins.inc'
    inc.write_text('// Generated by tools/subareas/gen_ojd_skins.py: Open Joystick Display styles (id, menu name)\n' +
                   ''.join(f'BTT_OJD_SKIN("{i}", "{name.replace(chr(34), "")}")\n' for i, name, _, _ in entries),
                   encoding='utf-8')
    (out_root / 'OJD-LICENSE.txt').write_text(
        'Input display styles OJD000 ... converted from the themes of Open Joystick Display 2.8.0\n'
        '(https://ojdproject.com), BSD 4-Clause Attribution.\n'
        'Copyright 2019 Anthony Mattera (KernelZechs), Open Joystick Display.\n\n'
        'Converted styles:\n' + ''.join(f'{i}  {t}  {name}\n' for i, name, t, _ in entries), encoding='utf-8')
    print(len(entries), 'skins')


if __name__ == '__main__':
    main()
