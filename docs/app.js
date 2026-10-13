// BTT Studio — Practice Mod 1.0 Web App

document.addEventListener('DOMContentLoaded', () => {
  initTabs();
  initCopyButtons();
});

// 1. Installation Tabs
function initTabs() {
  const tabs = document.querySelectorAll('.tab-btn');
  const contents = document.querySelectorAll('.tab-content');

  tabs.forEach(tab => {
    tab.addEventListener('click', () => {
      const target = tab.dataset.tab;

      tabs.forEach(t => t.classList.remove('active'));
      contents.forEach(c => c.classList.remove('active'));

      tab.classList.add('active');
      const content = document.getElementById(target);
      if (content) content.classList.add('active');
    });
  });
}

// 2. Copy to Clipboard Buttons
function initCopyButtons() {
  document.querySelectorAll('.copy-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      const textToCopy = btn.dataset.copy;
      if (textToCopy) {
        navigator.clipboard.writeText(textToCopy).then(() => {
          const origText = btn.textContent;
          btn.textContent = '✓ Copié !';
          btn.style.background = 'rgba(16, 185, 129, 0.3)';
          btn.style.color = '#10b981';
          setTimeout(() => {
            btn.textContent = origText;
            btn.style.background = '';
            btn.style.color = '';
          }, 2000);
        }).catch(err => {
          console.error('Erreur copie presse-papier:', err);
        });
      }
    });
  });
}
