"""Builds the 3D controllers of the Input Display style "Motion" (include/menu/MotionModels.h): Pro Controller, left and
right Joy-Con, made here from simple shapes (rounded outlines extruded with a bevel, cylinders, rounded boxes).

  py gen_motion_models.py <out.h> [preview.png]

Units: x right, y top, z out of the buttons (Pro Controller about 6 x 4, Joy-Con about 1.5 x 4.3). Vertices are written
in 1/2048 unit; each part keeps its kind (body, rail, button...) and its button (pressed color in game).
"""
import math, sys
import numpy as np

PAD = {
    'A': 'cPadIdx_A', 'B': 'cPadIdx_B', 'X': 'cPadIdx_X', 'Y': 'cPadIdx_Y', 'Plus': 'cPadIdx_Plus',
    'Minus': 'cPadIdx_Minus', 'Up': 'cPadIdx_Up', 'Down': 'cPadIdx_Down', 'Left': 'cPadIdx_Left',
    'Right': 'cPadIdx_Right', 'L': 'cPadIdx_L', 'R': 'cPadIdx_R', 'ZL': 'cPadIdx_ZL', 'ZR': 'cPadIdx_ZR',
    'StickL': 'cPadIdx_1', 'StickR': 'cPadIdx_2',
}


# --- 2D outlines (counter-clockwise) ------------------------------------------------------------------------------

def round_rect(x0, y0, x1, y1, radii, seg=6):
    """radii: bottom left, bottom right, top right, top left"""
    pts = []
    corners = [(x0 + radii[0], y0 + radii[0], math.pi), (x1 - radii[1], y0 + radii[1], 1.5 * math.pi),
               (x1 - radii[2], y1 - radii[2], 0.0), (x0 + radii[3], y1 - radii[3], 0.5 * math.pi)]
    for (cx, cy, a0), r in zip(corners, radii):
        n = seg if r > 0 else 1
        for i in range(n + 1 if r > 0 else 1):
            a = a0 + (math.pi / 2) * i / max(n, 1)
            pts.append((cx + math.cos(a) * r, cy + math.sin(a) * r))
    return pts


def circle(cx, cy, r, n=16):
    return [(cx + math.cos(2 * math.pi * i / n) * r, cy + math.sin(2 * math.pi * i / n) * r) for i in range(n)]


def smooth(pts, iterations=2):
    """Chaikin corner cutting of a closed outline"""
    for _ in range(iterations):
        out = []
        for i in range(len(pts)):
            p, q = np.array(pts[i]), np.array(pts[(i + 1) % len(pts)])
            out += [tuple(0.75 * p + 0.25 * q), tuple(0.25 * p + 0.75 * q)]
        pts = out
    return pts


def area(pts):
    return sum(pts[i][0] * pts[(i + 1) % len(pts)][1] - pts[(i + 1) % len(pts)][0] * pts[i][1] for i in range(len(pts))) / 2


def inset(pts, d):
    """outline moved inward by d (vertex normals)"""
    n = len(pts)
    out = []
    for i in range(n):
        p0, p1, p2 = np.array(pts[i - 1]), np.array(pts[i]), np.array(pts[(i + 1) % n])
        e0, e1 = p1 - p0, p2 - p1
        n0 = np.array([e0[1], -e0[0]]) / (np.linalg.norm(e0) or 1)
        n1 = np.array([e1[1], -e1[0]]) / (np.linalg.norm(e1) or 1)
        nn = n0 + n1
        nn /= np.linalg.norm(nn) or 1
        out.append(tuple(p1 - nn * d))
    return out


def triangulate(pts):
    """ear clipping of a counter-clockwise simple polygon -> index triangles"""
    idx = list(range(len(pts)))
    P = [np.array(p) for p in pts]
    tris = []

    def inside(p, a, b, c):
        def s(u, v, w):
            return (v[0] - u[0]) * (w[1] - u[1]) - (v[1] - u[1]) * (w[0] - u[0])
        return s(a, b, p) >= 0 and s(b, c, p) >= 0 and s(c, a, p) >= 0

    guard = 0
    while len(idx) > 3 and guard < 10000:
        guard += 1
        for k in range(len(idx)):
            i0, i1, i2 = idx[k - 1], idx[k], idx[(k + 1) % len(idx)]
            a, b, c = P[i0], P[i1], P[i2]
            if (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]) <= 1e-12:
                continue
            if any(inside(P[j], a, b, c) for j in idx if j not in (i0, i1, i2)):
                continue
            tris.append((i0, i1, i2))
            idx.pop(k)
            break
        else:
            break
    if len(idx) == 3:
        tris.append(tuple(idx))
    return tris


