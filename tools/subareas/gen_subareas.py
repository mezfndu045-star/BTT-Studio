import json, sys, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).parent))
from areanames import NAMES

here = pathlib.Path(__file__).parent
d = {k['world']: k for k in json.load(open(here / 'subareas.json', encoding='utf-8'))}
order = [('Cap', 'Cap'), ('Waterfall', 'Cascade'), ('Sand', 'Sand'), ('Forest', 'Wooded'), ('Lake', 'Lake'), ('Cloud', 'Cloud'),
         ('Clash', 'Lost'), ('City', 'Metro'), ('Snow', 'Snow'), ('Sea', 'Seaside'), ('Lava', 'Luncheon'), ('Attack', 'Ruined'),
         ('Sky', 'Bowser'), ('Moon', 'Moon'), ('Peach', 'Mushroom'), ('Special1', 'Dark Side'), ('Special2', 'Darker Side')]


def esc(s):
    return s.replace('\\', '\\\\').replace('"', '\\"')


L = ['#pragma once', '',
     '// Generated from SystemData/WorldList.szs (WorldListFromDb.byml): every sub area of each kingdom, in the order of',
     '// the Stages > Kingdom list. Preview images: romfs ImGuiData/SubAreas/<stage>.astc', '',
     'namespace btt {', '',
     'struct SubArea {', '    const char* stage;', '    const char* name;', '};', '',
     'struct KingdomSubAreas {', '    const char* title;', '    const SubArea* areas;', '    const char* const* names;',
     '    int count;', '};', '']
tables = []
for world, title in order:
    areas = sorted(d[world]['areas'], key=lambda a: NAMES[a['stage']].lower())
    ident = 'sSubAreas' + world
    L.append(f'inline constexpr SubArea {ident}[] = {{')
    for a in areas:
        L.append(f'    {{ "{a["stage"]}", "{esc(NAMES[a["stage"]])}" }},')
    L.append('};')
    L.append(f'inline constexpr const char* {ident}Names[] = {{')
    for a in areas:
        L.append(f'    "{esc(NAMES[a["stage"]])}",')
    L.append('};')
    tables.append((title, ident, len(areas)))
L.append('')
L.append('inline constexpr KingdomSubAreas sKingdomSubAreas[] = {')
for title, ident, n in tables:
    L.append(f'    {{ "{title} Sub Area", {ident}, {ident}Names, {n} }},')
L += ['};', '', '} // namespace btt', '']
pathlib.Path(sys.argv[1]).write_bytes('\n'.join(L).encode())
print(sum(n for _, _, n in tables), [t[2] for t in tables])
