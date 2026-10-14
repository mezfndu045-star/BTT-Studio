"""ChangeStageId values present in each stage (zones merged into their kingdom's home stage): valid arrival labels."""
import oead, sys, json, pathlib, collections
R = pathlib.Path(sys.argv[1]) / 'StageData'
areas = json.load(open(sys.argv[2], encoding='utf-8'))
homes = [k['kingdom'] for k in areas]
ids = collections.defaultdict(set)
for p in sorted(R.glob('*Map.szs')):
    src = p.name[:-len('Map.szs')]
    stage = src
    if src.endswith('Zone'):
        stage = next((h for h in homes if src.startswith(h.replace('HomeStage', ''))), src)
    try:
        s = oead.Sarc(oead.yaz0.decompress(p.read_bytes()))
    except Exception:
        continue
    for f in s.get_files():
        if not f.name.endswith('.byml'): continue
        data = bytes(f.data)
        if b'ChangeStageId' not in data: continue
        stack = [oead.byml.from_binary(data)]
        while stack:
            n = stack.pop()
            if isinstance(n, oead.byml.Hash):
                v = n.get('ChangeStageId')
                if isinstance(v, str): ids[stage].add(v)
                stack.extend(n.values())
            elif isinstance(n, oead.byml.Array):
                stack.extend(n)
json.dump({k: sorted(v) for k, v in ids.items()}, open(sys.argv[3], 'w', encoding='utf-8'), indent=1)
print(len(ids), 'stages')
