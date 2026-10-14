import json, sys, time, urllib.parse, urllib.request, pathlib

UA = 'BTT-Studio-practice-mod/1.0 (personal project)'
API = 'https://www.mariowiki.com/api.php'
KINGDOMS = [
    ('CapWorldHomeStage', 'Cap Kingdom'), ('WaterfallWorldHomeStage', 'Cascade Kingdom'),
    ('SandWorldHomeStage', 'Sand Kingdom'), ('ForestWorldHomeStage', 'Wooded Kingdom'),
    ('LakeWorldHomeStage', 'Lake Kingdom'), ('CloudWorldHomeStage', 'Cloud Kingdom'),
    ('ClashWorldHomeStage', 'Lost Kingdom'), ('CityWorldHomeStage', 'Metro Kingdom'),
    ('SnowWorldHomeStage', 'Snow Kingdom'), ('SeaWorldHomeStage', 'Seaside Kingdom'),
    ('LavaWorldHomeStage', 'Luncheon Kingdom'), ('BossRaidWorldHomeStage', 'Ruined Kingdom'),
    ('SkyWorldHomeStage', "Bowser's Kingdom"), ('MoonWorldHomeStage', 'Moon Kingdom'),
    ('PeachWorldHomeStage', 'Mushroom Kingdom'), ('Special1WorldHomeStage', 'Dark Side'),
    ('Special2WorldHomeStage', 'Darker Side'),
]


def get(url):
    req = urllib.request.Request(url, headers={'User-Agent': UA})
    with urllib.request.urlopen(req, timeout=60) as r:
        return r.read()


out_dir = pathlib.Path(sys.argv[1])
out_dir.mkdir(exist_ok=True)
res = {}
for stage, page in KINGDOMS:
    q = urllib.parse.urlencode({'action': 'query', 'titles': page, 'prop': 'pageimages', 'pithumbsize': 384,
                                'redirects': 1, 'format': 'json'})
    d = json.loads(get(API + '?' + q))['query']
    url = next((p['thumbnail']['source'] for p in d['pages'].values() if 'thumbnail' in p), None)
    res[stage] = {'page': page, 'url': url}
    print(stage, url)
    if url:
        ext = pathlib.Path(urllib.parse.urlparse(url).path).suffix
        (out_dir / (stage + ext)).write_bytes(get(url))
    time.sleep(1)
json.dump(res, open(out_dir / 'kingdom_images.json', 'w', encoding='utf-8'), indent=1)