# --- 3D shapes ------------------------------------------------------------------------------------------------------

class Mesh:
    def __init__(self):
        self.v, self.t = [], []

    def add(self, verts, tris):
        base = len(self.v)
        self.v += [tuple(x) for x in verts]
        self.t += [(a + base, b + base, c + base) for a, b, c in tris]


def extrude(outline, layers):
    """outline (counter-clockwise) extruded through layers [(z, inset)], z rising; caps at both ends"""
    if area(outline) < 0:
        outline = outline[::-1]
    m = Mesh()
    n = len(outline)
    rings = []
    for z, d in layers:
        ring = inset(outline, d) if d else outline
        rings.append([(x, y, z) for x, y in ring])
    verts = [p for r in rings for p in r]
    tris = []
    for L in range(len(rings) - 1):
        for i in range(n):
            j = (i + 1) % n
            a0, b0, a1, b1 = L * n + i, L * n + j, (L + 1) * n + i, (L + 1) * n + j
            tris += [(a0, b0, b1), (a0, b1, a1)]
    cap = triangulate([(p[0], p[1]) for p in rings[-1]])
    top = (len(rings) - 1) * n
    tris += [(top + a, top + b, top + c) for a, b, c in cap]
    cap = triangulate([(p[0], p[1]) for p in rings[0]])
    tris += [(c, b, a) for a, b, c in cap]
    m.add(verts, tris)
    return m


def bevel_layers(z0, z1, bevel, steps=2):
    """layers of a slab from z0 to z1 whose two faces are rounded by bevel"""
    out = []
    for i in range(steps + 1):
        a = (math.pi / 2) * i / steps
        out.append((z0 + bevel * (1 - math.sin(a)), bevel * (1 - math.cos(a))))
    for i in range(steps + 1):
        a = (math.pi / 2) * i / steps
        out.append((z1 - bevel * (1 - math.cos(a)), bevel * (1 - math.sin(a))))
    return out


def cylinder(cx, cy, r, z0, z1, n=16, top_bevel=0.0):
    layers = [(z0, 0), (z1 - top_bevel, 0)] + ([(z1, top_bevel)] if top_bevel else [])
    return extrude(circle(cx, cy, r, n), layers)


def rbox(x0, y0, x1, y1, z0, z1, r, bevel):
    return extrude(round_rect(x0, y0, x1, y1, [r] * 4, 4), bevel_layers(z0, z1, bevel, 1))


# --- the three controllers ------------------------------------------------------------------------------------------

def pro():
    parts = []
    # Body: a wide top part and two grips, as one outline (pixels of a 1270 x 720 drawing, then scaled to 5.9 wide)
    px = [(470, 55), (560, 48), (700, 48), (790, 55), (828, 85), (847, 160), (860, 260), (862, 330), (850, 385),
          (815, 398), (788, 370), (760, 305), (735, 288), (525, 288), (500, 305), (472, 370), (445, 398),
          (410, 385), (398, 330), (400, 260), (413, 160), (432, 85)]
    s = 5.9 / (862 - 398)
    outline = smooth([((x - 630) * s, (222 - y) * s) for x, y in px], 2)
    parts.append(('Body', None, extrude(outline, bevel_layers(-1.05, 0.3, 0.28, 3))))
    # Sticks: base well, then the stick
    for name, (x, y) in (('StickL', (-1.66, 0.83)), ('StickR', (0.78, 0.01))):
        parts.append(('Face', None, cylinder(x, y, 0.42, 0.2, 0.34, 20)))
        parts.append(('Stick', name, cylinder(x, y, 0.33, 0.25, 0.68, 20, 0.08)))
    # A B X Y
    for name, (x, y) in (('A', (2.03, 0.83)), ('B', (1.56, 0.42)), ('X', (1.56, 1.24)), ('Y', (1.09, 0.83))):
        parts.append(('Button', name, cylinder(x, y, 0.22, 0.2, 0.47, 14, 0.05)))
    # D-Pad: four arms
    cx, cy, w, l = -0.94, 0.01, 0.13, 0.3
    for name, (x0, y0, x1, y1) in (('Up', (cx - w, cy + w, cx + w, cy + l)), ('Down', (cx - w, cy - l, cx + w, cy - w)),
                                   ('Left', (cx - l, cy - w, cx - w, cy + w)), ('Right', (cx + w, cy - w, cx + l, cy + w))):
        parts.append(('Button', name, rbox(x0, y0, x1, y1, 0.2, 0.43, 0.03, 0.03)))
    parts.append(('Face', None, rbox(cx - w, cy - w, cx + w, cy + w, 0.2, 0.4, 0.0, 0.0)))
    # + and -
    parts.append(('Button', 'Plus', cylinder(0.73, 1.28, 0.13, 0.2, 0.35, 12)))
    parts.append(('Button', 'Minus', cylinder(-0.75, 1.28, 0.13, 0.2, 0.35, 12)))
    # Home / Capture
    parts.append(('Face', None, cylinder(0.3, 0.75, 0.11, 0.2, 0.33, 12)))
    parts.append(('Face', None, rbox(-0.42, 0.66, -0.24, 0.84, 0.2, 0.33, 0.02, 0.0)))
    # Shoulder buttons: L / R on the top edge, ZL / ZR behind
    for side, (b1, b2) in ((-1, ('L', 'ZL')), (1, ('R', 'ZR'))):
        x0, x1 = sorted((side * 0.8, side * 2.15))
        parts.append(('Shoulder', b1, rbox(x0, 1.95, x1, 2.38, -0.42, 0.08, 0.18, 0.06)))
        x0, x1 = sorted((side * 1.0, side * 2.2))
        parts.append(('Shoulder', b2, rbox(x0, 1.85, x1, 2.25, -1.0, -0.48, 0.16, 0.06)))
    return parts


