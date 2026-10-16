"""Controller Color / Controller Pattern / Controller Pressed Color of the input display skins.

For each controller skin with a body (KEEP_BODY), adds to its pack (romfs/ImGuiData/Skins/<id>.pack):
  - body.astc: the body of the controller (without its buttons and sticks) in gray, keeping its shading: drawn over the
    background tinted with the Controller Color ("body b x y w h" in skin.txt);
  - one picture per pattern (the designs of the SCUF styles: Tesla, Supernova...), stretched over the body with the
    shading of the controller ("pat <Name> pN x y w h");
and to every skin a gray copy of each pressed button picture ("btng <inputs> gN x y w h"), tinted with the Controller
Pressed Color.

  py gen_skin_variants.py <romfs/ImGuiData/Skins> [preview.png]

Needs the source packs of the patterns (PATTERNS) in the same folder; run before removing them.
"""
import pathlib, sys
import numpy as np
import cv2
from PIL import Image

sys.path.insert(0, str(pathlib.Path(__file__).parent))
from skin_packs import read_pack, write_pack, raw_astc, resolve_ref, decode, encode  # noqa: E402

# Controllers whose body can be recolored / get a pattern
KEEP_BODY = ['OJD019', 'OJD094', 'OJD099', 'OJD114', 'OJD136', 'OJD196', 'OJD197']
# Every skin kept (pressed color for all of them)
KEEP_ALL = ['OJD003', 'OJD009', 'OJD019', 'OJD027', 'OJD094', 'OJD099', 'OJD114', 'OJD115', 'OJD122', 'OJD136',
            'OJD196', 'OJD197']
# Patterns: name -> SCUF style it comes from (same layout as OJD136)
PATTERNS = {
    'Carbon': 'OJD148', 'Camo': 'OJD149', 'Galaxy': 'OJD150', 'Nebula': 'OJD152', 'Supernova': 'OJD157',
    'Star Storm': 'OJD156', 'Tesla': 'OJD158', 'Honeycomb': 'OJD145', 'Honeycomb White': 'OJD146',
    'Apex Legends': 'OJD147', 'Knights of SCUF': 'OJD151', 'Night': 'OJD153', 'Reaper': 'OJD154',
    'Sand Storm': 'OJD155', 'Utopia': 'OJD159',
}
PATTERN_LAYOUT = 'OJD136'


SKINS = None


class SkinPics:
    def __init__(self, files):
        self.files = files
        self.lines = files['skin.txt'].decode().splitlines()
        self.bg = np.asarray(decode(raw_astc(files['bg.astc'], SKINS))).astype(np.float32) / 255
        self.btns, self.sticks = [], []
        for line in self.lines:
            p = line.split()
            if p and p[0] in ('btn', 'btnt'):
                self.btns.append((p[1], p[2], *map(float, p[3:7])))
            elif p and p[0] == 'stick':
                self.sticks.append((p[1], p[2], *map(float, p[3:7])))

    def pic(self, name):
        return np.asarray(decode(raw_astc(self.files[name + '.astc'], SKINS))).astype(np.float32) / 255

    def placed_alpha(self, name, x, y, w, h):
        """alpha of a picture drawn at x, y, w, h over the background"""
        H, W = self.bg.shape[:2]
        out = np.zeros((H, W), np.float32)
        img = self.pic(name)
        iw, ih = max(1, round(w)), max(1, round(h))
        a = cv2.resize(img[..., 3], (iw, ih), interpolation=cv2.INTER_LINEAR)
        x0, y0 = round(x), round(y)
        xs, ys = max(0, x0), max(0, y0)
        xe, ye = min(W, x0 + iw), min(H, y0 + ih)
        if xe > xs and ye > ys:
            out[ys:ye, xs:xe] = a[ys - y0:ye - y0, xs - x0:xe - x0]
        return out

    def body_mask(self):
        """the controller without its buttons and sticks (0..1). A pressed button picture covers a whole rectangle: only
        the pixels it changes are the button."""
        mask = self.bg[..., 3].copy()
        holes = np.zeros_like(mask)
        H, W = mask.shape
        for _, img, x, y, w, h in self.btns:
            pic = self.pic(img)
            iw, ih = max(1, round(w)), max(1, round(h))
            p = cv2.resize(pic, (iw, ih), interpolation=cv2.INTER_LINEAR)
            x0, y0 = round(x), round(y)
            xs, ys, xe, ye = max(0, x0), max(0, y0), min(W, x0 + iw), min(H, y0 + ih)
            if xe <= xs or ye <= ys:
                continue
            p = p[ys - y0:ye - y0, xs - x0:xe - x0]
            under = self.bg[ys:ye, xs:xe]
            shown = p[..., :3] * p[..., 3:] + under[..., :3] * (1 - p[..., 3:])
            changed = (np.abs(shown - under[..., :3]).max(axis=2) > 0.08).astype(np.float32)
            holes[ys:ye, xs:xe] = np.maximum(holes[ys:ye, xs:xe], changed)
        for _, img, x, y, w, h in self.sticks:
            holes = np.maximum(holes, (self.placed_alpha(img, x, y, w, h) > 0.25).astype(np.float32))
        holes = cv2.dilate((holes > 0.5).astype(np.uint8), np.ones((3, 3), np.uint8)).astype(np.float32)
        return mask * (1 - holes)

    def luminance(self):
        c = self.bg[..., :3]
        return 0.299 * c[..., 0] + 0.587 * c[..., 1] + 0.114 * c[..., 2]


