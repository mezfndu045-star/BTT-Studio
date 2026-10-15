"""glb (HOS-InputDisplay models) -> simplified meshes for the Motion input display (include/menu/MotionModels.h).

Each part (mesh) keeps its button: body, rail, D-Pad, sticks, A B X Y, + -, L R ZL ZR. Simplified by vertex clustering
(cell size per model), vertices quantized to int16 (1/2048 unit).

  py glb2model.py <Controller.glb> <JoyL.glb> <JoyR.glb> <out.h> [preview.png]
"""
import json, struct, sys
import numpy as np

# part kinds (match MotionDisplay.cpp)
KINDS = ['Body', 'Dark', 'Face', 'Stick', 'Button', 'Shoulder']
BUTTONS = {  # name -> sead::Controller pad index
    'A': 'cPadIdx_A', 'B': 'cPadIdx_B', 'X': 'cPadIdx_X', 'Y': 'cPadIdx_Y', 'Plus': 'cPadIdx_Plus',
    'Minus': 'cPadIdx_Minus', 'DUp': 'cPadIdx_Up', 'DDown': 'cPadIdx_Down', 'DLeft': 'cPadIdx_Left',
    'DRight': 'cPadIdx_Right', 'L': 'cPadIdx_L', 'R': 'cPadIdx_R', 'ZL': 'cPadIdx_ZL', 'ZR': 'cPadIdx_ZR',
    'StickL': 'cPadIdx_1', 'StickR': 'cPadIdx_2',
}


def load(path):
    b = open(path, 'rb').read()
    n = struct.unpack_from('<I', b, 12)[0]
    j = json.loads(b[20:20 + n])
    binoff = 20 + n + 8
    out = []
    for node in j['nodes']:
        if 'mesh' not in node:
            continue
        for pr in j['meshes'][node['mesh']]['primitives']:
            def acc(i, comps):
                a = j['accessors'][i]
                bv = j['bufferViews'][a['bufferView']]
                off = binoff + bv.get('byteOffset', 0) + a.get('byteOffset', 0)
                dt = {5126: np.float32, 5123: np.uint16, 5125: np.uint32, 5121: np.uint8}[a['componentType']]
                return np.frombuffer(b, dt, a['count'] * comps, off).reshape(a['count'], comps)
            pos = acc(pr['attributes']['POSITION'], 3).astype(np.float64)
            idx = acc(pr['indices'], 1).reshape(-1, 3).astype(np.int64)
            mat = j['materials'][pr['material']]['name'] if 'material' in pr else ''
            out.append((node['name'], mat, pos, idx))
    return out


def simplify(pos, idx, cell):
    key = np.floor(pos / cell).astype(np.int64)
    uniq, inv = np.unique(key, axis=0, return_inverse=True)
    inv = inv.reshape(-1)
    sums = np.zeros((len(uniq), 3))
    np.add.at(sums, inv, pos)
    cnt = np.bincount(inv, minlength=len(uniq))[:, None]
    verts = sums / cnt
    tris = inv[idx]
    ok = (tris[:, 0] != tris[:, 1]) & (tris[:, 1] != tris[:, 2]) & (tris[:, 0] != tris[:, 2])
    tris = tris[ok]
    # drop duplicated triangles (same 3 vertices, same winding)
    rolled = np.array([np.roll(t, -int(np.argmin(t))) for t in tris]) if len(tris) else tris
    if len(tris):
        _, keep = np.unique(rolled, axis=0, return_index=True)
        tris = tris[np.sort(keep)]
    used = np.unique(tris)
    remap = -np.ones(len(verts), np.int64)
    remap[used] = np.arange(len(used))
    return verts[used], remap[tris]


def classify(model, node, mat):
    name = node.split('.')[0]
    if model == 'pro':
        m = mat.split(':')[-1].split('.')[0]
        if m in ('Base',):
            return 'Body', None
        if m == 'BaseDPadSG':
            return 'Face', None
        if m in ('StickL', 'StickR'):
            return 'Stick', m
        if m in ('ZL', 'ZR', 'L', 'R'):
            return 'Shoulder', m
        if m in BUTTONS:
            return 'Button', m
        return 'Body', None
    # Joy-Con
    side = 'L' if model == 'joyl' else 'R'
    if name == 'main_body':
        return 'Body', None
    if name in ('connect_thingie', 'connect', 'screw1', 'screw2', 'screw3'):
        return 'Dark', None
    if name == 'Stick':
        return 'Stick', 'StickL' if side == 'L' else 'StickR'
    if name in ('SL', 'SR'):
        return 'Dark', None
    if name in ('ZL', 'L'):
        # the right Joy-Con model reuses the left names
        return 'Shoulder', name if side == 'L' else name.replace('L', 'R')
    if name == 'Minus':
        return 'Button', 'Minus' if side == 'L' else 'Plus'
    if side == 'L':
        return 'Button', name
    # right Joy-Con: D-Pad names = A B X Y by position
    return 'Button', {'DUp': 'X', 'DDown': 'B', 'DLeft': 'Y', 'DRight': 'A'}[name]


def decimate(pos, idx, target, kind):
    # light vertex clustering (welds and cleans), then quadric simplification down to target triangles
    v, t = simplify(pos, idx, 0.03 if kind in ('Body', 'Dark') else 0.01)
    if len(t) <= target:
        return v, t
    import pyfqmr
    s = pyfqmr.Simplify()
    s.setMesh(v.astype(np.float64), t.astype(np.int32))
    s.simplify_mesh(target_count=target, aggressiveness=7, preserve_border=False, verbose=False)
    v, t, _ = s.getMesh()
    return np.asarray(v, np.float64), np.asarray(t, np.int64)


