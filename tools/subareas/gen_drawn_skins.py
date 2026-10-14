"""Drawn controller skins for the BTT-Studio input display (same skin.txt format as gen_skins.py).

Every controller is built from simple shapes at 4x resolution (smooth edges), filled with a translucent body color
and outlined in white, like the Periscope skin. Pressed pictures are white shapes tinted in game with the Pressed Color.
"""
import math, pathlib, shutil, subprocess, sys
from PIL import Image, ImageDraw, ImageFilter

here = pathlib.Path(__file__).parent
ASTCENC = here / 'astcenc_dl/x/bin/astcenc-avx2.exe'
OUT = pathlib.Path(sys.argv[1])
SS = 4  # supersampling
WHITE = (255, 255, 255, 255)


# ---------- shapes (in final pixels, drawn at SS x) ----------
class Canvas:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.mask = Image.new('L', (w * SS, h * SS), 0)
        self.d = ImageDraw.Draw(self.mask)

    def circle(self, cx, cy, r, v=255):
        self.d.ellipse([(cx - r) * SS, (cy - r) * SS, (cx + r) * SS, (cy + r) * SS], fill=v)

    def ellipse(self, cx, cy, rx, ry, angle=0, v=255):
        pts = []
        for i in range(96):
            t = 2 * math.pi * i / 96
            x, y = rx * math.cos(t), ry * math.sin(t)
            a = math.radians(angle)
            pts.append(((cx + x * math.cos(a) - y * math.sin(a)) * SS, (cy + x * math.sin(a) + y * math.cos(a)) * SS))
        self.d.polygon(pts, fill=v)

    def rrect(self, x0, y0, x1, y1, r, v=255):
        self.d.rounded_rectangle([x0 * SS, y0 * SS, x1 * SS, y1 * SS], radius=r * SS, fill=v)

    def poly(self, pts, v=255):
        self.d.polygon([(x * SS, y * SS) for x, y in pts], fill=v)

    def ngon(self, cx, cy, r, n, rot=22.5, v=255):
        self.poly([(cx + r * math.cos(math.radians(rot + 360 * i / n)), cy + r * math.sin(math.radians(rot + 360 * i / n)))
                   for i in range(n)], v)

    def cross(self, cx, cy, arm, half, r=4, v=255):
        self.rrect(cx - arm, cy - half, cx + arm, cy + half, r, v)
        self.rrect(cx - half, cy - arm, cx + half, cy + arm, r, v)


def outline_of(mask, thick):
    """Ring of `thick` px inside the edge of a SS mask"""
    if thick <= 0:
        return Image.new('L', mask.size, 0)
    k = thick * SS * 2 + 1
    inner = mask.filter(ImageFilter.MinFilter(k if k % 2 else k + 1))
    from PIL import ImageChops
    return ImageChops.subtract(mask, inner)


