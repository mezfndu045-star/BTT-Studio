import struct
def parse_msbt(data):
    assert data[:8] == b'MsgStdBn', data[:8]
    bom = data[8:10]; e = '<' if bom == b'\xff\xfe' else '>'
    nsec = struct.unpack_from(e + 'H', data, 0x0e)[0]
    off = 0x20; labels = {}; texts = []
    for _ in range(nsec):
        magic = data[off:off+4]; size = struct.unpack_from(e + 'I', data, off + 4)[0]; body = off + 0x10
        if magic == b'LBL1':
            n = struct.unpack_from(e + 'I', data, body)[0]
            for i in range(n):
                cnt, o = struct.unpack_from(e + 'II', data, body + 4 + i * 8)
                p = body + o
                for _ in range(cnt):
                    l = data[p]; name = data[p+1:p+1+l].decode('ascii'); idx = struct.unpack_from(e + 'I', data, p+1+l)[0]
                    labels[name] = idx; p += 1 + l + 4
        elif magic == b'TXT2':
            n = struct.unpack_from(e + 'I', data, body)[0]
            offs = [struct.unpack_from(e + 'I', data, body + 4 + i * 4)[0] for i in range(n)] + [size]
            for i in range(n):
                raw = data[body + offs[i]: body + offs[i+1]]
                # strip control tags (0x0e ...)
                out = []; j = 0
                while j + 1 < len(raw):
                    c = struct.unpack_from(e + 'H', raw, j)[0]
                    if c == 0x0e:
                        argsz = struct.unpack_from(e + 'H', raw, j + 6)[0]; j += 8 + argsz; continue
                    if c == 0x0f: j += 6; continue
                    if c == 0: break
                    out.append(chr(c)); j += 2
                texts.append(''.join(out))
        off = body + size; off = (off + 15) & ~15
    return {k: texts[v] for k, v in labels.items()}