def convert(model, path, cells):
    parts = []
    for node, mat, pos, idx in load(path):
        kind, btn = classify(model, node, mat)
        v, t = decimate(pos, idx, cells.get(kind, cells['default']), kind)
        if len(t) == 0:
            continue
        parts.append((kind, btn, v, t, node))
    return parts


def emit(name, parts, out):
    allv, allt, pinfo = [], [], []
    for kind, btn, v, t, node in parts:
        base = sum(len(x) for x in allv)
        tstart = sum(len(x) for x in allt)
        allv.append(v)
        allt.append(t + base)
        pinfo.append((kind, btn, tstart, len(t), node))
    V = np.concatenate(allv)
    T = np.concatenate(allt)
    assert len(V) < 65536
    q = np.round(V * 2048).astype(np.int64)
    assert np.abs(q).max() < 32767
    out.append(f'// {name}: {len(V)} vertices, {len(T)} triangles')
    out.append(f'constexpr int16_t c{name}Verts[][3] = {{')
    for i in range(0, len(q), 6):
        out.append('    ' + ' '.join('{%d, %d, %d},' % tuple(p) for p in q[i:i + 6]))
    out.append('};')
    out.append(f'constexpr uint16_t c{name}Tris[][3] = {{')
    for i in range(0, len(T), 8):
        out.append('    ' + ' '.join('{%d, %d, %d},' % tuple(t) for t in T[i:i + 8]))
    out.append('};')
    out.append(f'constexpr MotionModelPart c{name}Parts[] = {{')
    for kind, btn, ts, tn, node in pinfo:
        pad = f'al::NpadController::{BUTTONS[btn]}' if btn else '-1'
        out.append(f'    {{ MotionPartKind::{kind}, {pad}, {ts}, {tn} }}, // {node}')
    out.append('};')
    return len(T)


def preview(parts_list, path):
    from PIL import Image, ImageDraw
    img = Image.new('RGB', (1200, 420), (0, 160, 0))
    d = ImageDraw.Draw(img)
    col = {'Body': (45, 45, 48), 'Dark': (25, 25, 28), 'Face': (40, 40, 44), 'Stick': (160, 160, 160),
           'Button': (232, 232, 232), 'Shoulder': (232, 232, 232)}
    for (parts, cx, sc, body, rot) in parts_list:
        tris = []
        ca, sa = np.cos(rot), np.sin(rot)
        for kind, btn, v, t, node in parts:
            # rotate around y then x a bit to see the 3D
            r = v.copy()
            x, z = r[:, 0] * ca + r[:, 2] * sa, -r[:, 0] * sa + r[:, 2] * ca
            r[:, 0], r[:, 2] = x, z
            y, z = r[:, 1] * np.cos(0.5) - r[:, 2] * np.sin(0.5), r[:, 1] * np.sin(0.5) + r[:, 2] * np.cos(0.5)
            r[:, 1], r[:, 2] = y, z
            c = body if kind == 'Body' else col[kind]
            for a, b_, c_ in t:
                p = r[[a, b_, c_]]
                n = np.cross(p[1] - p[0], p[2] - p[0])
                if n[2] <= 0:
                    continue
                ln = np.linalg.norm(n)
                light = 0.6 + 0.4 * (n[2] / ln if ln else 0)
                tris.append((p[:, 2].mean(), [(cx + pp[0] * sc, 210 - pp[1] * sc) for pp in p], tuple(int(cc * light) for cc in c)))
        for _, pts, c in sorted(tris, key=lambda x: x[0]):
            d.polygon(pts, fill=c)
    img.save(path)


# target triangles per part
pro = convert('pro', sys.argv[1], {'default': 80, 'Body': 1100, 'Stick': 140, 'Shoulder': 70, 'Face': 10})
joyl = convert('joyl', sys.argv[2], {'default': 80, 'Body': 1100, 'Dark': 80, 'Stick': 140, 'Shoulder': 70})
joyr = convert('joyr', sys.argv[3], {'default': 80, 'Body': 1100, 'Dark': 80, 'Stick': 140, 'Shoulder': 70})
out = ['#pragma once', '', '// Generated by tools/subareas/glb2model.py from the 3D models of HOS-InputDisplay (Res/Controller.glb,',
       '// JoyL.glb, JoyR.glb), simplified. Vertices in 1/2048 unit: x right, y top, z out of the buttons.', '',
       '#include <cstdint>', '#include "al/Library/Controller/NpadController.h"', '', 'namespace btt {', '',
       'enum class MotionPartKind : uint8_t { Body, Dark, Face, Stick, Button, Shoulder };', '',
       'struct MotionModelPart {', '    MotionPartKind kind;', '    int padIdx;', '    int triStart;', '    int triNum;', '};', '']
n = [emit('Pro', pro, out), emit('JoyL', joyl, out), emit('JoyR', joyr, out)]
out += ['', '} // namespace btt', '']
open(sys.argv[4], 'w', encoding='utf-8').write('\n'.join(out))
print('tris', n)
if len(sys.argv) > 5:
    preview([(pro, 250, 60, (45, 45, 48), 0.35), (joyl, 700, 70, (0, 175, 225), 0.35), (joyr, 900, 70, (250, 65, 50), 0.35)], sys.argv[5])
