"""Pack each input display skin folder of romfs/ImGuiData/Skins into one <id>.pack file (fewer files to copy when the
mod is installed), or unpack them again.

  py pack_skins.py pack <romfs/ImGuiData/Skins>
  py pack_skins.py unpack <romfs/ImGuiData/Skins>

Format (read by btt::loadRomfsFile, src/menu/SubAreaImages.cpp): "BTTP", u32 count, count x { char name[24];
u32 offset; u32 size; }, then the files. skin.txt and the pictures (.astc: Yaz0 or plain ASTC, or "REF:<path>") are
packed; other files (licenses) stay in the folder.
"""
import pathlib, struct, sys

ENTRY = struct.Struct('<24sII')


def pack(skins):
    for d in sorted(p for p in skins.iterdir() if p.is_dir()):
        files = sorted(p for p in d.iterdir() if p.is_file() and (p.suffix == '.astc' or p.name == 'skin.txt'))
        if not files:
            continue
        assert len(files) < 256 and all(len(p.name) < 24 for p in files), d
        offset = 8 + ENTRY.size * len(files)
        table, blob = b'', b''
        for p in files:
            data = p.read_bytes()
            table += ENTRY.pack(p.name.encode(), offset + len(blob), len(data))
            blob += data
        (skins / f'{d.name}.pack').write_bytes(b'BTTP' + struct.pack('<I', len(files)) + table + blob)
        for p in files:
            p.unlink()
        if not any(d.iterdir()):
            d.rmdir()


def unpack(skins):
    for pk in sorted(skins.glob('*.pack')):
        data = pk.read_bytes()
        assert data[:4] == b'BTTP'
        count = struct.unpack_from('<I', data, 4)[0]
        d = skins / pk.stem
        d.mkdir(exist_ok=True)
        for i in range(count):
            name, off, size = ENTRY.unpack_from(data, 8 + i * ENTRY.size)
            (d / name.rstrip(b'\0').decode()).write_bytes(data[off:off + size])
        pk.unlink()


if __name__ == '__main__':
    {'pack': pack, 'unpack': unpack}[sys.argv[1]](pathlib.Path(sys.argv[2]))
