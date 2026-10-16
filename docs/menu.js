// Interactive copy of the in-game menu (data: menu-data.js). Click a category to open it, click an option to read
// what it does; click the value (or the arrows) to change it like in the game.
(function () {
  const root = document.getElementById('game-menu');
  if (!root || typeof MENU === 'undefined') return;

  const cats = root.querySelector('.gm-cats');
  const panel = root.querySelector('.gm-items');
  const help = root.querySelector('.gm-help');
  const state = MENU.map((c) => c.items.map((it) => ('v' in it ? it.v : null)));
  let current = -1;
  let selected = -1;

  function valueText(it, v) {
    if (it.kind === 'bool') return v ? 'true' : 'false';
    if (it.kind === 'enum') return it.o[v];
    if (it.kind === 'int' || it.kind === 'key') return String(v);
    return '';
  }

  function change(ci, ii, dir) {
    const it = MENU[ci].items[ii];
    let v = state[ci][ii];
    if (it.kind === 'bool') v = !v;
    else if (it.kind === 'enum') v = (v + dir + it.o.length) % it.o.length;
    else if (it.kind === 'int') v = Math.min(it.max, Math.max(it.min, v + dir * it.step));
    else return;
    state[ci][ii] = v;
    renderItems();
  }

  function showHelp(it) {
    if (!it) {
      help.innerHTML = '<h4>Click an option</h4><p>Its explanation shows here, like the help panel of the game.</p>';
      return;
    }
    const how = {
      bool: 'Click the value to turn it on or off.',
      enum: 'Use the arrows to change the value.',
      int: 'Use the arrows to change the value.',
      button: 'A button: in the game, A runs it.',
      key: 'A keybind: "Custom" records any combination of buttons.',
      sub: 'In the game: A to choose, Left / Right to change the area, A to warp.',
    }[it.kind];
    help.innerHTML = '';
    const h = document.createElement('h4');
    h.textContent = it.t;
    const p = document.createElement('p');
    p.textContent = it.h;
    const s = document.createElement('p');
    s.className = 'gm-how';
    s.textContent = how;
    help.append(h, p, s);
  }

  function row(ci, ii) {
    const it = MENU[ci].items[ii];
    const el = document.createElement('div');
    el.className = 'gm-row' + (ii === selected ? ' is-selected' : '');
    el.setAttribute('role', 'button');
    el.tabIndex = 0;
    const title = document.createElement('span');
    title.className = 'gm-title';
    const hasValue = it.kind !== 'button' && it.kind !== 'sub';
    title.textContent = it.t + (hasValue ? ': ' : '');
    el.append(title);
    if (hasValue) {
      const arrows = it.kind === 'enum' || it.kind === 'int';
      if (arrows) el.append(arrow('‹', () => change(ci, ii, -1)));
      const val = document.createElement('span');
      val.className = 'gm-value' + (it.kind === 'bool' && state[ci][ii] ? ' is-on' : '');
      val.textContent = valueText(it, state[ci][ii]);
      if (it.kind === 'bool') val.addEventListener('click', (ev) => { ev.stopPropagation(); select(ii); change(ci, ii, 1); });
      el.append(val);
      if (arrows) el.append(arrow('›', () => change(ci, ii, 1)));
    }
    el.addEventListener('click', () => select(ii));
    el.addEventListener('keydown', (ev) => {
      if (ev.key === 'Enter' || ev.key === ' ') { ev.preventDefault(); select(ii); if (it.kind === 'bool') change(ci, ii, 1); }
      if (ev.key === 'ArrowLeft') change(ci, ii, -1);
      if (ev.key === 'ArrowRight') change(ci, ii, 1);
    });
    function arrow(text, fn) {
      const a = document.createElement('button');
      a.type = 'button';
      a.className = 'gm-arrow';
      a.textContent = text;
      a.setAttribute('aria-label', text === '‹' ? 'Previous value' : 'Next value');
      a.addEventListener('click', (ev) => { ev.stopPropagation(); select(ii); fn(); });
      return a;
    }
    return el;
  }

  function select(ii) {
    selected = ii;
    renderItems();
    showHelp(MENU[current].items[ii]);
  }

  function renderItems() {
    panel.innerHTML = '';
    if (current < 0) { panel.hidden = true; return; }
    panel.hidden = false;
    const cat = MENU[current];
    const split = cat.split || cat.items.length;
    const cols = cat.split ? [[0, split], [split, cat.items.length]] : [[0, cat.items.length]];
    panel.classList.toggle('two-cols', cols.length > 1);
    for (const [a, z] of cols) {
      const col = document.createElement('div');
      col.className = 'gm-col';
      for (let i = a; i < z; i++) col.append(row(current, i));
      panel.append(col);
    }
  }

  function openCat(ci) {
    current = ci;
    selected = -1;
    cats.querySelectorAll('.gm-cat').forEach((el, i) => el.classList.toggle('is-selected', i === ci));
    renderItems();
    showHelp(null);
  }

  MENU.forEach((c, ci) => {
    const el = document.createElement('button');
    el.type = 'button';
    el.className = 'gm-cat';
    el.textContent = c.name;
    el.addEventListener('click', () => openCat(ci));
    cats.append(el);
  });
  openCat(0);
})();
