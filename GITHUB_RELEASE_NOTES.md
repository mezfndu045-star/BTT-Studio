# BTT Studio 1.0 — Practice Mod 1.0 (Super Mario Odyssey 1.0.0)

> **Version :** `v1.0.0`  
> **Jeu compatible :** Super Mario Odyssey 1.0.0 (Title ID: `0100000000010000`)  
> **Plateformes :** Nintendo Switch (Atmosphere CFW / Simple Mod Manager) & Émulateur Yuzu  
> **Binaire :** `exefs/subsdk4` (~598 Ko) + `exefs/main.npdm`

---

## 🌟 Points Forts de la Mise à Jour v1.0.0

Cette mise à jour majeure unifie le **moteur natif de Super Mario Odyssey 1.0.0** (préservant tous les glitchs iconiques de speedrun tels que le *First Person Glitch*) avec la superbe **interface verticale translucide et moderne de la version 1.3** (police officielle Rodin haute définition).

### 🌈 1. Input Display Déplaçable & Couleur Dynamique "Multicolor" (RGB Rainbow)
- **Déplacement libre à la souris (Drag & Drop) :** Lorsque le menu est ouvert, un contour interactif discret entoure la manette. Cliquez et glissez simplement pour placer l'Input Display exactement où vous le souhaitez.
- **Contrôle précis et presets :**
  - **Preset Pos :** *Center*, *Bottom Left*, *Bottom Right*, *Top Left*, *Top Right* pour caler instantanément le contrôleur dans les 4 coins de l'écran.
  - **Position X & Position Y :** Réglage fin par pas de 25 pixels.
  - **Reset Position :** Rétablit la manette au centre.
- **Couleur "Multicolor" (RGB Rainbow dynamique) :**
  - Génération procédurale d'un dégradé arc-en-ciel animé en continu via conversion dynamique HSV.
  - Applicable indépendamment ou simultanément pour : **Button Color**, **Pressed Color**, **Stick Color**, **Ring Color** et **BG Color** (avec opacité adaptée pour le fond).
  - Support complet des manettes Player 1 et Player 2.

### 🎥 2. Caméra Instantanée & Vitesse Zéro sur Load Position
- **Zéro délai de transition :** Annulation immédiate de l'interpolation de la caméra (`al::requestCancelCameraInterpole`) verrouillée sur une boucle de 10 frames dès le chargement de position (`D-Pad Droite`). La vue se cale instantanément sur Mario sans aucun glissement visuel.
- **Annulation de la vitesse résiduelle (`al::setVelocityZero`) :** Mario ne subit aucune secousse ni inertie de son déplacement précédent.
- **Support des captures / transformations (`hack`) :** La sauvegarde et la téléportation gèrent parfaitement les ennemis capturés (Grenouille, Goomba, etc.).

### 🔄 3. Reload at Pos / Instant Reload
- **Rechargement de scène à la position actuelle :** Recharge la scène complète tout en replaçant immédiatement Mario à ses coordonnées et orientation actuelles, avec vie restaurée et sans cinématique d'introduction.
- **Option "Reload at Pos" (Yes/No) :** Dans les catégories **Options** et **Misc** (synchronisé avec ImGui).
- **Bouton d'action directe "Restart at Pos" :** Dans la catégorie **Misc** pour recharger à la demande en 1 clic.
- **Machine d'état robuste (`mReloadState`) :** Garantit zéro bug de désynchronisation entre le déchargement de l'ancien monde et la création de Mario dans le nouveau.