def down(img):
    return img.resize((img.width // SS, img.height // SS), Image.LANCZOS)


def layer(color, mask):
    im = Image.new('RGBA', mask.size, color[:3] + (0,))
    a = mask.point(lambda v: v * color[3] // 255)
    im.putalpha(a)
    return im


class Skin:
    def __init__(self, sid, w, h):
        self.sid, self.w, self.h = sid, w, h
        self.out = OUT / sid
        self.out.mkdir(parents=True, exist_ok=True)
        self.bg = Image.new('RGBA', (w * SS, h * SS), (0, 0, 0, 0))
        self.lines = [f'bg bg {w} {h}']
        self.n = 0

    # body: filled translucent + white outline
    def body(self, build, fill, line=4, line_color=WHITE):
        c = Canvas(self.w, self.h)
        build(c)
        self.bg.alpha_composite(layer(fill, c.mask))
        self.bg.alpha_composite(layer(line_color, outline_of(c.mask, line)))

    # a button: outline (and optional fill) on the background, white filled picture shown while pressed
    def button(self, inputs, build, line=3, fill=None, line_color=WHITE, label=None, label_size=None, label_at=None):
        c = Canvas(self.w, self.h)
        build(c)
        if fill: self.bg.alpha_composite(layer(fill, c.mask))
        self.bg.alpha_composite(layer(line_color, outline_of(c.mask, line)))
        box = c.mask.getbbox()
        if not box: return
        x0, y0, x1, y1 = (v // SS for v in box)
        x1 += 1; y1 += 1
        crop = c.mask.crop((x0 * SS, y0 * SS, x1 * SS, y1 * SS))
        pic = down(layer((255, 255, 255, 235), crop))
        name = self.save(pic)
        self.lines.append(f'btnt {inputs} {name} {x0} {y0} {x1 - x0} {y1 - y0}')
        if label:
            size = label_size or max(11, min((y1 - y0) * 0.55, (x1 - x0) * 0.5, 28))
            lx, ly = label_at if label_at else ((x0 + x1) / 2, (y0 + y1) / 2)
            self.lines.append(f'label {label} {lx:.0f} {ly:.0f} {size:.0f}')

    # a stick: well drawn on the background, cap picture moving
    def stick(self, which, cx, cy, well_r, cap_r, rng, cap_fill=(70, 70, 78, 255), well_fill=(0, 0, 0, 120),
              ring_color=WHITE, cap_line=WHITE, click=True):
        c = Canvas(self.w, self.h)
        c.circle(cx, cy, well_r)
        self.bg.alpha_composite(layer(well_fill, c.mask))
        self.bg.alpha_composite(layer(ring_color, outline_of(c.mask, 3)))
        cap = Canvas(cap_r * 2 + 4, cap_r * 2 + 4)
        cap.circle(cap_r + 2, cap_r + 2, cap_r)
        img = layer(cap_fill, cap.mask)
        if cap_line: img.alpha_composite(layer(cap_line, outline_of(cap.mask, 3)))
        ring = Canvas(cap_r * 2 + 4, cap_r * 2 + 4)
        ring.circle(cap_r + 2, cap_r + 2, cap_r * 0.62)
        img.alpha_composite(layer((255, 255, 255, 90), outline_of(ring.mask, 2)))
        name = self.save(down(img))
        size = cap_r * 2 + 4
        self.lines.append(f'stick {which} {name} {cx - size / 2:.0f} {cy - size / 2:.0f} {size} {size} {rng} {rng}')
        if not click: return
        # stick click: the ring of the well lights up
        self.button('ls' if which == 'L' else 'rs', lambda c: (c.circle(cx, cy, well_r - 2), c.circle(cx, cy, well_r - 11, v=0)), line=0)

    def label(self, text, x, y, size):
        self.lines.append(f'label {text} {x:.0f} {y:.0f} {size:.0f}')

    def save(self, img):
        name = f'i{self.n}'
        self.n += 1
        encode(img, self.out / f'{name}.astc')
        return name

    def finish(self):
        encode(down(self.bg), self.out / 'bg.astc')
        (self.out / 'skin.txt').write_text('\n'.join(self.lines) + '\n')
        return len(self.lines)


def encode(img, path):
    png = path.with_suffix('.png')
    img.save(png)
    subprocess.run([str(ASTCENC), '-cl', str(png), str(path), '4x4', '-medium', '-silent'], check=True)
    png.unlink()


def face_diamond(s, cx, cy, sp, r, labels=('X', 'A', 'B', 'Y'), fills=(None, None, None, None), line_colors=None):
    pos = {'x': (cx, cy - sp), 'a': (cx + sp, cy), 'b': (cx, cy + sp), 'y': (cx - sp, cy)}
    for i, k in enumerate(('x', 'a', 'b', 'y')):
        x, y = pos[k]
        s.button(k, lambda c, x=x, y=y: c.circle(x, y, r), fill=fills[i],
                 line_color=(line_colors[i] if line_colors else WHITE), label=labels[i])


def dpad(s, cx, cy, arm, half, fill=None):
    s.body(lambda c: c.cross(cx, cy, arm, half, 5), fill or (0, 0, 0, 90), line=3)
    s.button('up', lambda c: c.rrect(cx - half + 3, cy - arm + 3, cx + half - 3, cy - half, 4), line=0)
    s.button('down', lambda c: c.rrect(cx - half + 3, cy + half, cx + half - 3, cy + arm - 3, 4), line=0)
    s.button('left', lambda c: c.rrect(cx - arm + 3, cy - half + 3, cx - half, cy + half - 3, 4), line=0)
    s.button('right', lambda c: c.rrect(cx + half, cy - half + 3, cx + arm - 3, cy + half - 3, 4), line=0)


# ---------- controllers ----------
def pro_controller(sid='ProController', body_fill=(28, 28, 34, 205), xbox=False):
    s = Skin(sid, 640, 470)
    # shoulders first (behind the body)
    s.button('zl', lambda c: c.rrect(78, 8, 228, 52, 20), fill=(0, 0, 0, 120), label='ZL')
    s.button('zr', lambda c: c.rrect(412, 8, 562, 52, 20), fill=(0, 0, 0, 120), label='ZR')
    s.button('l', lambda c: c.rrect(70, 46, 250, 86, 18), fill=(0, 0, 0, 120), label='LB' if xbox else 'L', label_size=18, label_at=((70 + 250) / 2, 59))
    s.button('r', lambda c: c.rrect(390, 46, 570, 86, 18), fill=(0, 0, 0, 120), label='RB' if xbox else 'R', label_size=18, label_at=((390 + 570) / 2, 59))

    def body(c):
        c.rrect(58, 70, 582, 300, 105)
        c.ellipse(150, 320, 88, 140, -18)
        c.ellipse(490, 320, 88, 140, 18)
        c.rrect(170, 220, 470, 330, 70)
    s.body(body, body_fill, line=5)
    s.stick('L', 168, 172, 54, 40, 20)
    dpad(s, 238, 285, 42, 15)
    s.stick('R', 402, 285, 54, 40, 20)
    if xbox:
        face_diamond(s, 472, 172, 44, 21, labels=('Y', 'B', 'A', 'X'),
                     fills=((230, 190, 40, 110), (210, 50, 45, 110), (70, 170, 60, 110), (40, 110, 210, 110)))
        s.button('home', lambda c: c.circle(320, 120, 20), fill=(80, 180, 70, 140))
        s.button('minus', lambda c: c.circle(266, 172, 13), label='<', label_size=12)
        s.button('plus', lambda c: c.circle(374, 172, 13), label='>', label_size=12)
    else:
        face_diamond(s, 472, 172, 44, 21)
        s.button('minus', lambda c: c.rrect(238, 108, 270, 124, 6), label='-', label_size=14)
        s.button('plus', lambda c: c.rrect(370, 102, 402, 130, 6), label='+', label_size=14)
        s.button('capture', lambda c: c.rrect(254, 160, 280, 186, 5))
        s.button('home', lambda c: c.circle(374, 173, 15))
    return s.finish()


def joycons(sid='JoyCons'):
    s = Skin(sid, 640, 470)
    blue, red = (0, 160, 225, 190), (240, 65, 55, 190)
    s.button('zl', lambda c: c.rrect(70, 10, 230, 48, 18), fill=(0, 0, 0, 120), label='ZL')
    s.button('l', lambda c: c.rrect(60, 44, 250, 82, 18), fill=(0, 0, 0, 120), label='L', label_size=18, label_at=((60 + 250) / 2, 56))
    s.button('zr', lambda c: c.rrect(410, 10, 570, 48, 18), fill=(0, 0, 0, 120), label='ZR')
    s.button('r', lambda c: c.rrect(390, 44, 580, 82, 18), fill=(0, 0, 0, 120), label='R', label_size=18, label_at=((390 + 580) / 2, 56))

    def left(c):
        c.rrect(60, 66, 300, 462, 30)
        c.rrect(60, 66, 200, 462, 110)
    s.body(lambda c: (c.rrect(150, 66, 300, 462, 26), c.rrect(60, 66, 260, 462, 110)), blue, line=5)
    s.body(lambda c: (c.rrect(340, 66, 490, 462, 26), c.rrect(380, 66, 580, 462, 110)), red, line=5)
    s.stick('L', 180, 170, 50, 37, 18)
    for k, (x, y) in {'up': (180, 268), 'down': (180, 352), 'left': (138, 310), 'right': (222, 310)}.items():
        s.button(k, lambda c, x=x, y=y: c.circle(x, y, 19), fill=(0, 0, 0, 100),
                 label={'up': '^', 'down': 'v', 'left': '<', 'right': '>'}[k], label_size=15)
    s.button('minus', lambda c: c.rrect(250, 102, 280, 116, 5), label='-', label_size=13)
    s.button('capture', lambda c: c.rrect(236, 400, 262, 426, 5))
    face_diamond(s, 460, 170, 42, 20)
    s.stick('R', 460, 310, 50, 37, 18)
    s.button('plus', lambda c: c.rrect(360, 96, 388, 122, 5), label='+', label_size=13)
    s.button('home', lambda c: c.circle(386, 412, 15))
    return s.finish()


def gamecube(sid='GameCubeHD'):
    s = Skin(sid, 640, 470)
    s.button('zl|l', lambda c: c.rrect(70, 20, 250, 70, 24), fill=(0, 0, 0, 110), label='L')
    s.button('zr|r', lambda c: c.rrect(390, 20, 570, 70, 24), fill=(0, 0, 0, 110), label='R')
    s.button('r', lambda c: c.rrect(440, 64, 540, 92, 12), fill=(110, 80, 220, 150), label='Z', label_size=18)

    def body(c):
        c.rrect(70, 80, 570, 300, 100)
        c.ellipse(150, 330, 78, 120, -22)
        c.ellipse(490, 330, 78, 120, 22)
        c.rrect(200, 220, 440, 300, 50)
    s.body(body, (80, 66, 170, 200), line=5)
    s.stick('L', 160, 175, 56, 36, 22)
    c = Canvas(640, 470); c.ngon(160, 175, 58, 8)
    s.bg.alpha_composite(layer((255, 255, 255, 120), outline_of(c.mask, 2)))
    dpad(s, 225, 300, 34, 12)
    s.stick('R', 425, 305, 44, 26, 18, cap_fill=(225, 190, 40, 255))
    s.button('plus', lambda c: c.circle(320, 175, 13), label='START', label_size=11)
    s.button('a', lambda c: c.circle(480, 180, 36), fill=(40, 185, 120, 120), label='A')
    s.button('b', lambda c: c.circle(418, 222, 20), fill=(220, 50, 60, 120), label='B')
    s.button('x', lambda c: c.ellipse(540, 160, 18, 38, -10), fill=(210, 210, 220, 90), label='X')
    s.button('y', lambda c: c.ellipse(470, 118, 38, 17, -8), fill=(210, 210, 220, 90), label='Y')
    return s.finish()


def n64(sid='N64HD'):
    s = Skin(sid, 640, 520)
    s.button('l', lambda c: c.rrect(60, 20, 220, 60, 18), fill=(0, 0, 0, 110), label='L')
    s.button('r', lambda c: c.rrect(420, 20, 580, 60, 18), fill=(0, 0, 0, 110), label='R')

    def body(c):
        c.rrect(50, 50, 590, 250, 90)
        c.rrect(60, 150, 200, 470, 70)
        c.rrect(250, 180, 390, 510, 70)
        c.rrect(440, 150, 580, 470, 70)
    s.body(body, (150, 150, 160, 190), line=5)
    dpad(s, 140, 150, 48, 16)
    s.stick('L', 320, 330, 50, 32, 22)
    s.button('zl|zr', lambda c: c.rrect(290, 440, 350, 470, 10), fill=(0, 0, 0, 120), label='Z', label_size=16)
    s.button('plus', lambda c: c.circle(320, 150, 18), fill=(210, 40, 40, 140), label='S', label_size=16)
    s.button('b', lambda c: c.circle(440, 160, 24), fill=(40, 160, 70, 130), label='B')
    s.button('a', lambda c: c.circle(475, 205, 26), fill=(50, 90, 210, 130), label='A')
    yc = (235, 200, 40, 130)
    s.button('rsu|x', lambda c: c.circle(520, 110, 16), fill=yc, label='^', label_size=14)
    s.button('rsd', lambda c: c.circle(520, 170, 16), fill=yc, label='v', label_size=14)
    s.button('rsl|y', lambda c: c.circle(490, 140, 16), fill=yc, label='<', label_size=14)
    s.button('rsr', lambda c: c.circle(550, 140, 16), fill=yc, label='>', label_size=14)
    return s.finish()


def snes(sid='SNESHD'):
    s = Skin(sid, 640, 330)
    s.button('l|zl', lambda c: c.rrect(80, 10, 260, 50, 20), fill=(0, 0, 0, 110), label='L')
    s.button('r|zr', lambda c: c.rrect(380, 10, 560, 50, 20), fill=(0, 0, 0, 110), label='R')
    s.body(lambda c: (c.circle(170, 190, 135), c.circle(470, 190, 135), c.rrect(170, 70, 470, 310, 20)),
           (190, 190, 200, 190), line=5)
    s.body(lambda c: c.ellipse(470, 190, 115, 95, -25), (120, 110, 150, 140), line=2)
    dpad(s, 165, 190, 58, 19)
    s.button('minus', lambda c: c.ellipse(290, 215, 22, 10, -25), fill=(90, 90, 100, 160), label='SELECT', label_size=10)
    s.button('plus', lambda c: c.ellipse(350, 215, 22, 10, -25), fill=(90, 90, 100, 160), label='START', label_size=10)
    face_diamond(s, 470, 190, 50, 26,
                 fills=((60, 70, 200, 130), (210, 40, 45, 130), (235, 200, 40, 130), (40, 160, 70, 130)))
    # the left stick also lights the cross
    for k, d in (('up', 'lsu'), ('down', 'lsd'), ('left', 'lsl'), ('right', 'lsr')):
        s.lines = [l.replace(f'btnt {k} ', f'btnt {k}|{d} ') for l in s.lines]
    return s.finish()


def nes(sid='NESHD'):
    s = Skin(sid, 640, 290)
    s.body(lambda c: c.rrect(20, 20, 620, 270, 18), (200, 200, 205, 200), line=5)
    s.body(lambda c: c.rrect(45, 60, 595, 245, 10), (35, 35, 38, 220), line=2)
    dpad(s, 150, 155, 62, 20)
    s.body(lambda c: c.rrect(250, 120, 390, 205, 10), (170, 170, 170, 200), line=2)
    s.button('minus', lambda c: c.rrect(262, 150, 312, 172, 10), fill=(25, 25, 25, 255), line=2)
    s.button('plus', lambda c: c.rrect(328, 150, 378, 172, 10), fill=(25, 25, 25, 255), line=2)
    s.label('SELECT', 287, 190, 11)
    s.label('START', 353, 190, 11)
    s.button('b|y', lambda c: c.circle(455, 160, 34), fill=(205, 30, 40, 220), label='B')
    s.button('a|x', lambda c: c.circle(545, 160, 34), fill=(205, 30, 40, 220), label='A')
    for k, d in (('up', 'lsu'), ('down', 'lsd'), ('left', 'lsl'), ('right', 'lsr')):
        s.lines = [l.replace(f'btnt {k} ', f'btnt {k}|{d} ') for l in s.lines]
    return s.finish()


def playstation(sid='PlayStationHD'):
    s = Skin(sid, 640, 470)
    s.button('zl', lambda c: c.rrect(78, 8, 228, 52, 20), fill=(0, 0, 0, 120), label='L2')
    s.button('zr', lambda c: c.rrect(412, 8, 562, 52, 20), fill=(0, 0, 0, 120), label='R2')
    s.button('l', lambda c: c.rrect(70, 46, 250, 86, 18), fill=(0, 0, 0, 120), label='L1', label_size=18, label_at=((70 + 250) / 2, 59))
    s.button('r', lambda c: c.rrect(390, 46, 570, 86, 18), fill=(0, 0, 0, 120), label='R1', label_size=18, label_at=((390 + 570) / 2, 59))

    def body(c):
        c.rrect(55, 72, 585, 270, 80)
        c.ellipse(140, 320, 70, 150, -12)
        c.ellipse(500, 320, 70, 150, 12)
        c.rrect(150, 230, 490, 330, 60)
    s.body(body, (34, 36, 48, 205), line=5)
    s.body(lambda c: c.rrect(225, 90, 415, 200, 18), (20, 22, 30, 200), line=3)
    dpad(s, 145, 165, 46, 16)
    s.stick('L', 240, 290, 50, 36, 18)
    s.stick('R', 400, 290, 50, 36, 18)
    face_diamond(s, 495, 165, 46, 22, labels=('/\\', 'O', 'X', '[]'),
                 line_colors=((90, 230, 170, 255), (240, 90, 90, 255), (130, 170, 255, 255), (240, 140, 215, 255)))
    s.button('minus', lambda c: c.rrect(196, 98, 214, 128, 6))
    s.button('plus', lambda c: c.rrect(426, 98, 444, 128, 6))
    s.button('home', lambda c: c.circle(320, 240, 14))
    return s.finish()


def minimal(sid='Minimal'):
    """Gray minimal display (L / L2 / R2 / R on top, L3 / R3 under), like the screenshot the player sent"""
    s = Skin(sid, 560, 360)
    gray = (118, 118, 122, 255)
    s.body(lambda c: c.rrect(0, 0, 560, 360, 26), (0, 0, 0, 175), line=0)
    for inputs, x0, x1, text in (('l', 16, 112, 'L'), ('zl', 124, 220, 'L2'), ('zr', 310, 406, 'R2'), ('r', 420, 528, 'R')):
        s.button(inputs, lambda c, x0=x0, x1=x1: c.rrect(x0, 8, x1, 48, 20), fill=gray, line=0, label=text, label_size=24)
    s.stick('L', 76, 126, 60, 36, 22, cap_fill=gray, well_fill=(0, 0, 0, 0), ring_color=(200, 200, 205, 255),
            cap_line=None, click=False)
    s.stick('R', 360, 238, 66, 40, 24, cap_fill=gray, well_fill=(0, 0, 0, 0), ring_color=(200, 200, 205, 255),
            cap_line=None, click=False)
    s.button('minus', lambda c: c.circle(180, 88, 12), fill=gray, line=0, label='-', label_size=18)
    s.button('capture', lambda c: c.rrect(204, 128, 226, 150, 4), fill=(200, 200, 205, 255), line=0)
    s.button('plus', lambda c: c.circle(346, 88, 12), fill=gray, line=0, label='+', label_size=18)
    s.button('home', lambda c: c.circle(310, 140, 12), fill=gray, line=0, label='^', label_size=14)
    for k, (x, y) in {'x': (440, 84), 'y': (392, 128), 'a': (486, 132), 'b': (436, 176)}.items():
        s.button(k, lambda c, x=x, y=y: c.circle(x, y, 21), fill=gray, line=0, label=k.upper(), label_size=20)
    cx, cy, arm, half = 156, 236, 46, 16
    s.body(lambda c: c.cross(cx, cy, arm, half, 4), gray, line=0)
    s.button('up', lambda c: c.rrect(cx - half, cy - arm, cx + half, cy - half, 4), line=0, label='^', label_size=14)
    s.button('down', lambda c: c.rrect(cx - half, cy + half, cx + half, cy + arm, 4), line=0, label='v', label_size=14)
    s.button('left', lambda c: c.rrect(cx - arm, cy - half, cx - half, cy + half, 4), line=0, label='<', label_size=14)
    s.button('right', lambda c: c.rrect(cx + half, cy - half, cx + arm, cy + half, 4), line=0, label='>', label_size=14)
    s.button('ls', lambda c: c.rrect(112, 300, 210, 340, 20), fill=gray, line=0, label='L3', label_size=24)
    s.button('rs', lambda c: c.rrect(296, 300, 400, 340, 20), fill=gray, line=0, label='R3', label_size=24)
    return s.finish()


if OUT.exists():
    shutil.rmtree(OUT)
print('minimal', minimal())
for f in (pro_controller, joycons, gamecube, n64, snes, nes, playstation):
    print(f.__name__, f())
print('xbox', pro_controller('XboxHD', (235, 235, 240, 70), xbox=True))