def joycon(isLeft):
    parts = []
    m = 1 if isLeft else -1
    # Body: rounded on the outer side, rail on the inner side
    x0, x1 = sorted((m * -0.76, m * 0.62))
    outer, inner = 0.68, 0.08
    radii = [outer, inner, inner, outer] if isLeft else [inner, outer, outer, inner]
    parts.append(('Body', None, extrude(round_rect(x0, -2.12, x1, 2.12, radii, 8), bevel_layers(-0.62, 0.28, 0.14, 2))))
    rx0, rx1 = sorted((m * 0.6, m * 0.74))
    parts.append(('Dark', None, rbox(rx0, -1.75, rx1, 1.9, -0.45, 0.17, 0.04, 0.02)))
    if isLeft:
        parts.append(('Face', None, cylinder(-0.02, 1.03, 0.4, 0.15, 0.31, 20)))
        parts.append(('Stick', 'StickL', cylinder(-0.02, 1.03, 0.31, 0.2, 0.62, 20, 0.08)))
        for name, (x, y) in (('Up', (-0.02, 0.15)), ('Down', (-0.02, -0.48)), ('Left', (-0.34, -0.17)), ('Right', (0.3, -0.17))):
            parts.append(('Button', name, cylinder(x, y, 0.14, 0.2, 0.36, 12, 0.03)))
        parts.append(('Button', 'Minus', rbox(0.29, 1.67, 0.52, 1.74, 0.2, 0.33, 0.02, 0.0)))
        parts.append(('Face', None, rbox(0.1, -1.15, 0.3, -0.95, 0.2, 0.32, 0.02, 0.0)))  # Capture
    else:
        parts.append(('Face', None, cylinder(0.02, -0.2, 0.4, 0.15, 0.31, 20)))
        parts.append(('Stick', 'StickR', cylinder(0.02, -0.2, 0.31, 0.2, 0.62, 20, 0.08)))
        for name, (x, y) in (('X', (0.02, 1.3)), ('B', (0.02, 0.66)), ('Y', (-0.3, 0.98)), ('A', (0.34, 0.98))):
            parts.append(('Button', name, cylinder(x, y, 0.14, 0.2, 0.36, 12, 0.03)))
        parts.append(('Button', 'Plus', rbox(-0.52, 1.67, -0.29, 1.74, 0.2, 0.33, 0.02, 0.0)))
        parts.append(('Button', 'Plus', rbox(-0.44, 1.59, -0.37, 1.82, 0.2, 0.33, 0.02, 0.0)))
        parts.append(('Face', None, cylinder(-0.2, -1.05, 0.12, 0.2, 0.32, 12)))  # Home
    b1, b2 = ('L', 'ZL') if isLeft else ('R', 'ZR')
    sx0, sx1 = sorted((m * -0.55, m * 0.45))
    parts.append(('Shoulder', b1, rbox(sx0, 2.0, sx1, 2.3, -0.12, 0.16, 0.12, 0.05)))
    sx0, sx1 = sorted((m * -0.45, m * 0.45))
    parts.append(('Shoulder', b2, rbox(sx0, 1.95, sx1, 2.25, -0.6, -0.25, 0.12, 0.05)))
    return parts


