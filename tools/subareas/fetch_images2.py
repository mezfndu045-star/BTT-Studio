import json, sys, time, re, html, urllib.parse, urllib.request, pathlib
sys.path.insert(0, str(pathlib.Path(__file__).parent))
from areanames import PAGES

UA = 'BTT-Studio-practice-mod/1.0 (personal project)'
API = 'https://www.mariowiki.com/api.php'

LIST_PAGES = {
    'Cap': 'Cap Kingdom', 'Waterfall': 'Cascade Kingdom', 'Sand': 'Sand Kingdom', 'Forest': 'Wooded Kingdom',
    'Lake': 'Lake Kingdom', 'Cloud': 'Cloud Kingdom', 'Clash': 'Lost Kingdom', 'City': 'Metro Kingdom',
    'Snow': 'Snow Kingdom', 'Sea': 'Seaside Kingdom', 'Lava': 'Luncheon Kingdom', 'Attack': 'Ruined Kingdom',
    'Sky': "Bowser's Kingdom", 'Moon': 'Moon Kingdom', 'Peach': 'Mushroom Kingdom', 'Special1': 'Dark Side',
    'Special2': 'Darker Side',
}


def get(url):
    req = urllib.request.Request(url, headers={'User-Agent': UA})
    with urllib.request.urlopen(req, timeout=60) as r:
        return r.read()


def norm(s):
    s = html.unescape(s).replace('’', "'").replace('‘', "'").replace(' ', ' ')
    return re.sub(r'\s+', ' ', s).strip().lower()


def moon_images(page):
    q = urllib.parse.urlencode({'action': 'parse', 'page': page, 'prop': 'text', 'redirects': 1, 'format': 'json'})
    d = json.loads(get(API + '?' + q))
    if 'parse' not in d: return {}
    h = d['parse']['text']['*']
    res = {}
    for row in re.findall(r'<tr[^>]*>(.*?)</tr>', h, re.S):
        img = re.search(r'srcset="[^"]*?(\S+) 2x"', row) or re.search(r'<img[^>]*src="([^"]+)"', row)
        name = re.search(r'<i>(.*?)</i>', row, re.S)
        if img and name:
            res[norm(re.sub(r'<[^>]+>', '', name.group(1)))] = img.group(1)
    return res


def page_image(title):
    q = urllib.parse.urlencode({'action': 'query', 'titles': title, 'prop': 'pageimages', 'pithumbsize': 384,
                                'redirects': 1, 'format': 'json'})
    d = json.loads(get(API + '?' + q))['query']
    for p in d['pages'].values():
        if 'thumbnail' in p: return p['thumbnail']['source']
    return None


areas = json.load(open(sys.argv[1], encoding='utf-8'))
out = {}
for k in areas:
    page = 'List of Power Moons in the ' + LIST_PAGES[k['world']]
    imgs = moon_images(page)
    print('==', page, len(imgs))
    time.sleep(1)
    for a in k['areas']:
        pick = None
        for m in a['moons']:
            url = imgs.get(norm(m))
            if url: pick = {'moon': m, 'url': url}; break
        if not pick and a['stage'] in PAGES:
            url = page_image(PAGES[a['stage']]); time.sleep(1)
            if url: pick = {'page': PAGES[a['stage']], 'url': url}
        out[a['stage']] = pick
        print('  ', a['stage'], '->', pick)
json.dump(out, open(sys.argv[2], 'w', encoding='utf-8'), ensure_ascii=False, indent=1)
print('found', sum(1 for v in out.values() if v), '/', len(out))