def bbox(mask):
    ys, xs = np.nonzero(mask > 0.05)
    return xs.min(), ys.min(), xs.max() + 1, ys.max() + 1


def shade_of(sk, mask):
    lum = sk.luminance()
    med = np.median(lum[mask > 0.5]) if (mask > 0.5).any() else 0.5
    return lum / max(med, 0.08)


def to_image(rgb, alpha, box):
    x0, y0, x1, y1 = box
    out = np.dstack([np.clip(rgb, 0, 1), np.clip(alpha, 0, 1)])[y0:y1, x0:x1]
    return Image.fromarray((out * 255 + 0.5).astype(np.uint8), 'RGBA')


def largest_rect(valid):
    """largest rectangle (x0, y0, x1, y1) where valid is true everywhere"""
    H, W = valid.shape
    heights = np.zeros(W, np.int32)
    best, box = 0, (0, 0, 1, 1)
    for y in range(H):
        heights = np.where(valid[y], heights + 1, 0)
        stack = []
        for x in range(W + 1):
            h = heights[x] if x < W else 0
            start = x
            while stack and stack[-1][1] >= h:
                start, sh = stack.pop()
                if sh * (x - start) > best:
                    best, box = sh * (x - start), (start, y - sh + 1, x, y + 1)
            stack.append((start, h))
    return box


class PatternTexture:
    """the design of a SCUF style over its whole body box: what every SCUF style has (sticks, touchpad, logo) and the
    space around the body are filled with the largest clean rectangle of the design, repeated as a mirror"""

    def __init__(self, src, fixed):
        if fixed.shape != src.bg.shape[:2]:
            fixed = cv2.resize(fixed, (src.bg.shape[1], src.bg.shape[0]), interpolation=cv2.INTER_NEAREST)
        mask = src.body_mask() * (1 - fixed)
        valid = cv2.erode((mask > 0.95).astype(np.uint8), np.ones((3, 3), np.uint8)) > 0
        rx0, ry0, rx1, ry1 = largest_rect(valid)
        tile = src.bg[ry0:ry1, rx0:rx1, :3]
        bx0, by0, bx1, by1 = bbox(src.body_mask())
        w, h = bx1 - bx0, by1 - by0
        filler = mirror_tile(tile, w, h, rx0 - bx0, ry0 - by0)
        design = src.bg[by0:by1, bx0:bx1, :3]
        keep = cv2.GaussianBlur(valid[by0:by1, bx0:bx1].astype(np.float32), (7, 7), 0)[..., None]
        self.texture = design * keep + filler * (1 - keep)

    def cover(self, w, h, targetBodyW=None):
        return cv2.resize(self.texture, (w, h), interpolation=cv2.INTER_AREA)


def mirror_tile(tile, w, h, ox, oy):
    """tile repeated as a mirror over w x h, its first copy at (ox, oy)"""
    th, tw = tile.shape[:2]
    block = np.concatenate([np.concatenate([tile, tile[:, ::-1]], axis=1),
                            np.concatenate([tile[::-1], tile[::-1, ::-1]], axis=1)], axis=0)
    ys = (np.arange(h) - oy) % (2 * th)
    xs = (np.arange(w) - ox) % (2 * tw)
    return block[ys][:, xs]


def scuf_fixed(skins):
    """pixels that look the same in every SCUF style (the controller itself, not its design)"""
    lums = []
    shape = None
    for n in range(135, 160):
        p = skins / f'OJD{n:03d}.pack'
        if p.exists():
            lum = SkinPics(read_pack(p)).luminance()
            shape = shape or lum.shape
            if lum.shape != shape:
                lum = cv2.resize(lum, (shape[1], shape[0]), interpolation=cv2.INTER_LINEAR)
            lums.append(lum)
    std = np.std(np.stack(lums), axis=0)
    fixed = (std < 0.05).astype(np.uint8)
    return cv2.dilate(fixed, np.ones((5, 5), np.uint8)).astype(np.float32)


