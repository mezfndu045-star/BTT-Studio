# BTT Studio — Practice Mod 1.0

[![Release](https://img.shields.io/badge/Release-v1.0.0-blue.svg)](GITHUB_RELEASE_NOTES.md)
[![Game](https://img.shields.io/badge/Game-Super_Mario_Odyssey_1.0.0-red.svg)](#)
[![TitleID](https://img.shields.io/badge/Title_ID-0100000000010000-green.svg)](#)
[![Platform](https://img.shields.io/badge/Platform-Nintendo_Switch_%7C_Yuzu-purple.svg)](#)
[![Docs](https://img.shields.io/badge/Showcase-GitHub_Pages-cyan.svg)](docs/index.html)

> **BTT Studio Practice Mod 1.0** est la fusion ultime entre le moteur natif de **Super Mario Odyssey 1.0.0** (préservant tous les glitchs et mécaniques de speedrun) et la superbe interface graphique verticale translucide du **Practice Mod 1.3** (police Rodin).

---

## 🌐 Site Web & Guide Officiel

Le site officiel en ligne est disponible sur **GitHub Pages** ([https://mezfndu045-star.github.io/BTT-Studio/](https://mezfndu045-star.github.io/BTT-Studio/)) :
- **Guide complet des 8 catégories d'options** expliquées simplement et sans jargon.
- **Téléchargement direct en 1 clic** pour Yuzu, Simple Mod Manager et Atmosphere.
- **Comparatif interactif des versions** (SMO Vanilla vs Practice Mod 1.3 vs BTT Studio 1.0).
- **Instructions détaillées d'installation** avec copie des dossiers.

---

## 🚀 Nouveautés de la Version 1.0

### 🌈 1. Input Display Déplaçable & Couleur Multicolor
- **Drag & Drop libre :** Déplacez directement l'Input Display à l'écran à la souris lorsque le menu est ouvert.
- **Presets & Réglages :** Presets 4 coins (*Center*, *Bottom Left*, *Bottom Right*, *Top Left*, *Top Right*) et réglage fin X/Y par pas de 25 pixels.
- **Couleur Multicolor (RGB Rainbow) :** Onde arc-en-ciel animée en continu pour tous les boutons, sticks, anneaux et fond.

### 🎥 2. Caméra Instantanée sur Load Position
- Annulation multi-frame de l'interpolation de la caméra (`al::requestCancelCameraInterpole`) et annulation de la vitesse résiduelle (`al::setVelocityZero`).
- Verrouillage instantané sur Mario à la première frame sans aucun glissement visuel.
- Support complet des transformations et captures (`hack`).

### 🔄 3. Reload at Pos (Rechargement à la position)
- Recharge entièrement la scène tout en replaçant Mario exactement à ses coordonnées actuelles avec vie restaurée.
- Option toggle "Reload at Pos" (Yes/No) et bouton direct "Restart at Pos".

### 🪙 4. Le Vrai Luigi 8-bit du Jeu
- Dans le jeu officiel, 28 buissons et touffes d'herbe (8 royaumes) ont 10 % de chance de cacher un Luigi 8-bit qui sort du sol et donne des pièces.
- L'option « Luigi Pixels » force ce tirage à 100 % : l'animation, les pièces et les sons sont ceux du jeu.
- Rechargez la scène après avoir activé l'option. *(en test)*

### 🐰 Skip Broodals *(nouveau en v1.1, en test)*
- Dès la fin de la cinématique d'intro, le boss est vaincu par son propre code de « dernier coup » : le jeu joue la cinématique de fin et fait apparaître la Multi-Lune.
- Madame Broode (Chutes), Hariet (Sables), Rango (Lac), Spewart (Bois, revanche aux Gourmets). Topper n'est pas concerné.

### 🔒 5. Blocage des Entrées du Jeu en Menu
- Mario reste immobile pendant la navigation dans le menu vertical (aucune entrée manette envoyée au jeu).

### 🎩 6. Sélecteur d'Outfits (42 Costumes & Chapeaux)
- Choix complet de 42 costumes et 42 chapeaux avec bouton **"Apply Outfit"** instantané.

### ⚡ 7. Speedrun Refreshes Intégrés
- **Cutscene Refresh :** Réactive toutes les cinématiques et dialogues PNJ (Capitaine Toad, Hint Toad, Jammin' Toad, Traveling Peach).
- **Warp Text Refresh :** Réactive les explications Cappy des checkpoints.

---

## 📋 Guide Complet des Options du Menu

Le menu vertical translucide s'ouvre avec la combinaison **`L + D-Pad Gauche`**. Voici l'explication simple et courte de chaque option :

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
- **Luigi Pixels** : Le vrai Luigi 8-bit sort à chaque fois des buissons concernés (10 % de chance normalement).
- **Spawn All Hint Art** : Fait apparaître tous les indices artistiques d'Hint Toad.
- **Pipe Maze Override** : Force le passage dans les labyrinthes de tuyaux.
- **No Checkpoint Touch** : Empêche d'activer un drapeau par erreur en le frôlant.
- **No Damage** : Mario ne subit aucun dégât (invincibilité totale).
- **Disable Music** : Coupe la musique pour n'entendre que les bruitages du jeu.
- **Disable Moon Lock** : Débloque l'Odyssée sans le nombre de lunes requis.
- **Skip Broodals** : Termine les combats de Broodals juste après leur intro et fait apparaître la Multi-Lune.
- **Always Odyssey Power Up** : À chaque passage à l'Odyssée (même sans lune), elle se recharge et propose le royaume suivant de l'histoire ; l'arrivée joue l'intro, l'atterrissage et le scénario.
- **Odyssey In All Kingdoms** : L'Odyssée est là dans tous les royaumes et on peut s'y téléporter depuis la carte. *(en test)*
- **Night Metro Multi Moon** : Au début de la Ville de nuit, la Multi-Lune de Méca-Chenille tombe près de Mario pour passer à la Ville de jour. *(en test)*
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

## 🎮 Contrôles & Raccourcis

| Raccourci | Fonction |
|---|---|
| **L + D-Pad Gauche** | Ouvrir / Fermer le menu vertical BTT Studio |
| **D-Pad Gauche** | Sauvegarder la position (`Save State`) |
| **D-Pad Droite** | Charger la position (`Load State` - Caméra instantanée) |
| **D-Pad Haut** | Recharger la scène (`Restart Scene` / `Reload at Pos`) |
| **D-Pad Bas** | Scène précédente (`Prev Stage`) |
| **Maintenir R3 (Stick Droit)** | Roue rapide d'actions (`Quick Wheel`) |

*Les touches de Save Pos, Load Pos et Restart Scene sont entièrement configurables dans le menu **Keybinds**.*

---

## 📥 Installation

### Sur Émulateur Yuzu
Placez les fichiers dans :
```
%APPDATA%\yuzu\load\0100000000010000\Practice mod 1.0\exefs\subsdk4
%APPDATA%\yuzu\load\0100000000010000\Practice mod 1.0\exefs\main.npdm
```
Dans Yuzu, faites clic droit sur **Super Mario Odyssey** &rarr; **Propriétés** &rarr; cochez **Practice mod 1.0**.

### Sur Nintendo Switch (Simple Mod Manager)
Copiez dans votre carte SD :
```
SD:/mods/Practice mod 1.0/contents/0100000000010000/exefs/subsdk4
SD:/mods/Practice mod 1.0/contents/0100000000010000/exefs/main.npdm
```
Activez le mod dans **Simple Mod Manager**.

### Sur Nintendo Switch (Atmosphere)
Copiez directement dans :
```
SD:/atmosphere/contents/0100000000010000/exefs/subsdk4
SD:/atmosphere/contents/0100000000010000/exefs/main.npdm
```

---

## 👥 Crédits

- [BTT-Studio](https://github.com/MrKatzenGaming/BTT-Studio) par **MrKatzenGaming**
- [smo-practice-2](https://github.com/fruityloops1/smo-practice-2) par **fruityloops1**
- [smo-practice](https://github.com/tetraxile/smo-practice) & [Calypso](https://github.com/tetraxile/Calypso) par **tetraxile**
- [smo-lunakit](https://github.com/Amethyst-szs/smo-lunakit) par **Amethyst-szs**
- [SMO-Exlaunch-Base](https://github.com/CraftyBoss/SMO-Exlaunch-Base) par **CraftyBoss**
- [OdysseyHeaders](https://github.com/MonsterDruide1/OdysseyHeaders) par **MonsterDruide1**
- [Dear ImGui](https://github.com/ocornut/imgui) par **ocornut**