# --- output ---------------------------------------------------------------------------------------------------------

def emit(name, parts, out):
    V, T, info = [], [], []
    for kind, btn, mesh in parts:
        base = len(V)
        info.append((kind, btn, len(T), len(mesh.t)))
        V += mesh.v
        T += [(a + base, b + base, c + base) for a, b, c in mesh.t]
    q = np.round(np.array(V) * 2048).astype(np.int64)
    assert len(V) < 2048 and len(T) < 4096 and np.abs(q).max() < 32767, (name, len(V), len(T))
    out.append(f'// {name}: {len(V)} vertices, {len(T)} triangles')
    out.append(f'constexpr int16_t c{name}Verts[][3] = {{')
    for i in range(0, len(q), 6):
        out.append('    ' + ' '.join('{%d, %d, %d},' % tuple(p) for p in q[i:i + 6]))
    out.append('};')
    out.append(f'constexpr uint16_t c{name}Tris[][3] = {{')
    for i in range(0, len(T), 8):
        out.append('    ' + ' '.join('{%d, %d, %d},' % t for t in T[i:i + 8]))
    out.append('};')
    out.append(f'constexpr MotionModelPart c{name}Parts[] = {{')
    for kind, btn, ts, tn in info:
        pad = f'al::NpadController::{PAD[btn]}' if btn else '-1'
        out.append(f'    {{ MotionPartKind::{kind}, {pad}, {ts}, {tn} }},')
    out.append('};')
    return len(T)


def preview(models, path):
    from PIL import Image, ImageDraw
    img = Image.new('RGB', (1200, 420), (0, 160, 0))
    d = ImageDraw.Draw(img)
    col = {'Body': (45, 46, 50), 'Dark': (25, 25, 28), 'Face': (25, 25, 28), 'Stick': (160, 160, 160),
           'Button': (232, 232, 232), 'Shoulder': (232, 232, 232)}
    for parts, cx, sc, rot in models:
        tris = []
        for kind, btn, mesh in parts:
            r = np.array(mesh.v, float)
            ca, sa = math.cos(rot), math.sin(rot)
            r[:, 0], r[:, 2] = r[:, 0] * ca + r[:, 2] * sa, -r[:, 0] * sa + r[:, 2] * ca
            cb, sb = math.cos(0.45), math.sin(0.45)
            r[:, 1], r[:, 2] = r[:, 1] * cb - r[:, 2] * sb, r[:, 1] * sb + r[:, 2] * cb
            if kind in ('Face', 'Button', 'Stick') and math.cos(rot) * cb <= 0.05:
                continue
            for a, b, c in mesh.t:
                p = r[[a, b, c]]
                n = np.cross(p[1] - p[0], p[2] - p[0])
                if n[2] <= 0:
                    continue
                light = 0.62 + 0.38 * n[2] / (np.linalg.norm(n) or 1)
                front = kind in ('Face', 'Button', 'Stick')
                tris.append(((1 if front else 0, p[:, 2].mean()), [(cx + q[0] * sc, 210 - q[1] * sc) for q in p],
                             tuple(int(v * light) for v in col[kind])))
        for _, pts, c in sorted(tris, key=lambda t: (t[0][0], t[0][1])):
            d.polygon(pts, fill=c)
    img.save(path)


P, JL, JR = pro(), joycon(True), joycon(False)
out = ['#pragma once', '', '// Generated by tools/subareas/gen_motion_models.py: the controllers of the Input Display style "Motion",',
       '// made from simple shapes. Vertices in 1/2048 unit: x right, y top, z out of the buttons.', '',
       '#include <cstdint>', '#include "al/Library/Controller/NpadController.h"', '', 'namespace btt {', '',
       'enum class MotionPartKind : uint8_t { Body, Dark, Face, Stick, Button, Shoulder };', '',
       'struct MotionModelPart {', '    MotionPartKind kind;', '    int padIdx;', '    int triStart;', '    int triNum;', '};', '']
n = [emit('Pro', P, out), emit('JoyL', JL, out), emit('JoyR', JR, out)]
out += ['', '} // namespace btt', '']
open(sys.argv[1], 'w', encoding='utf-8', newline='\n').write('\n'.join(out))
print('tris', n)
if len(sys.argv) > 2:
    preview([(P, 250, 60, 0.35), (JL, 700, 70, 0.35), (JR, 900, 70, 0.35)], sys.argv[2])
    if len(sys.argv) > 3:
        preview([(P, 250, 60, 0.0), (JL, 700, 70, 0.0), (JR, 900, 70, 0.0)], sys.argv[3])
