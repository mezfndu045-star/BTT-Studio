import oead, sys, json, pathlib, collections
sys.stdout.reconfigure(encoding='utf-8')
R = pathlib.Path(sys.argv[1]) / 'StageData'
areas = json.load(open(sys.argv[2], encoding='utf-8'))
targets = {a['stage'] for k in areas for a in k['areas']} | {k['kingdom'] for k in areas}


def vec(n, key):
    v = n.get(key)
    return [float(v['X']), float(v['Y']), float(v['Z'])] if v else [0.0, 0.0, 0.0]


found = collections.defaultdict(list)
units = collections.Counter()
for p in sorted(R.glob('*Map.szs')):
    src = p.name[:-len('Map.szs')]
    try:
        s = oead.Sarc(oead.yaz0.decompress(p.read_bytes()))
    except Exception:
        continue
    for f in s.get_files():
        if not f.name.endswith('.byml'):
            continue
        data = bytes(f.data)
        if b'ChangeStageName' not in data:
            continue
        root = oead.byml.from_binary(data)
        scen_list = list(root) if isinstance(root, oead.byml.Array) else [root]
        for si, scen in enumerate(scen_list):
            stack = [scen]
            while stack:
                n = stack.pop()
                if isinstance(n, oead.byml.Hash):
                    name = n.get('ChangeStageName')
                    if isinstance(name, str) and name in targets and 'Translate' in n:
                        units[str(n.get('UnitConfigName'))] += 1
                        found[name].append({'src': src, 'scenario': si, 'unit': str(n.get('UnitConfigName')),
                                            'model': str(n.get('ModelName')), 'id': str(n.get('Id')),
                                            'label': str(n.get('ChangeStageId')), 'pos': vec(n, 'Translate'),
                                            'rot': vec(n, 'Rotate'), 'scale': vec(n, 'Scale')})
                    stack.extend(n.values())
                elif isinstance(n, oead.byml.Array):
                    stack.extend(n)

# merge identical objects found in several scenarios
out = {}
for st, lst in found.items():
    merged = {}
    for e in lst:
        key = (e['src'], e['id'])
        if key not in merged:
            merged[key] = dict(e, scenarios=[e['scenario']])
            del merged[key]['scenario']
        else:
            merged[key]['scenarios'].append(e['scenario'])
    out[st] = list(merged.values())
json.dump(out, open(sys.argv[3], 'w', encoding='utf-8'), indent=1)
print(units)
print('with entrance', len(out), '/', len(targets))
print('missing', sorted(targets - set(out)))