def main():
    global SKINS
    skins = SKINS = pathlib.Path(sys.argv[1])
    textures = {}
    fixed = scuf_fixed(skins)
    for name, sid in PATTERNS.items():
        textures[name] = PatternTexture(SkinPics(read_pack(skins / f'{sid}.pack')), fixed)
    preview = []
    for sid in KEEP_ALL:
        files = read_pack(skins / f'{sid}.pack')
        sk = SkinPics(files)
        lines = [l for l in sk.lines if l.split()[:1] not in (['body'], ['pat'], ['btng'])]
        files = {k: v for k, v in files.items() if not (k.startswith('g') or k.startswith('p') or k == 'b.astc')}
        # pictures shared with other styles: copied here (those styles are removed)
        files = {k: resolve_ref(v, skins) for k, v in files.items()}
        # gray pressed buttons
        for i, (inputs, img, x, y, w, h) in enumerate(sk.btns):
            pic = sk.pic(img)
            lum = pic[..., :3] @ np.array([0.299, 0.587, 0.114], np.float32)
            gray = np.clip(lum / max(np.percentile(lum[pic[..., 3] > 0.3], 90) if (pic[..., 3] > 0.3).any() else 1, 0.1), 0, 1)
            g = Image.fromarray((np.dstack([gray, gray, gray, pic[..., 3]]) * 255 + 0.5).astype(np.uint8), 'RGBA')
            files[f'g{i}.astc'] = encode(g, '4x4')
            lines.append(f'btng {inputs} g{i} {x:g} {y:g} {w:g} {h:g}')
        if sid in KEEP_BODY:
            mask = sk.body_mask()
            box = bbox(mask)
            shade = shade_of(sk, mask)
            gray = np.clip(shade * 0.85, 0, 1)
            files['b.astc'] = encode(to_image(np.dstack([gray] * 3), mask, box), '6x6')
            bx0, by0, bx1, by1 = box
            lines.append(f'body b {bx0} {by0} {bx1 - bx0} {by1 - by0}')
            H, W = mask.shape
            k = 0
            for name, tex in textures.items():
                # repeated over the body box, with the shading of the controller
                t = tex.cover(bx1 - bx0, by1 - by0, bx1 - bx0)
                full = np.zeros((H, W, 3), np.float32)
                full[by0:by1, bx0:bx1] = t
                rgb = full * np.clip(shade, 0.55, 1.35)[..., None]
                # 75 % of the size, 8x8 blocks: about 13 KB a picture (drawn stretched over the body box)
                pat = to_image(rgb, mask, box)
                pat = pat.resize((max(8, round(pat.width * 0.75)), max(8, round(pat.height * 0.75))), Image.LANCZOS)
                files[f'p{k}.astc'] = encode(pat, '8x8')
                lines.append(f'pat {name.replace(" ", "_")} p{k} {bx0} {by0} {bx1 - bx0} {by1 - by0}')
                if sid in ('OJD094', 'OJD019', 'OJD197') and name in ('Tesla', 'Galaxy', 'Carbon'):
                    bgimg = Image.fromarray((sk.bg * 255).astype(np.uint8), 'RGBA')
                    bgimg.alpha_composite(to_image(rgb, mask, box), (bx0, by0))
                    preview.append((f'{sid} {name}', bgimg))
                k += 1
            if sid in ('OJD094', 'OJD019', 'OJD197', 'OJD099', 'OJD114', 'OJD136'):
                bgimg = Image.fromarray((sk.bg * 255).astype(np.uint8), 'RGBA')
                col = np.array([0.9, 0.15, 0.15], np.float32)
                bgimg.alpha_composite(to_image(np.dstack([gray] * 3) * col, mask, box), (bx0, by0))
                preview.append((f'{sid} Red', bgimg))
        files['skin.txt'] = ('\n'.join(lines) + '\n').encode()
        write_pack(skins / f'{sid}.pack', files)
        print(sid, len(files), 'files', sum(len(v) for v in files.values()) // 1024, 'KB', flush=True)
    if len(sys.argv) > 2:
        W = 5
        sheet = Image.new('RGBA', (W * 370, ((len(preview) + W - 1) // W) * 250), (0, 140, 0, 255))
        from PIL import ImageDraw
        d = ImageDraw.Draw(sheet)
        for i, (label, img) in enumerate(preview):
            img = img.copy()
            img.thumbnail((360, 220))
            x, y = (i % W) * 370, (i // W) * 250
            sheet.alpha_composite(img, (x + 5, y + 24))
            d.text((x + 5, y + 4), label, fill=(255, 255, 255, 255))
        sheet.save(sys.argv[2])


if __name__ == '__main__':
    main()
