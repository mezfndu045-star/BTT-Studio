import json, sys, pathlib
sys.stdout.reconfigure(encoding='utf-8')
here = pathlib.Path(__file__).parent
ent = json.load(open(here / 'entrances.json', encoding='utf-8'))
ids = json.load(open(here / 'stage_ids.json', encoding='utf-8'))
areas = json.load(open(here / 'subareas.json', encoding='utf-8'))
PRIO = ['DokanStageChange', 'DoorWarpStageChange', 'PictureStageChange', 'DoorAreaChangeCap', 'DokanStageChange2D',
        'SnowManRaceNpc', 'YukimaruRaceTutorialObj', 'BazookaElectric', 'KoopaChurch', 'ChangeStageArea']


def stage_of(src, home):
    # objects of a zone are part of the kingdom's home stage
    return home if src.endswith('Zone') else src


tour = []
report = []
for k in areas:
    home = k['kingdom']
    prefix = home.replace('HomeStage', '')
    for a in k['areas']:
        st = a['stage']
        enters = [e for e in ent.get(st, []) if e['src'] != st and e['unit'] in PRIO]
        enters.sort(key=lambda e: (0 if stage_of(e['src'], home) == home else 1 if e['src'].startswith(prefix) else 2,
                                   PRIO.index(e['unit'])))
        enter = enters[0] if enters else None
        parent = stage_of(enter['src'], home) if enter else home
        # exits: objects of the sub area (or its zones) leading back to the parent stage
        exits = [e for e in ent.get(parent, []) if (e['src'] == st or (e['src'].endswith('Zone') and e['src'].startswith(st.replace('Stage', ''))))
                 and e['unit'] in PRIO]
        exits.sort(key=lambda e: PRIO.index(e['unit']))
        # an exit label only works if the parent stage has an object with that ChangeStageId; the entrance's own label
        # always exists there (it is the pipe / door itself)
        exits = [e for e in exits if e['label'] in ids.get(parent, [])]
        exit_ = exits[0] if exits else None
        if not exit_ and enter and enter['label'] in ids.get(parent, []):
            exit_ = enter
        tour.append({'shot': st + '_in', 'stage': st, 'label': enter['label'] if enter else 'start'})
        if exit_:
            tour.append({'shot': st + '_out', 'stage': parent, 'label': exit_['label']})
        report.append((st, enter and enter['unit'], enter and enter['label'], parent, exit_ and exit_['unit'], exit_ and exit_['label']))

for r in report:
    print('%-34s in:%-20s %-24s out:%-26s %-20s %s' % tuple(str(x) for x in r))
print(len(tour), 'shots,', sum(1 for r in report if r[5]), 'with exterior')
json.dump(tour, open(here / 'tour.json', 'w', encoding='utf-8'), indent=1)