### 🪙 4. Les 18 Pixel Luigi 3D SMB1 Authentiques (100% Fidèles)
- Les 18 emplacements secrets officiels de Super Mario Odyssey reproduits avec une précision 1:1 d'après la vidéo officielle `1011.mp4` :
  1. Anneau de poussière d'impact au sol lors de la charge (`ZL`/`ZR`).
  2. Jaillissement en arc de cercle culminant à 280 unités avec pirouette verticale.
  3. Plaquage au sol à plat ventre horizontal.
  4. Rebond dynamique et redressement debout fier face caméra.
  5. Flèche blanche indicatrice ⬇ animée en flottement.
  6. Deux grosses pièces d'or 3D en rotation avec étincelles rétro scintillantes.
  7. Réaction de cabriole si Mario ou Cappy le touche.
  8. Réinitialisation automatique au **Restart Scene** (+200 pièces ré-obtenues à chaque reset pour s'entraîner aux routes de speedrun).

### 🔒 5. Isolation Complète des Entrées en Menu
- Quand le menu vertical est ouvert, les entrées manette (sauts, déplacements de Mario, caméra, interactions Cappy) sont isolées du moteur de jeu. Mario reste parfaitement immobile.
- Détection stricte des raccourcis hotkeys pour éviter toute téléportation involontaire en naviguant dans le menu.

### 🎩 6. Sélecteur d'Outfits (42 Costumes & 42 Chapeaux)
- Sélecteurs complets **"Outfit Body"** et **"Outfit Cap"** dans la catégorie **Misc**.
- Bouton **"Apply Outfit"** appliquant instantanément la tenue et rechargeant la scène.

### ⚡ 7. Speedrun Refreshes Intégrés
- **Cutscene Refresh :** Réactive toutes les cinématiques de monde, de scénario, la première capture de Bonneton et les dialogues de tous les PNJ majeurs (Capitaine Toad, Hint Toad, Jammin' Toad, Traveling Peach).
- **Warp Text Refresh :** Réactive l'explication complète de Cappy lors du premier contact avec un checkpoint.
- **Stage Warper :** Téléportation instantanée dans tous les mondes et zones annexes du jeu.

### ⌨️ 8. Raccourcis Manette Personnalisables
- Choix parmi 20 combinaisons pour **Save Position**, **Load Position** et **Restart Scene** directement depuis le menu **Keybinds**.

---

## 📋 Guide Complet des 8 Catégories d'Options

Le menu vertical translucide s'ouvre avec la combinaison **`L + D-Pad Gauche`** :

### ⚙️ 1. Options (Entraînement)
- **Moon Refresh** : Permet de reprendre une lune déjà obtenue.
- **Gray Moon Refresh** : Fait réapparaître les lunes grises/transparentes.
- **Purple Coins Refresh** : Fait réapparaître les pièces violettes régionales.
- **Doors Refresh** : Réinitialise l'état fermé des portes du niveau.
- **Moon Shards Refresh** : Fait réapparaître les 5 fragments de lune.
- **Kingdom Enter Refresh** : Rejoue l'arrivée de l'Odyssée dans le pays.
- **Warp Text Refresh** : Réactive l'explication de Cappy au drapeau.
- **Seeds Refresh** : Réinitialise les pots et graines dorées.
- **Cutscene Refresh** : Réactive les cinématiques et dialogues initiaux des PNJ.
- **Always Manual Cutscene** : Permet de passer les cinématiques sans attente.
- **Always Checkpoints** : Valide les checkpoints dès qu'on s'en approche.
- **Disable Autosave** : Coupe la sauvegarde auto pour garder sa partie propre.
- **Disable TP Puppet** : Supprime l'animation d'attente de Mario pendant un warp.
- **Skip Cloud** : Passe les transitions nuageuses entre les zones.
- **All Checkpoints** : Débloque tous les drapeaux de téléportation du niveau.
- **Luigi Pixels** : Affiche les 18 Pixel Luigi 8-bit dans le sol (+200 pièces chacun).
- **Spawn All Hint Art** : Fait apparaître tous les indices artistiques d'Hint Toad.
- **Pipe Maze Override** : Force le passage dans les labyrinthes de tuyaux.
- **No Checkpoint Touch** : Empêche d'activer un drapeau par erreur en le frôlant.
- **No Damage** : Mario ne subit aucun dégât (invincibilité totale).
- **Disable Music** : Coupe la musique pour n'entendre que les bruitages du jeu.
- **Disable Moon Lock** : Débloque l'Odyssée sans le nombre de lunes requis.
- **Reload Scene at Pos** : Recharge tout le niveau en restant exactement à sa position.

### 🗺️ 2. Stages (Téléportation)
- **Kingdom** : Sélection du pays parmi les 17 royaumes (Bonneton jusqu'à Côté Très Sombre).
- **Scenario** : Sélection du chapitre d'histoire (0 à 15).
- **Warp to Stage** : Bouton pour se téléporter immédiatement au monde et scénario choisis.

### 🎒 3. Misc (Actions & Utilitaires)
- **Outfit Body** : Sélection de la tenue parmi 42 costumes officiels.
- **Outfit Cap** : Sélection du chapeau parmi 42 casquettes/chapeaux.
- **Apply Outfit** : Bouton pour équiper immédiatement la tenue choisie.
- **Save Position** : Sauvegarde manuelle des coordonnées actuelles.
- **Load Position** : Téléportation immédiate à la sauvegarde (caméra instantanée).
- **Restart Scene** : Relance le niveau au point de départ normal.
- **Reload at Pos** : Active le rechargement de niveau à la position actuelle.
- **Restart at Pos** : Bouton direct pour relancer la scène à votre position.
- **Kill Mario** : Élimine Mario immédiatement (mort instantanée).
- **Damage Mario** : Inflige 1 point de dégât pour tester une réaction physique.
- **Life Up Heart** : Donne un cœur géant (passe la barre à 6 PV max).
- **Heal Mario** : Rend toute sa vie à Mario à 100%.
- **Add 1000 Coins** : Ajoute 1 000 pièces d'or.
- **Remove 1000 Coins** : Retire 1 000 pièces d'or.
- **Reload Scene** : Force un rechargement propre de la scène.
- **Previous Scene** : Téléporte dans la zone ou le niveau précédent.
- **Wiggler Pattern** : Choix du pattern de déplacement du boss Méca-Wiggler (23 choix).

### ⏱️ 4. Timer (Chronomètre)
- **Segment Timer** : Affiche le chronomètre de speedrun à l'écran.
- **Timer Split** : Active le découpage automatique des temps aux checkpoints.
- **Start Timer** : Démarre le chronomètre.
- **Stop Timer** : Met le chronomètre en pause.
- **Reset Timer** : Remet le chronomètre à zéro (00:00:00).

### 🎮 5. Input Display (Affichage Manette)
- **Input Display** : Affiche ou masque la manette à l'écran.
- **2P Mode** : Affiche aussi la 2e manette (Joueur 2 / Cappy).
- **Preset Pos** : Place la manette au Centre ou dans les 4 coins d'un clic.
- **Position X & Y** : Règle l'emplacement par pas de 25px (ou glisser-déposer à la souris).
- **Reset Position** : Replace la manette au centre de l'écran.
- **Button Color** : Couleur des boutons au repos (13 teintes au choix).
- **Pressed Color** : Couleur des boutons quand ils sont enfoncés.
- **Stick Color** : Couleur des sticks analogiques.
- **Ring Color** : Couleur des cercles entourant les sticks.
- **BG Color** : Couleur et transparence du boîtier de la manette.
- **Multicolor** : Effet dynamique arc-en-ciel RGB animé fluide sur n'importe quel élément.

### 🕹️ 6. Keybinds (Raccourcis Manette)
- **Open Menu** : Touche pour ouvrir le menu (*L + D-Pad Gauche* par défaut).
- **Save Position** : Touche rapide pour sauver la position (*D-Pad Gauche* par défaut).
- **Load Position** : Touche rapide pour se téléporter (*D-Pad Droite* par défaut).
- **Restart Scene** : Touche rapide pour relancer le niveau (*D-Pad Haut* par défaut).
- **Heal Mario** : Raccourci pour redonner toute la vie.
- **Life Up** : Raccourci pour donner le cœur 6 PV.
- **Prev Scene** : Raccourci pour revenir en arrière.
- **Warp Last CP** : Raccourci pour aller au dernier checkpoint touché.
- **Timer Start** : Raccourci pour démarrer le chronomètre à la manette.
- **Timer End** : Raccourci pour stopper le chronomètre.
- **Timer Reset** : Raccourci pour remettre le chronomètre à zéro.

### 📊 7. Info (Données en direct)
- **Info Window** : Affiche ou masque la fenêtre de données.
- **Player Info** : Affiche en direct les coordonnées (X, Y, Z), la vitesse de Mario et l'entité possédée par Cappy.

### 🛠️ 8. Settings (Configuration)
- **Menu Corner** : Choisit l'angle de l'écran où s'ancre le menu vertical (Haut-Gauche, Haut-Droite, Bas-Gauche, Bas-Droite).

---

## 🎮 Raccourcis par Défaut

| Raccourci | Action |
|---|---|
| **L + D-Pad Gauche** | Ouvrir / Fermer le menu vertical BTT Studio |
| **D-Pad Gauche** | Sauvegarder la position (Save State) |
| **D-Pad Droite** | Charger la position (Load State - Caméra instantanée) |
| **D-Pad Haut** | Recharger la scène (Restart Scene / Reload at Pos) |
| **D-Pad Bas** | Scène précédente (Prev Stage) |
| **Maintenir R3 (Stick Droit)** | Roue d'actions rapides (Quick Wheel) |

---

## 📦 Instructions d'Installation

### 1. Sur Émulateur Yuzu (PC)
1. Téléchargez l'archive ou récupérez les fichiers `exefs/subsdk4` et `exefs/main.npdm`.
2. Placez-les dans votre dossier de mod Yuzu :
   ```
   %APPDATA%\yuzu\load\0100000000010000\Practice mod 1.0\exefs\subsdk4
   %APPDATA%\yuzu\load\0100000000010000\Practice mod 1.0\exefs\main.npdm
   ```
3. Dans Yuzu, faites clic droit sur **Super Mario Odyssey** &rarr; **Propriétés** &rarr; cochez **Practice mod 1.0** (désactivez la 1.3 si présente).
4. Lancez le jeu (version SMO 1.0.0).

### 2. Sur Nintendo Switch (Simple Mod Manager)
1. Copiez le dossier `Practice mod 1.0` dans `SD:/mods/` :
   ```
   SD:/mods/Practice mod 1.0/contents/0100000000010000/exefs/subsdk4
   SD:/mods/Practice mod 1.0/contents/0100000000010000/exefs/main.npdm
   ```
2. Ouvrez l'application **Simple Mod Manager** sur votre Switch et activez le mod.

### 3. Sur Nintendo Switch (Atmosphere Direct)
1. Copiez les fichiers dans :
   ```
   SD:/atmosphere/contents/0100000000010000/exefs/subsdk4
   SD:/atmosphere/contents/0100000000010000/exefs/main.npdm
   ```
2. Démarrez votre console avec Atmosphere.

---

## 👥 Crédits & Remerciements
- **MrKatzenGaming** — Développeur de BTT-Studio 1.0 original
- **fruityloops1 / tetraxile** — Auteurs de smo-practice et smo-practice-2 (Practice Mod 1.3)
- **CraftyBoss** — SMO-Exlaunch-Base
- **MonsterDruide1** — OdysseyHeaders
- **ocornut** — Dear ImGui
