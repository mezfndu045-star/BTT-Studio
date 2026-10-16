"""Input display skin packs (romfs/ImGuiData/Skins/<id>.pack): read / write, decode the ASTC pictures (astcenc, path in
$ASTCENC), encode back (Yaz0)."""
import os, pathlib, struct, subprocess, tempfile
import oead
from PIL import Image

ASTCENC = os.environ.get('ASTCENC', 'astcenc-avx2')
ENTRY = struct.Struct('<24sII')


def read_pack(path):
    data = pathlib.Path(path).read_bytes()
    assert data[:4] == b'BTTP'
    n = struct.unpack_from('<I', data, 4)[0]
    files = {}
    for i in range(n):
        name, off, size = ENTRY.unpack_from(data, 8 + i * ENTRY.size)
        files[name.rstrip(b'\0').decode()] = data[off:off + size]
    return files


def write_pack(path, files):
    names = sorted(files)
    offset = 8 + ENTRY.size * len(names)
    table, blob = b'', b''
    for nm in names:
        d = files[nm]
        table += ENTRY.pack(nm.encode(), offset + len(blob), len(d))
        blob += d
    pathlib.Path(path).write_bytes(b'BTTP' + struct.pack('<I', len(names)) + table + blob)


def raw_astc(data, skins=None):
    """the ASTC file of a picture: Yaz0 decompressed, a "REF:ImGuiData/Skins/<id>/<name>" picture read from its pack
    (skins: the Skins folder)"""
    if data[:4] == b'REF:':
        ref = data[4:].decode().strip()
        sid, name = ref.split('/')[-2:]
        return raw_astc(read_pack(pathlib.Path(skins) / f'{sid}.pack')[name], skins)
    if data[:4] == b'Yaz0':
        data = bytes(oead.yaz0.decompress(data))
    return data


def resolve_ref(data, skins):
    """a picture file without REF: (the referenced file, as stored: Yaz0 or plain)"""
    while data[:4] == b'REF:':
        sid, name = data[4:].decode().strip().split('/')[-2:]
        data = read_pack(pathlib.Path(skins) / f'{sid}.pack')[name]
    return data


def decode(data):
    with tempfile.TemporaryDirectory() as t:
        a, p = os.path.join(t, 'i.astc'), os.path.join(t, 'o.png')
        open(a, 'wb').write(data)
        subprocess.run([ASTCENC, '-ds', a, p], check=True, capture_output=True)
        return Image.open(p).convert('RGBA').copy()


def encode(img, block='6x6', srgb=True):
    with tempfile.TemporaryDirectory() as t:
        p, a = os.path.join(t, 'i.png'), os.path.join(t, 'o.astc')
        img.save(p)
        subprocess.run([ASTCENC, '-cs' if srgb else '-cl', p, a, block, '-medium'], check=True, capture_output=True)
        return bytes(oead.yaz0.compress(open(a, 'rb').read()))
