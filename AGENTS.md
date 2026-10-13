# BTT Studio Switch — Contexte Projet

> **Ce fichier est lu automatiquement par l'IA a chaque nouvelle conversation.**
> Il sert de memoire persistante entre les sessions et les comptes.
> **Mets-le a jour** quand un probleme est resolu ou quand un nouveau probleme apparait.

---

## Description du projet

**BTT Studio Switch** est un mod **Atmosphere** pour **Super Mario Odyssey** (Nintendo Switch / emulateur Yuzu).
- **Title ID** : `0100000000010000`
- **Base sur** : Practice Mod 1.3 (smo-practice-2 v0.3-7) avec preconfiguration des touches du 1.0
- **Version du jeu** : SMO 1.3.0
- **Emulateur** : Yuzu (2 installations sur le PC)

### Structure du mod

```
BTT-Studio-Switch/
  atmosphere/
    contents/
      0100000000010000/
        exefs/
          main.npdm          # Permissions du module
          subsdk9            # Le binaire compile du mod (~1 Mo)
        romfs/
          ImGuiData/         # Interface ImGui (police Rodin, config)
          LayoutData/        # SpeedbootLoad.szs
          StageData/         # CapWorldHomeStageMap.szs
```

### Code source de reference

1. **Practice Mod 1.3** (SMO 1.3.0) :
   - `C:\Users\FlowUP\Downloads\Practicemod1.3\source\src\program\` — Code principal du mod
   - `C:\Users\FlowUP\Downloads\Practicemod1.3\source\include\` — Headers (al, Player, Scene, MapObj, etc.)
   - `C:\Users\FlowUP\Downloads\Practicemod1.3\source\lib\` — Bibliotheques (imgui, NintendoSDK, odyssey, sead)
   - `C:\Users\FlowUP\Downloads\Practicemod1.3\source\syms\130\` — Symboles pour la version 1.3.0

2. **BTT-Studio 1.0** (SMO 1.0.0 avec interface graphique 1.3) :
   - `C:\Users\FlowUP\.gemini\antigravity-ide\scratch\BTT-Studio\` — Code source complet basé sur MrKatzenGaming/BTT-Studio (1.0.0) réadapté avec l'UI verticale et le thème 1.3 (MenuComponent, Menu.cpp, ImGui.cpp).
   - Toolchain de compilation : MSYS2 / Clang baremetal + devkitPro.
   - Binaire généré : `subsdk4` (~592 Ko) et `main.npdm`.
   - Script de post-build : `elf2nso.py` utilise un fallback automatique sur Python Windows 3.12 pour lz4/elftools.

### Installation Yuzu

Deux mods sont déployés dans `C:\Users\FlowUP\AppData\Roaming\yuzu\load\0100000000010000\` :
1. **`Practice mod 1.3\`** : Version SMO 1.3.0 (`subsdk9`).
2. **`Practice mod 1.0\`** : Version SMO 1.0.0 avec tout le code et les fonctions natives de BTT-Studio 1.0 combinés avec l'interface graphique de Practice Mod 1.3 (menu vertical translucide, police Rodin blanche : Options, Stages, Misc, Timer, Input Display, Keybinds, Info, Settings). Binaire : `exefs/subsdk4` + `exefs/main.npdm`.

Également synchronisé dans `C:\Users\FlowUP\Downloads\BTT-Studio-SimpleModManager\Practice mod 1.0\` et `C:\Users\FlowUP\Downloads\Practice mod 1.0\`.

---

## Problemes connus — Journal de suivi

### A TESTER — Timer : types "Capture" et "Moon Get"

- **Capture** : `PlayerHackKeeper::startHackStartDemo` et `startHackStartDemoPuppetable` (début de l'animation de capture).
- **Moon Get** : 1re frame de `Shine::exeWaitRequestDemo` (mis par `Shine::receiveMsg` au contact et par `getDirect`) + `Shine::getDirectWithDemo`. Pas `Shine::get` (appelé plus tard, pendant la cinématique).
- Anti-doublon : même type ignoré pendant 0,5 s (`eventOnce` dans `TimerHooks.cpp`). Types ajoutés à la fin de `TimerHookType` (u8 sauvegardé).
- **Black Screen / White Screen** : `exeCloseEnd` n'est JAMAIS exécuté (constaté en jeu) → événement après `al::WipeSimple::exeClose` quand `isCloseEnd` devient vrai, et après `startCloseEnd` (fermeture instantanée). Noir = archives `FadeBlack` + `WipeCircle` (tuyaux), blanc = `FadeWhite` (téléportations). La couleur est mémorisée par pointeur dans les constructeurs (C1 : this, name, archive, info, suffix ; **C2 : this, VTT, name, archive, info, suffix** — héritage virtuel).
- `Timer::event` réécrit : le type de départ remet à zéro et relance toujours ; le type de fin arrête toujours ; départ == fin → alterne départ/arrêt (ou split si Timer Split) ; les autres types sont ignorés.
- **Escape Wire** : fil électrique final de la fuite du Pays de la Lune (Bowser capturé). Se déclenche quand Bowser part le long du fil après le swing (`ElectricWireKoopa` passe de InWait à InCharge, nerf `ElectricWireKoopaNrvInCharge` 0x1cf5458 dans custom.sym, détecté via le hook `al::setNerve`). L’ancien hook `ElectricWire::endCapture` (fin d’un fil normal) a été retiré.
- **Press Start** (remplace "New Game", même valeur `TimerHookType::GameStart`) : = appuyer sur A sur l'écran de choix du mode d'une nouvelle partie ("Start" / "Start in Assist Mode", capture d'écran de l'utilisateur) → `SceneStateSelectMode` passe de Wait au nerve Decide (`SceneStateSelectModeNrvDecide` = 0x1d8b7e0), détecté dans le hook `al::setNerve`. Essais précédents retirés : bouton +, nerve wipe de `TitleMenuScene` (0x1d95c38, trop tôt), FadeToNewGame du menu pause.
- *(ancien)* **New Game depuis l'écran titre** : `TitleMenuScene` passe de son menu au nerve 0x1d95c38 (`TitleMenuSceneNrvWipe`) quand on valide (Start/A) ; événement si `TitleMenuScene::isNewGame` (emplacement vide ou "nouvelle partie"). Le nerve FadeToNewGame ci-dessous ne sert qu'au menu pause.
- **Liste des types dans les menus** : `VisibleTimerHookTypes` / `VisibleTimerHookTypeNames` (Menu.h) sans Capture Glow (non implémenté). L'enum `TimerHookType` garde toutes ses valeurs (réglages sauvegardés en u8). Ancien tableau `TimerHookTypes` supprimé.
- **Start Count / End Count** (réglages u8 `mTimerStartCount` / `mTimerEndCount`, ajoutés en fin de `Settings`, défaut 1, 0 traité comme 1, 1–99) : nombre d’événements du Start Type / End Type nécessaires pour lancer / arrêter le timer (ex. End Type Black Screen + End Count 3 = 3e écran noir). Compteurs `mStartEventCount` / `mEndEventCount` dans `Timer` : le compteur de fin ne compte que pendant que le timer tourne, les deux sont remis à 0 au start(), au reset() et quand on change un type ou un nombre. Valable pour tous les types.
- **New Game** (`TimerHookType::GameStart`, ancien "Game Start (Not Implemented)") : `HakoniwaSequence` passe au nerve 0x1d97880 (exécuteur 0x510e94 = copie inlinée de `exeFadeToNewGame`) quand on choisit une nouvelle partie → détecté dans le même hook `al::setNerve` que Black/White Screen (`HakoniwaSequenceNrvFadeToNewGame`). Noms affichés : "2 Moons (Grab)", "2 Moons (Tick)".
- **2 Moons Tick** : `Timer::onMoonTick` (appelé par le hook `ShineCounter::exeShineCountAdd`, step 1, qui envoie aussi `ShineTick`) compte les ticks du compteur de lunes depuis le dernier `start()` (`mMoonTickCount`). Nom affiché de `TwoMoons` changé en "2 Moons Grab".
- **2 Moons** : `Timer::onMoonGet` compte les lunes depuis le dernier `start()` (`mMoonCount`) et envoie `TwoMoons` toutes les 2 lunes. **Damage** : dans `NoDamageHook` (`GameDataFunction::damagePlayer`), envoyé même si No Damage est actif.

### A TESTER — Moon Shards Refresh sur une sauvegarde où la lune des fragments est déjà prise

- **Cause** : en plus de `ShineChip::init` (déjà patché via `ShineChipRefresh`), `ShineChipWatcher` vérifie `rs::isGotShine(lune des fragments)` ; si elle est déjà prise, il allume l'interrupteur "terminé" et les fragments restent cachés.
- **Correction** : les 8 `bl rs::isGotShine` du watcher (init +0x1d4a60/+0x1d4ba4, `createShineChips` +0x1d4c30 qui crée les fragments "vides"/gris si la lune est prise, `initAfterPlacement` +0x1d4df4/+0x1d4ff4/+0x1d50bc, `updateHints` +0x1d5174, `appear` +0x1d5314 → `ShineChipWatcherGotCheck1..8`) renvoient `false` quand l'option est active. 1re version (3 appels seulement) : fragments encore gris. Recharger la scène après avoir activé l'option.
- **Compteur en haut de l'écran** : `StageSceneLayout` / `ShineChipLayoutParts` ne l'affichent pas si `rs::isCurrentShineChipWatcherTypeEmpty` (= lune du watcher déjà prise) → trampoline qui renvoie `false` avec l'option.

### PUBLIÉ — v1.4.0 sur le compte GitHub mezfndu045-star
- v1.4.0 : sous-zones + images dans Stages, sticks retirés de Open Menu, timer Odyssey Power Up, crash Disable TP Puppet, ImGui sans attente GPU. Zips `docs/downloads/*-v1.4.0.zip` (binaire + 156 images romfs `ImGuiData/SubAreas` + section 3e du guide). La modif hakkun (`sys`, triple tampon ImGui) est dans le binaire mais pas dans le code publié (sous-module).

### PUBLIÉ — v1.3.0 sur le compte GitHub mezfndu045-star (commit 65d0857)
- v1.3.0 : site + README (Seeds Growing, Noclip, Start/End Count, types du timer, sauvegarde), zips `docs/downloads/*-v1.3.0.zip` (binaires à jour + section 3d du guide), v1.2.0 retirés. Instantané poussé avec pour parent le commit v1.2.0 (65827a7).

### PUBLIÉ — v1.2.0 sur le compte GitHub mezfndu045-star (13/10/2026)

- Dépôt : https://github.com/mezfndu045-star/BTT-Studio — site : https://mezfndu045-star.github.io/BTT-Studio/ (Pages depuis `main /docs`).
- Publié comme **instantané** (1 commit, sans historique) **sans** `.github/workflows/release.yml` : le jeton de ce compte n'a pas le scope `workflow`. Pour publier à nouveau : refaire un instantané (`git read-tree` / `git commit-tree` sans le workflow) et `git push` avec `credential.helper='!gh auth git-credential'`, compte gh actif = mezfndu045-star.
- Le remote local `origin` reste ademjab33-create/BTT-Studio (v1.1.0 en ligne là-bas, v1.2.0 non poussée).

### A TESTER — Menu Options en 2 colonnes (BTT-Studio 1.0)

- **Description** : La catégorie **Options** du menu vertical s'affiche sur 2 colonnes. Gauche : Moon Refresh → Skip Cloud (14). Droite : All Checkpoints → Disable Moon Lock, puis Reload Scene at Pos.
- **Implémentation** : champ `Category::columnSplit` dans `include/menu/Menu.h` (index du 1er élément de la colonne droite, fixé après "Skip Cloud" dans `initCategories`). Nouveau `Menu::drawTwoColumnCategory()`. Navigation : Haut/Bas bouclent dans la colonne, Droite passe à la colonne droite (même ligne), Gauche revient à gauche puis ferme la catégorie.
- **Statut** : compilé et déployé (13/10/2026), à vérifier en jeu.

### A TESTER — Option "Night Metro Multi Moon" (BTT-Studio 1.0)

- **Description** : au début de Ville de nuit (Pays de la Ville scénario 1), la Multi-Lune de Méca-Chenille apparaît au-dessus de Mario dès qu'il peut bouger ; la prendre joue la séquence normale → Ville de jour.
- **Implémentation** : `Mofumofu::init` (Méca-Chenille, placé seulement dans ce scénario) garde sa Multi-Lune à +0x1b8 (`rs::initLinkBossShine`) → capturée par un trampoline. Remise à zéro dans `StageScene::init`. Dans le hook `StageScene::control` : nerve `StageSceneNrvPlay` (0x1d8cc78) et `!rs::isActiveDemo(scene)` → `rs::appearPopupShineWithoutDemo(shine, posMario + 300 Y)` une seule fois.

### Arrivée après voyage forcé (suite "Always Odyssey Power Up") — CONFIRMÉ par l'utilisateur

- `StageScene::init` (main+0x4cb4dc) : royaume non débloqué → démarrage sans intro ni atterrissage → patch `StageSceneInitUnlockCheck`.
- Séquence "vaisseau de Bowser s'envole" (Pays des Sables, `rs::tryStartKoopaShipDemoHomeFlyAway`) ignorée pendant un level up forcé.
- `findKoopa` / `bossAttackHome` ignorés pendant la fermeture de la carte (`GameDataHolder::isFindKoopaNext` / `isBossAttackedHomeNext`).
- Destination non débloquée → `kill` de la carte la débloque (liste révélée = seulement la destination).
- Scénario principal de la destination (`GameDataFile+0x20 -> s32[world]`, -1 = pas commencé) mis à 0 → quêtes actives → caméra de scénario.
- Filet de sécurité : Odyssée bloquée sur le nerve d'attente d'atterrissage (0x1d2e580) 180 frames sans démo active → `validateEndEntranceCamera` + `showModelIfHide` + nerve Wait.

### A TESTER — Timer déplaçable (BTT-Studio 1.0)

- **Description** : comme l'Input Display. Menu ouvert → contour autour du timer, glisser-déposer à la souris (`##TimerDrag` dans `Timer::draw`). Catégorie Timer du menu vertical : "Preset Pos" (Center, Top Left, Top Center, Top Right, Bottom Left, Bottom Right), "Position X/Y" (pas de 25), "Reset Position". Position sauvegardée dans `mTimerPos` (déjà présent dans les réglages, coin haut gauche du texte).

### A TESTER — Timer "Odyssey Power Up" (BTT-Studio 1.0)

- **Description** : nouveau type `TimerHookType::OdysseyPowerUp` (Start Type / End Type, maintenant aussi réglables dans la catégorie Timer du menu vertical). Événement à la frame où l'écran devient entièrement noir APRÈS le message "L'Odyssée a été rechargée" : `exeDemoInformPowerUpMessage` met un flag, puis `exeGoToWorldMapWithFade` (wipe à +0x268, `WipeSimple::isCloseEnd`) déclenche l'événement quand le fondu est terminé. Pour la réparation / Odyssée attaquée : 1er step de `exeDemoUpLevelWaitFade` (déjà sur écran noir). 1re version (déclenchée à l'apparition du message) refusée par l'utilisateur : il faut l'écran noir. Start = End = Odyssey Power Up → le 1er message lance le timer, le suivant l'arrête.
- **Statut** : compilé et déployé (13/10/2026), à vérifier en jeu. "Always Odyssey Power Up" + "Odyssey In All Kingdoms" confirmés fonctionnels par l'utilisateur ; logs de diagnostic ODYDBG retirés.

### A TESTER — Option "Odyssey In All Kingdoms" (BTT-Studio 1.0)

- **Cause** : `ShineTowerRocket::init` appelle `GameDataFunction::isUnlockedWorld(currentWorld)` (main+0x309904, et +0x309938 pour le Pays des Chapeaux) et supprime l'Odyssée (vtable+0x30) si le royaume n'est pas débloqué dans la sauvegarde → pas d'Odyssée après un TP du menu Stages.
- **Implémentation** : les 2 `bl` remplacés (`OdysseyInitUnlockCheck1/2`) par `odysseyInitIsUnlockedWorld` → `true` si l'option est active.
- **Warp vers l'Odyssée depuis la carte** : la carte demande `GameDataFunction::isExistHome` = jeu fini OU (`isActivateHome` ET royaume courant débloqué). Trampoline : avec l'option, `isActivateHome` suffit.

### Carte du monde après un level up forcé (suite "Always Odyssey Power Up")

- `StageSceneStateWorldMap::appear` : liste des royaumes débloqués (+0x110, nombre +0x104, curseur +0x108). Si un royaume vient d'être débloqué (flag +0x150) → animation de révélation des royaumes `calcNextLockedWorld*ForWorldMap` (+0x118 nombre, +0x120 liste) et curseur sur le dernier. Avec 17 royaumes (save 100 %) le flag est effacé → pas d'animation.
- `kill` applique les règles de l'histoire à sa sélection (+0xfc) : Bowser → `bossAttackHome` (crash au Pays des Ruines) et `unlockWorld` des royaumes révélés.
- **Implémentation** : pendant un level up forcé, `calcNextLockedWorldNum/Id` (Scene + LayoutActor) renvoient 1 / royaume suivant de l'histoire, `getUnlockWorldNumForWorldMap(Scene)` renvoie 16 au lieu de 17, curseur placé sur ce royaume après `appear`. Dans `kill` : sélection = royaume redirigé et +0x118 = 0 (rien n'est débloqué dans la sauvegarde). B (`cancelWorldMap`) annule normalement.

### A TESTER — Option "Always Odyssey Power Up" (BTT-Studio 1.0)

- **Description** : toucher l'Odyssée joue la cinématique "Odyssée rechargée" (level up), même avec 0 lune et même si le pays suivant est déjà débloqué. À chaque passage.
- **Caméra bloquée à l'arrivée (corrigé)** : le dépôt se déclenche quand Cappy vise l'Odyssée (`rs::isMsgCapStartLockOn` dans `receiveMsg`). À l'arrivée Mario apparaît collé à l'Odyssée → le dépôt "sans lune" démarrait par-dessus la caméra d'arrivée. Désormais le faux dépôt n'est autorisé qu'après que Mario s'est éloigné une fois de l'Odyssée (> 1500 unités, vérifié dans `exeWait` via `al::isNearPlayer`, remis à zéro dans `init`). Les vraies lunes se déposent normalement.
- **Sans lune** : `ShineTowerRocket::receiveMsg` lit `GameDataFunction::getCurrentShineNum` (main+0x30c37c) et ne lance le dépôt que si ≥ 1. Ce `bl` est remplacé (`OdysseyGetCurrentShineNum`) pour renvoyer 1 quand il n'y a aucune lune (`sIsOdysseyFakePayment`), puis `mPayShineNum` (+0x12c) est remis à 0 juste avant `tryLevelUp` → `addPayShine(0)`, aucune lune fantôme dans la sauvegarde. `addPayShine` n'est appelé que dans `tryLevelUp`.
- **Fonctionnement du jeu (1.0.0)** : `ShineTowerRocket` (= l'Odyssée). La fonction non exportée main+0x30da68 `(rocket, worldId, payShineNum) -> bool` teste si les lunes payées atteignent le prochain déblocage (`findUnlockShineNum` vs `getPayShineNum`). Appelée par `exeDemoWalkPlayerToPoint`, `exeDemoScaleUp` et `tryLevelUp` (qui paie les lunes via `addPayShine` puis lance le level up).
- **Destination** : la destination du décollage est `ShineTowerRocket+312`, écrite par `decideWorldMap(worldId)` (appelé par `StageSceneStateWorldMap`). Sans correction le jeu envoie au royaume le plus avancé de la sauvegarde. Si un level up a lieu avec l'option active (`sIsOdysseyPowerUpForced`, mis dans le hook du test ET dans `tryLevelUp` ; 1re version qui ne le mettait que si le vrai test disait non = ne marchait pas sur une sauvegarde avancée, ex. Chutes avec une save à Ville → Neiges), `decideWorldMap` reçoit le royaume suivant (dans l'ordre de l'histoire) du royaume **mémorisé au moment du level up** (`sOdysseyPowerUpWorldId` = `worldId` reçu par le test). Ne PAS utiliser `getCurrentWorldId` dans `decideWorldMap` : à ce moment le "monde courant" est déjà la sélection de la carte (Chutes + save à Ville → Neiges → donnait Mer) (`getStoryNextWorldId`) : Chapeaux > Chutes > Sables > Lac > Bois > Nuages > Oublié > Ville > Neiges > Mer > Gourmets > Ruines > Bowser > Lune. Index : Hat 0, Waterfall 1, Sand 2, Forest 3, Lake 4, Cloud 5, Clash 6, City 7, Sea 8, Snow 9, Lava 10, Boss 11, Sky 12, Moon 13. Flag remis à zéro par `cancelWorldMap` et `ShineTowerRocket::init`.
- **Implémentation** : trampoline sur `OdysseyIsLevelUpByPay = 0x0030da68` (`syms/custom.sym`) → `true` si option active et `payShineNum > 0`, tant que `sIsOdysseyPowerUpDone` est faux. `tryLevelUp` réussi → flag mis (sinon le jeu redemande pour enchaîner les niveaux = boucle infinie + lunes repayées). Flag remis à zéro dans `ShineTowerRocket::exeWait`. Symboles dans `syms/game/MapObj/ShineTowerRocket.sym`.
- **Statut** : compilé et déployé localement (13/10/2026), pas publié sur GitHub, à vérifier en jeu.

### A TESTER — Option "Skip Broodals" (BTT-Studio 1.0)

- **Description** : option dans Options (colonne droite) + menu ImGui. Dès la fin de la cinématique de début de combat, le boss est vaincu via son propre code de "dernier coup" → le jeu joue la cinématique de fin et fait apparaître la Multi-Lune comme d'habitude.
- **Boss gérés (noms internes)** : Hariet = `BombTail`, Spewart = `FireBlower` (+Lv2/Lv3), Rango = `CapThrower`, Madame Broode = `Breeda`. Topper = `Stacker` n'est PAS géré (aucune Multi-Lune liée).
- **Implémentation** : trampolines sur `<Boss>::control()` (`syms/game/Boss/Broodals.sym`), détection de la sortie du nerve "BattleStartDemo" (`isBroodalBattleStarting`). Hariet/Spewart : PV (+0x108) = 1 puis `damageAndSetNerve()` / `damageOrDie()`. Rango : PV (+0x158) = 0 puis nerve `BattleEndDemo`. Broode : nerve `DamageLast` (appelle `rs::endBossBattle`). Adresses des nerves (1.0.0 uniquement) dans `syms/custom.sym` (`*Nrv*`), trouvées via les tables de relocations de l'ExeFS.
- **Réglages** : `SETTING(SkipBroodals)` ajouté **à la fin** de `Settings`. `SaveFileHelper::loadSettings` accepte maintenant un fichier plus petit (anciens réglages conservés). Toujours ajouter les nouveaux réglages à la fin.
- **Statut** : compilé et déployé (13/10/2026), à vérifier en jeu.

### A TESTER — Luigi Pixels = vrai objet du jeu (BTT-Studio 1.0)

- **Description** : l'ancienne animation ImGui (`LuigiPixels.cpp/.h`, sprites dessinés + 18 spots inventés) est **supprimée** (remplace l'entrée "Détection, récompenses, toasts et affichage 3D des Pixel Luigi" plus bas). L'option "Luigi Pixels" force maintenant le vrai Luigi 8-bit officiel.
- **Fonctionnement du jeu (SMO 1.0.0, analysé depuis l'ExeFS dumpé par Yuzu)** : un item de type `DotMarioCat` (type 19) tire `al::isPercentProbability(10.0f)` à l'init (main+0x5556f4) : 10 % → item `ドットキャラクター(レア)` = `DotCharacterRare` (modèle `DotMarioCatRare`, sort du sol, pièces, s'enfuit), sinon `コイン[自動取得]`. Spawn via `ProjectItemDirector::appearItem` → `DotCharacterRare::appearPopUp`.
- **Implémentation** : `LuigiRareRoll = 0x005556f4` (@smo:100) dans `syms/custom.sym`, `writeBranchLinkAtSym<"LuigiRareRoll">(luigiRareRoll)` dans `SettingsHooks.cpp` → renvoie `true` si l'option est active, sinon le tirage normal. `_ZN2al20isPercentProbabilityEf` ajouté à `syms/al/Library/Math/MathUtil.sym`. Le tirage a lieu au chargement du stage : recharger la scène après avoir changé l'option.
- **Statut** : compilé et déployé (13/10/2026), à vérifier en jeu.

### A TESTER — Photo à la place de l'écran Joy-Con au boot (BTT-Studio 1.0)

- **Description** : `romfs/LayoutData/BootLoading.szs` (Yuzu `Practice mod 1.0` uniquement) remplace la texture `MyControllerJoyCon^w` (960x540 ASTC 4x4 sRGB) par une photo.
- **Piège** : reconstruire la SARC avec oead `SarcWriter` décale `layout.lyarc` (0x2000 → 0xd0) et casse l'alignement de la BNTX → crash `nn::gfx::TextureImpl::Initialize` dans `BootLayout`. Il faut patcher les octets de la texture en place dans l'archive d'origine et recompresser en Yaz0 avec alignement 0x2000.

### RESOLU — Écosystème GitHub Publié : Dépôt Public, Release v1.0.0 & Showcase Pages en Ligne (BTT-Studio 1.0)

- **Description** : Publication officielle et intégration complète de l'écosystème GitHub sous le compte utilisateur **`ademjab33-create`** :
  1. **Dépôt GitHub public créé et synchronisé** :
     - 🔗 **Dépôt officiel** : [https://github.com/ademjab33-create/BTT-Studio](https://github.com/ademjab33-create/BTT-Studio)
  2. **Release Officielle v1.0.0 publiée avec 3 packages ZIP** :
     - 🔗 **Page de Release** : [https://github.com/ademjab33-create/BTT-Studio/releases/tag/v1.0.0](https://github.com/ademjab33-create/BTT-Studio/releases/tag/v1.0.0)
     - `BTT-Studio-1.0-Yuzu-v1.0.0.zip` (4.53 Mo)
     - `BTT-Studio-1.0-SimpleModManager-v1.0.0.zip` (4.53 Mo)
     - `BTT-Studio-1.0-Atmosphere-v1.0.0.zip` (4.53 Mo)
  3. **Site Web Showcase en ligne (GitHub Pages)** :
     - 🔗 **Site officiel en direct** : [https://ademjab33-create.github.io/BTT-Studio/](https://ademjab33-create.github.io/BTT-Studio/)
     - Déployé nativement depuis `main /docs` avec bypass `.nojekyll`.
     - Simulateur d'Input Display déplaçable avec effet Multicolor Rainbow actif.
     - Cartes de téléchargement direct en 1 clic pour Yuzu, SMM et Atmosphere.
  4. **Templates communautaires intégrés** :
     - `.github/ISSUE_TEMPLATE/bug_report.yml`, `feature_request.yml` et `pull_request_template.md`.
- **Statut** : RESOLU (13/10/2026)

### RESOLU — Déplacement libre de l'Input Display & Couleur Multicolor / RGB Rainbow (BTT-Studio 1.0)

- **Description** : L'Input Display du Practice Mod 1.0 peut désormais être déplacé librement à la manette ou à la souris, et intègre une option de couleur dynamique "Multicolor" (RGB Rainbow) pour tous ses éléments :
  1. **Déplacement libre à la souris (Drag & Drop)** : lorsque le menu est ouvert, un contour interactif discret entoure l'Input Display. Il suffit de cliquer et glisser avec la souris/tactile pour déplacer instantanément l'Input Display n'importe où sur l'écran (supporte le Player 1 et Player 2 séparément).
  2. **Contrôle précis et presets à la manette** : dans la catégorie **Input Display** du menu vertical, ajout de :
     - "Preset Pos" (Center, Bottom Left, Bottom Right, Top Left, Top Right) pour placer instantanément le controller dans les 4 coins idéaux de l'écran.
     - "Position X" et "Position Y" avec réglage fin par pas de 25 pixels (support du nouveau `mStep` et callback `onChange` dans `IntMenuComponent`).
     - Bouton "Reset Position" réinitialisant la position au centre.
  3. **Couleur "Multicolor" (RGB Rainbow dynamique)** :
     - Ajout de l'énumération `Multicolor` dans `SettingsMgr::InputDisplayColor` et de son nom dans `sInputDisplayColorNames`.
     - Génération procédurale d'un dégradé arc-en-ciel animé (`ImGui::ColorConvertHSVtoRGB` + `ImGui::GetTime()` avec déphasage spatial subtil entre chaque bouton).
     - Utilisable indépendamment ou simultanément pour : **Button Color**, **Pressed Color**, **Stick Color**, **Ring Color** et **BG Color** (avec transparence 140 adaptée pour le fond).
  4. Binaire `subsdk4` (598 724 octets) compilé avec succès et déployé dans Yuzu (`load/0100000000010000/Practice mod 1.0/exefs/subsdk4`), SimpleModManager (`Practice mod 1.0/contents/0100000000010000/exefs/subsdk4`) et Downloads (`Practice mod 1.0/atmosphere/contents/0100000000010000/exefs/subsdk4`).
- **Statut** : RESOLU (13/10/2026)

### RESOLU — Caméra instantanée & Vitesse zéro sur Load Position (BTT-Studio 1.0)

- **Description** : Lors de l'utilisation de **Load Position** (touche rapide `D-Pad Droite` ou bouton dans le menu vertical / ImGui), la caméra se replace désormais **instantanément sur Mario sans aucun glissement ni interpolation lente**, exactement comme dans le BTT-Studio d'origine et la version 1.3 :
  1. **Annulation de l'interpolation caméra multi-frame** : appel direct à `al::requestCancelCameraInterpole(target, 0)` et `al::requestCancelCameraInterpole(stageScene, 0)` au moment du chargement, prolongé sur une boucle de 10 frames (`mCancelCameraInterpoleFrames = 10`) dans `handleAlways()`. Cela empêche le système de caméra d'Odyssey de lisser la transition depuis l'ancienne position et verrouille immédiatement la vue sur Mario.
  2. **Annulation de la vitesse résiduelle (`al::setVelocityZero`)** : Mario et ses captures (`hack`) voient leur vecteur de vitesse réinitialisé à zéro lors du chargement, évitant tout effet d'inertie ou de secousse physique résiduelle.
  3. **Support complet des captures / transformations (`hack`)** : `saveTeleport()` et `loadTeleport()` ciblent automatiquement l'acteur capturé si Mario en possède un (`mHackActor`), avec désactivation/réactivation temporaire des collisions.
  4. Binaire `subsdk4` (596 080 octets) compilé avec succès et déployé dans Yuzu (`load/0100000000010000/Practice mod 1.0/exefs/subsdk4`), SimpleModManager (`Practice mod 1.0/contents/0100000000010000/exefs/subsdk4`) et Downloads (`Practice mod 1.0/atmosphere/contents/0100000000010000/exefs/subsdk4`).
- **Statut** : RESOLU (13/10/2026)

### RESOLU — Reload at Pos / Instant Reload (Rechargement de scène à la position actuelle de Mario - BTT-Studio 1.0)

- **Description** : Ajout et intégration complète de l'option "Reload at Pos" (recharge la scène entière tout en replaçant Mario instantanément à ses coordonnées et rotation actuelles) ainsi que du bouton d'action directe "Restart at Pos" dans le Practice Mod 1.0 :
  1. **Option "Reload at Pos" (Yes/No)** intégrée dans la catégorie **Misc** et **Options** du menu vertical (et synchronisée avec le menu ImGui). Quand cette option est active, chaque rechargement de scène (que ce soit via le bouton "Restart Scene" ou le raccourci manette `D-Pad Haut` / `ZL+ZR`) recharge entièrement le monde et replace immédiatement Mario à sa position exacte avec restauration de ses PV et sans cinématique d'introduction.
  2. **Bouton d'action directe "Restart at Pos"** dans la catégorie **Misc** : permet d'exécuter un rechargement au point actuel à la demande en un seul clic, même si le toggle global est désactivé.
  3. **Machine d'état robuste anti-désynchronisation (`mReloadState`)** : attend proprement le déchargement de l'ancienne scène (`mReloadState = 1`), puis l'initialisation complète de la nouvelle scène et de l'acteur Mario (`mReloadState = 2`), applique la téléportation (`al::setTrans`, `al::updatePoseQuat`, `al::setVelocityZero`), annule l'interpolation de la caméra (`al::requestCancelCameraInterpole`), et affiche une notification à l'écran ("Reloaded at Position").
  4. Binaire `subsdk4` (596 065 octets) compilé avec succès et déployé dans Yuzu (`load/0100000000010000/Practice mod 1.0/exefs/subsdk4`), SimpleModManager et Downloads.
- **Statut** : RESOLU (13/10/2026)

### RESOLU — Détection, récompenses, toasts et affichage 3D des Pixel Luigi avec coordonnées in-game authentiques (BTT-Studio 1.0)

- **Description** : Les 18 emplacements officiels de Pixel Luigi fonctionnent intégralement en version **SMO 1.0.0** avec une **reproduction 1:1 exacte de l'animation officielle (vidéo 1011.mp4)** :
  1. **Impact au sol & onde de choc** : anneau de nuages de poussière blanche jaillissant en cercle au point d'impact de la charge au sol (`ZL`/`ZR`).
  2. **Jaillissement & saut acrobatique en cloche** (frames 0 à 44) : Luigi jaillit du sol en grand saut acrobatique (sprite officiel de saut/course SMB1, gants blancs, casquette verte, salopette bleue) culminant à 280 unités tout en effectuant sa pirouette sur son axe vertical.
  3. **Atterrissage à plat / Plaquage au sol** (frames 45 à 58) : Luigi atterrit à plat ventre horizontalement sur l'herbe/le sol (sprite tourné à 90°, tête à droite et pieds à gauche).
  4. **Rebond & posture fière debout** (frames 59 à 72) : petit rebond dynamique vers le haut, Luigi se redresse sur ses pieds face caméra avec son ombre portée au sol.
  5. **Flèche blanche indicatrice (⬇)** : une flèche blanche vers le bas avec bordure sombre apparaît au-dessus de sa tête et flotte avec un balancement subtil (*bobbing*).
  6. **Deux pièces d'or géantes 3D en rotation (🪙🪙)** : deux grosses pièces dorées flottent verticalement au-dessus de la flèche, tournent sur elles-mêmes en 3D avec des étincelles d'étoiles scintillantes, pendant que le compteur de pièces du joueur s'incrémente de manière progressive et fluide (+4/+2 par frame avec son de jackpot).
  7. **Réaction au coup de Cappy / contact Mario** : si Mario le touche ou lui lance Cappy, Luigi bondit à nouveau dans les airs en effectuant une pirouette tourbillonnante rapide à 360° avec projection d'étoiles avant de se reposer au sol.
  8. **Réinitialisation au Restart Scene** : l'état des spots est réinitialisé (`LuigiPixels::resetDiscoveredSpots`) lors d'un `Restart Scene` (hotkey ou bouton de menu) ou d'un rechargement de stage, permettant de ré-obtenir à chaque essai les **+200 pièces** pour l'entraînement aux routes de speedrun.
- **Cause & Implémentation** :
  1. Matrice exacte des 2 sprites 8-bit officiels : posture debout (`sLuigiSprite`) et posture de saut acrobatique SMB1 (`sLuigiJumpSprite`) avec gants blancs.
  2. Rendu complet des 5 phases d'animation fidèles à la vidéo `1011.mp4` dans `LuigiPixels::draw()` : arc de saut à 280 unités avec rotation, atterrissage à plat ventre horizontal (`isProne = true`), rebond de redressement, flèche ⬇ vectorielle blanche, 2 pièces d'or 3D rotatives et étincelles rétro.
  3. Binaire `subsdk4` (595 031 octets) compilé avec succès et déployé dans Yuzu (`load/0100000000010000/Practice mod 1.0/exefs/subsdk4`), SimpleModManager (`Practice mod 1.0/contents/0100000000010000/exefs/subsdk4`) et Downloads (`Practice mod 1.0/atmosphere/contents/0100000000010000/exefs/subsdk4`).
- **Statut** : RESOLU (11/10/2026)

### RESOLU — Blocage des entrées du jeu pendant la navigation dans le menu (Practice Mod 1.3)

- **Description** : Synchronisation avec la version 1.0 : lorsque le menu du Practice Mod 1.3 est ouvert, les touches de la manette (sauts, déplacements de Mario, caméra, interactions) ne sont plus captées par le jeu. Mario reste parfaitement immobile pendant la navigation dans le menu.
- **Cause & Implémentation** :
  1. Dans `Practicemod1.3/source/src/program/imgui_nvn.cpp`, `disableButtons` ne vérifiait que `InputHelper::isInputToggled()` (inactif par défaut) et ne vérifiait pas `pe::Menu::instance()->isEnabled()`. De plus, seul l'échantillon 0 était vidé au lieu des `count` échantillons.
  2. Mise à jour de `disableButtons(state, count)` pour vider systématiquement les boutons (`NpadButtonSet()`) et sticks (`AnalogStickState()`) sur l'ensemble des échantillons `state[0..count-1]` dès que `pe::Menu::instance()->isEnabled()` est vrai (tout en préservant `InputHelper::isReadInputs()`).
  3. Binaire `subsdk9` (1 022 162 octets) compilé avec succès et déployé dans Yuzu (`load/0100000000010000/Practice mod 1.3/exefs/subsdk9`), `BTT-Studio-Switch`, SimpleModManager et Downloads.
- **Statut** : RESOLU (11/10/2026)

### RESOLU — Blocage des entrées du jeu pendant la navigation dans le menu (BTT-Studio 1.0)

- **Description** : Lorsque le menu du Practice Mod est ouvert, les touches de la manette (sauts, déplacements de Mario, caméra, interactions) et les raccourcis hotkeys ne sont plus interceptés par le jeu. Mario reste parfaitement immobile pendant la navigation dans le menu.
- **Cause & Implémentation** :
  1. Dans `src/main.cpp`, la fonction `disableButtons(state)` vérifiait `InputHelper::isInputToggled()`, une variable résiduelle issue d'un ancien raccourci (`R + ZR + L`) qui était à `false` par défaut. Par conséquent, les hooks `DisableFullKeyState`, `DisableHandheldState`, `DisableJoyDualState`, `DisableJoyLeftState` et `DisableJoyRightState` ne masquaient jamais les entrées au jeu.
  2. Mise à jour de `disableButtons(state, count)` pour vider systématiquement les boutons (`NpadButtonSet()`) et sticks (`AnalogStickState()`) sur tous les échantillons d'état (`state[0..count-1]`) tant que `btt::Menu::instance()->mIsEnabledMenu` est actif (en préservant la lecture interne du menu via `isReadInputs()`).
  3. Sécurisation de `handleHotkeys()`, `isTpKey()` et `isHotkey()` dans [PageHotkeys.cpp](file:///c:/Users/FlowUP/.gemini/antigravity-ide/scratch/BTT-Studio/src/menu/pages/PageHotkeys.cpp) avec `if (mIsEnabledMenu) return;` afin d'empêcher tout déclenchement de raccourci (Save/Load Pos, Restart Scene) lors des pressions sur le D-Pad dans le menu.
  4. Binaire `subsdk4` (588 Ko) recompilé et déployé dans Yuzu (`load/0100000000010000/Practice mod 1.0/exefs/subsdk4`), SimpleModManager et Downloads.
- **Statut** : RESOLU (11/10/2026)

### RESOLU — Sélecteur d'Outfit (Costume & Chapeau) et bouton Apply Outfit (BTT-Studio 1.0)

- **Description** : Ajout du sélecteur complet de costumes ("Outfit Body", 42 choix) et de chapeaux ("Outfit Cap", 42 choix) ainsi que du bouton d'action **"Apply Outfit"** dans la catégorie **Misc** du menu vertical, permettant de changer de tenue instantanément.
- **Implémentation** :
  1. Intégration de deux `EnumMenuComponent<int>` ("Outfit Body" et "Outfit Cap", 42 options chacune couvrant toutes les tenues de SMO) et d'un `ButtonMenuComponent("Apply Outfit")` dans la catégorie Misc (`mCategories[2]`) de [Menu.cpp](file:///c:/Users/FlowUP/.gemini/antigravity-ide/scratch/BTT-Studio/src/menu/Menu.cpp).
  2. Implémentation de la méthode `Menu::applyOutfit()` : applique les tenues sélectionnées via `GameDataFunction::wearCap` et `GameDataFunction::wearCostume` sur `GameDataHolderWriter`, recharge immédiatement la scène (`stageScene->kill()`) pour que Mario apparaisse vêtu de son nouvel outfit, et affiche une notification à l'écran ("Outfit Applied").
  3. Mise à jour de [PageMisc.cpp](file:///c:/Users/FlowUP/.gemini/antigravity-ide/scratch/BTT-Studio/src/menu/pages/PageMisc.cpp) pour brancher également le bouton "Apply Outfit" de l'interface ImGui classique sur cette méthode centralisée.
  4. Suppression d'une double déclaration résiduelle de `restartScene()` dans [Menu.h](file:///c:/Users/FlowUP/.gemini/antigravity-ide/scratch/BTT-Studio/include/menu/Menu.h).
  5. Binaire `subsdk4` (588 Ko) compilé avec succès et déployé dans Yuzu (`load/0100000000010000/Practice mod 1.0/exefs/subsdk4`), SimpleModManager et Downloads.
- **Statut** : RESOLU (11/10/2026)

### RESOLU — Restauration de l'écran de démarrage normal (Suppression de SpeedbootLoad)

- **Description** : L'écran de démarrage personnalisé de BTT-Studio ("Hiiiiiiii :3", "BTT-Studio © 2026, MrKatzenGaming") a été retiré pour restaurer l'écran de démarrage officiel / normal (vanilla) de Super Mario Odyssey.
- **Cause & Implémentation** :
  1. Dans `src/main.cpp`, le hook `customboot::prepareCustomBootHook` sur `_ZN10BootLayoutC1ERKN2al14LayoutInitInfoE` (`BootLayout::BootLayout`) a été désactivé. Le jeu exécute désormais son constructeur `BootLayout` officiel sans injecter `CustomBootLoad`.
  2. Dans `include/menu/Menu.h`, `mIsEnabledMenu` est désormais initialisé à `false` par défaut au démarrage afin que le menu latéral ne vienne pas s'afficher par-dessus l'écran de titre ou de jeu au boot (le menu s'ouvre normalement avec `L + D-Pad Gauche`).
  3. L'archive d'interface personnalisée `SpeedbootLoad.szs` a été désactivée / renommée en `.szs.bak` dans tous les dossiers de déploiement.
  4. Le binaire `subsdk4` (588 Ko) a été recompilé et déployé dans Yuzu, SimpleModManager et Atmosphere.
- **Statut** : RESOLU (11/10/2026)

### RESOLU — Cutscene Refresh & Dialogues PNJ (BTT-Studio 1.0)

- **Description** : L'option "Cutscene Refresh" dans le menu Options réactive désormais toutes les cinématiques du jeu (introduction de monde, scénarios, roche lunaire, première chapimorphose comme la grenouille de Bonneton) ainsi que les premiers dialogues de tous les PNJ majeurs (Capitaine Toad, Hint Toad, Jammin' Toad, Traveling Peach) et la cinématique d'arrivée dans les royaumes (Odyssée).
- **Cause du bug** :
  1. `DemoHooks.hpp` ne vérifiait que `mIsEnableAlwaysManualCutscene` et ignorait totalement `mIsEnableRefreshNpc` (la variable associée à "Cutscene Refresh" dans l'UI). Les cinématiques de première capture (`isEnableShowHackDemo`), de monde (`isFirstDemoWorldIntroCamera`), de scénario (`isFirstDemoScenarioStartCamera`) et d'event (`isFirstDemo`) ne se rejouaient donc jamais.
  2. Dans `SettingsHooks.cpp`, seul le Capitaine Toad (`isOnFlagKinopioBrigadeNpcFirstTalk`) était hooké. Hint Toad, Jammin' Toad et Traveling Peach n'étaient pas hookés.
  3. `kingdomEnterHook` et `warpTextHook` ne vérifiaient pas `mIsEnableRefreshNpc`.
- **Solution & Implémentation** :
  1. Mise à jour de `DemoHooks.hpp` pour que `HOOK_CALLBACK` vérifie `(mIsEnableAlwaysManualCutscene || mIsEnableRefreshNpc)`.
  2. Ajout des symboles dynamiques dans `GameDataUtil.sym` pour `isOnFlagHintNpcFirstTalk`, `isTalkCollectBgmNpc`, et `isTalkWorldTravelingPeach`.
  3. Ajout et installation des hooks dédiés `hintToadRefreshHook`, `musicToadRefreshHook`, `peachRefreshHook` dans `SettingsHooks.cpp`.
  4. Mise à jour de `kingdomEnterHook` et `warpTextHook` pour se réinitialiser lorsque `mIsEnableRefreshNpc` est actif.
  5. Binaire `subsdk4` (594 Ko) recompilé et déployé dans Yuzu (`load/0100000000010000/Practice mod 1.0/exefs/subsdk4`) et SimpleModManager.
- **Statut** : RESOLU (11/10/2026)

### RESOLU — Warp Text Refresh & Checkpoint Flags (BTT-Studio 1.0)

- **Description** : L'option "Warp Text Refresh" réactive correctement l'explication complète de téléportation par Cappy lorsqu'on touche un drapeau de checkpoint.
- **Cause du bug** :
  1. Seule la fonction `GameDataFunction::isAlreadyShowExplainCheckpointFlag` était hookée. La fonction d'activation `GameDataFunction::isEnableShowExplainCheckpointFlag` (qui décide si l'explication est autorisée à s'afficher) et la méthode sous-jacente `GameDataFile::isAlreadyShowExplainCheckpointFlag` n'étaient pas hookées et continuaient de renvoyer que l'explication était désactivée ou déjà affichée.
  2. Si l'option "No Checkpoint Touch" était active, elle empêchait l'interaction physique avec le drapeau de checkpoint.
- **Solution** :
  1. Ajout des symboles `_ZN16GameDataFunction33isEnableShowExplainCheckpointFlagE22GameDataHolderAccessor` et `_ZNK12GameDataFile34isAlreadyShowExplainCheckpointFlagEv` dans `GameDataFunction.sym`.
  2. Installation des hooks dédiés dans `SettingsHooks.cpp` (`isEnableShowExplainHook` renvoie `true`, `warpTextHook` et `gameDataFileAlreadyShowExplainHook` renvoient `false`).
  3. Mise à jour de `checkpointFlagHook` pour ne pas bloquer l'enregistrement du drapeau quand `mIsEnableRefreshWarpText` est actif.
- **Statut** : RESOLU (11/10/2026)

### RESOLU — Touches personnalisables Save Position, Load Position et Restart Scene (BTT-Studio 1.0)

- **Description** : Ajout de la possibilité de configurer librement les touches de "Save Position", "Load Position" et "Restart Scene" directement depuis la catégorie **Keybinds** du menu, avec 20 choix possibles (D-Pad Left/Right/Up/Down, combinaisons avec L, ZL, R, LStick, RStick, ZL+ZR, ou Disabled).
- **Implémentation** :
  1. Enum `TpKey` (20 touches/combinaisons) et variables `mSavePosKey` (défaut `DLeft`), `mLoadPosKey` (défaut `DRight`) et `mRestartSceneKey` (défaut `DUp`) dans [SettingsMgr.h](file:///c:/Users/FlowUP/.gemini/antigravity-ide/scratch/BTT-Studio/src/settings/SettingsMgr.h).
  2. Intégration des composants `EnumMenuComponent<SettingsMgr::TpKey>` ("Save Position", "Load Position", "Restart Scene") dans `mCategories[5]` de [Menu.cpp](file:///c:/Users/FlowUP/.gemini/antigravity-ide/scratch/BTT-Studio/src/menu/Menu.cpp) et combos dans [PageHotkeys.cpp](file:///c:/Users/FlowUP/.gemini/antigravity-ide/scratch/BTT-Studio/src/menu/pages/PageHotkeys.cpp). Ajout d'un bouton d'action directe "Restart Scene" dans la catégorie Misc (`mCategories[2]`).
  3. Détection dynamique stricte via `Menu::isTpKey(TpKey key)` dans [PageHotkeys.cpp](file:///c:/Users/FlowUP/.gemini/antigravity-ide/scratch/BTT-Studio/src/menu/pages/PageHotkeys.cpp) : si configuré sur `DUp` (Flèche Haut), SEULE la flèche du haut redémarre la scène (suppression du trigger parasite ZL+ZR). ZL+ZR ne redémarre la scène que si ZL_ZR est explicitement sélectionné dans le menu.
  4. Méthode centralisée `Menu::restartScene()` avec restauration de vie Mario (`recoveryPlayer`) et recharge de stage propre.
  5. Ajout de toasts / notifications visuelles à l'écran ("Position Saved", "Position Loaded", "No Saved Position") avec auto-dismiss de 2 secondes.
  6. Recompilation du binaire `subsdk4` et déploiement synchronisé dans Yuzu (`load/0100000000010000/Practice mod 1.0/exefs/subsdk4`) et les dossiers de mod SimpleModManager.
- **Statut** : RESOLU (11/10/2026)

### RESOLU — Softlock / Crash ouverture menu Options BTT-Studio 1.0 (operator new / HeapSourceDynamic)

- **Description** : Lorsqu'on ouvrait le menu et qu'on appuyait sur la flèche de droite pour entrer dans la catégorie "Options", le jeu softlockait / crashait avec un crash ARM à l'adresse 0x4 (`Unmapped Read64 @ 0x0`, `pc = 0x00000004` dans `Menu::drawExpandedCategory`).
- **Cause** : L'addon `HeapSourceDynamic` redirigeait `operator new`, `malloc` et `free` vers les symboles dynamiques `hk_Znwm` et `hk_malloc`. Or dans la version 1.0.0 (comme tout binaire Switch retail), ces symboles ne sont pas exportés dynamiquement dans `RedStar.nss`. `operator new` renvoyait donc `nullptr` (0x0). Lors de l'initialisation du menu, chaque `new BoolMenuComponent` était instancié sur l'adresse nulle (écrivant sa vtable et ses membres à 0x0, 0x10, etc.), et le premier appel à `comp->getSize()` sautait sur un pointeur de vtable invalide (0x4).
- **Solution** :
  1. Suppression de `HeapSourceDynamic` de `config/config.cmake`.
  2. Implémentation d'un gestionnaire global d'allocation mémoire propre (`src/helpers/heapHelper.cpp`) branché directement sur `btt::sHeap` (8 Mo `sead::ExpHeap`) avec une arène statique de fallback sécurisée (512 Ko) pour les allocations très précoces.
  3. Sécurisation des classes de `MenuComponent.h` pour utiliser `ImGui::GetFont()` au lieu d'un accès vectoriel direct risqué `Fonts[0]`.
  4. Ajout de vérifications de pointeurs nuls défensives dans `Menu::drawExpandedCategory` et `Menu::initCategories`.
  5. Recompilation du binaire `subsdk4` et déploiement synchronisé dans Yuzu et les dossiers Atmosphere.
- **Statut** : RESOLU (11/10/2026)

### RESOLU — Crash au boot BTT-Studio 1.0 (Out of Memory font Rodin)

- **Description** : Le jeu 1.0.0 crashait dès le lancement lors de l'initialisation du menu (`sead::ExpHeap::tryAlloc` -> `HaltWithDetail`).
- **Cause** : `BTTStudioHeap` était créé avec seulement 2 Mo (`2_MB`), or la police `RodinNTLG-Bold.otf` fait à elle seule 2.21 Mo, saturant immédiatement le tas mémoire `sead::ExpHeap`. De plus, `main.npdm` généré par senobi provoquait un échec de placeholder Yuzu.
- **Solution** :
  1. Taille du tas portée à 8 Mo (`8_MB`) dans `src/main.cpp`.
  2. Allocation sécurisée dans `src/helpers/fsHelper.cpp` (`tryAlloc` avec fallback sur `malloc`).
  3. `main.npdm` propre de 1 516 octets restauré et `subsdk4` généré avec les checksums officiels `elf2nso.exe`.
- **Statut** : RESOLU (11/10/2026)

### RESOLU - Cutscene Refresh & Warp Text Refresh

- **Description** : Les options "Cutscene Refresh" (dialogues et cinematiques PNJ/Toads), "Kingdom Enter Cutscenes Refresh" (cinematique d'arrivee de l'Odyssee dans chaque royaume) et "Warp Text Refresh" (texte d'explication de teleportation des checkpoints) se reinitialisent et reapparaissent correctement.
- **Cause & Implementation SMO 1.3.0** :
  - **Warp Text Refresh** : La fonction `GameDataFunction::isAlreadyShowExplainCheckpointFlag` (offset `0x004d3880`) est branchee sur `warpTextHook`. Quand l'option est activee, elle renvoie `false`, forcant le jeu a afficher le dialogue explicatif a chaque contact avec un drapeau de checkpoint.
  - **Kingdom Enter Cutscenes Refresh** : La fonction `GameProgressData::isFirstTimeGoWorld` (offset `0x004e1060`) a ete corrigee et branchee sur `kingdomEnterHookStatic`. Elle renvoie `true` lorsque le refresh est actif, forcant le jeu a considerer chaque arrivee dans un royaume comme la premiere et a relancer la cinematique d'entree / survol / atterrissage.
  - **Cutscene Refresh (PNJ / Toads)** :
    - Brigade Toad / Capitaine Toad : `rs::isOnFlagKinopioBrigadeNpcFirstTalk` a l'offset `0x004dd5a0`.
    - Toad aux indices (Hint Toad) : `rs::isOnFlagHintNpcFirstTalk` a l'offset `0x004d4600`.
    - Toad melomane (Jammin' Toad) : `rs::isTalkCollectBgmNpc` a l'offset `0x004dfb60`.
    - Peach voyageuse : `rs::isTalkWorldTravelingPeach` a l'offset `0x004dfb20`.
    Ces hooks renvoient `false` quand `mIsCutsceneRefreshEnabled` est active, restaurant leurs cinematiques et premiers dialogues complets.
- **Fichiers modifies** : `Practicemod1.3/source/src/program/pe/Hacks/PracticeHacks.cpp` recompile en `subsdk9` et deploye dans Atmosphere et Yuzu.
- **Statut** : RESOLU (03/10/2026)

### RESOLU — Door Refresh (Portes reinitialisees au reload)

- **Description** : Quand l'option "Doors Refresh" est activee (Yes) dans le menu ImGui, les portes ouvertes se referment et se reinitialisent correctement lors d'un rechargement de scene (reload).
- **Cause du bug** : Dans SMO 1.3.0, l'offset de `DoorAreaChange::switchCloseAgain` est `0x00230170` (il avait ete erronelement configure sur `0x002301c0`, qui correspond en fait a `DoorAreaChange::appear` et ne fermait pas la porte). De plus, l'etat persistant du StageSwitch du porteur (`door + 0x108`) doit etre reinitialise a OFF via `0x004dd180` (`offStageSwitch`).
- **Fichiers modifies** : `Practicemod1.3/source/src/program/pe/Hacks/PracticeHacks.cpp` recompile en `subsdk9` et deploye dans Atmosphere et Yuzu.
- **Statut** : RESOLU (03/10/2026)

### EN COURS — Save State / Load State

- **Description** : Quand on sauvegarde la position de Mario (Save State) puis qu'on la recharge (Load State), Mario ne revient pas au bon endroit OU le jeu softlock.
- **Note technique** : Le mod ne peut sauvegarder que les coordonnees/rotation/etat du joueur (pas l'etat complet du monde — limitation memoire Switch 4 Go). Les vrais savestates complets sont uniquement possibles via la fonctionnalite native de Yuzu.
- **Statut** : NON RESOLU

### INCONNU — Mario clip le sol

- **Description** : Mario passe a travers le sol a certains endroits sur certains types de sol. Lie potentiellement au stage `CapWorldHomeStageMap.szs`.
- **Premiere mention** : 30 septembre 2026
- **Statut** : INCONNU (les conversations a ce sujet n'ont pas abouti clairement)

---

## Informations techniques

### Fonctionnalites du mod (preconfigurees via PeepaUserConfig.bin)
- Gray Shine Refresh : Active
- Shine Refresh : Active
- Disable Autosave : Active
- No Checkpoint Touch : Active
- Always Enable Warps : Active
- Segment Timer : Active
- Input Display : Active
- Mofumofu Pattern : Disponible

### Raccourcis en jeu

| Action | Raccourci |
|--------|-----------|
| Ouvrir/Fermer menu | L + D-Pad Gauche |
| Save State | D-Pad Gauche (menu ferme) |
| Load State | D-Pad Droite (menu ferme) |
| Recharger la scene | D-Pad Haut ou ZL + ZR |
| Scene precedente | D-Pad Bas |
| Roue d'action rapide | Maintenir L3/R3 + direction stick |

### Compilation
Le mod est compile en C++ avec le toolchain Switch (devkitPro). Le binaire resultant est `subsdk9`.
Le fichier `main.npdm` definit les permissions du module.

---

## Notes pour l'IA

- **Ne PAS modifier les fichiers binaires** (.szs, .bin, subsdk9) directement — ils necessitent des outils specialises.
- Le code source C++ dans `Practicemod1.3/source/` est la reference pour comprendre la logique du mod.
- L'utilisateur utilise Yuzu sur PC pour tester le mod.
- L'utilisateur parle **francais**.
- Quand un probleme est resolu, **mettre a jour ce fichier** en changeant le statut.
- **Seeds Refresh / Seeds Growing** (séparés) : Seeds Refresh = `getGrowFlowerTime` -> 0 et `isUsedGrowFlowerSeed` -> false (graine réapparaît, pot vide). Le niveau de pousse sauvegardé passe au max dès que la lune du pot est prise (`GrowFlowerPot::initAfterPlacement`), d'où la pousse instantanée d'avant : avec Seeds Refresh, `rs::getGrowFlowerGrowLevel` / `rs::addGrowFlowerGrowLevel` (appelés uniquement par GrowFlowerPot, actor = le pot) utilisent une table en mémoire par pot (64 max, vidée au `StageScene::init`), sans toucher à la sauvegarde. Seeds Growing (`mIsEnableSeedGrowing`, ajouté en fin de Settings) : `getGrowFlowerGrowLevel` -> 32, le pot plafonne à son max+2 donc la graine plantée pousse tout de suite.
- **Noclip** : déjà présent (`noclipHook` sur `PlayerActorHakoniwa::movement`, `Menu::mIsEnableNoclip`, non sauvegardé) mais seulement dans l'ancien menu ImGui ; ajouté dans Options (colonne de droite). Commandes : stick gauche = avancer, ZR = monter, ZL = descendre, X/Y = plus vite, A/B = moins vite. Mario passe en demo puppetable (pas de collision ni de gravité) ; `endDemoPuppetable` quand on désactive.
- **Sauvegarde des réglages** : avant, `ThreadSave` n'écrivait que toutes les 60 s et aucun `sd:/BTT-Studio/Settings.bin` n'existait dans `%APPDATA%/yuzu/sdmc`. Maintenant : vérification chaque seconde, écriture dès qu'un réglage diffère de la copie `mLastSavedSettings` (memcmp). `writeFileToPath` renvoyait `ConstructResult(1)` (échec) même en cas de réussite → renvoie 0 ; handle fermé si WriteFile échoue. Si `CreateDirectory` échoue, on refait `MountSdCardForDebug("sd")` et on réessaie. Logs yuzu : "Settings saved/loaded" ou "Settings: cannot ..." avec le code d'erreur (5 max).
- **Ouverture du menu** : les sticks (LStick/RStick, valeurs 0 et 1 de l'ancien `MenuKeys`) ne peuvent plus ouvrir le menu. `MenuKeys` = L+DLeft, L+DRight, R+DLeft, R+DRight ; L+DLeft marche toujours ; une valeur >= 4 est remise à 0 à l'init du menu.
- **Timer Odyssey Power Up** : déclenché seulement à l'écran noir de `exeGoToWorldMapWithFade` après le message. Le déclencheur `exeDemoUpLevelWaitFade` (retiré) passait aussi à chaque recharge, avant le message (tryLevelUp -> UpLevelCloseFade -> UpLevelWaitFade -> ... -> InformPowerUpMessage -> GoToWorldMapWithFade) : il arrêtait le timer, puis le second événement le relançait aussitôt.
- **Crash Disable TP Puppet** (load position pendant une prise de lune) : `helpers::isGetShineState` libérait (`free`) les chaînes de `__cxa_demangle` décalées de leur préfixe ("StageSceneState", "(anonymous namespace)::PlayerActorHakoniwaNrv") → corruption du tas. Maintenant on compare les noms complets et on libère les pointeurs d'origine.
- **Vibrations décalées / ralentissement ImGui** : `ImGuiBackendNvnImpl::draw` faisait `mBuffersSync.Wait(UINT64_MAX)` à chaque image dessinée (un seul tampon vertex/index) → le CPU attendait la fin du GPU de l'image précédente, ce qui décale les images. Maintenant 3 tampons + 3 Sync (`cVtxIdxBuffering`, déjà déclaré mais inutilisé) en rotation ; on n'attend que le tampon d'il y a 3 images. `cImGuiBackendNvnImplSize` passe de 2232 à 3648 (static_assert). ATTENTION : modif dans le sous-module `sys` (hakkun), non incluse dans l'instantané GitHub (gitlink).
- **Sub areas dans Stages** : sous Kingdom/Scenario/Warp to Stage, une ligne par royaume ("Cap Sub Area" ... "Darker Side Sub Area", 156 zones). Liste générée depuis `SystemData/WorldList.szs` (WorldListFromDb, catégories hors MainStage/Demo/Zone) → `include/menu/SubAreaList.h` (outils dans `tools/subareas/`). `SubAreaMenuComponent` (Menu.cpp) : A = choisir, Gauche/Droite = zone, A = téléporter (`warpToStage`, entrée "start", scénario du menu), B = annuler. Nouveaux virtuels `MenuComponent::onConfirm` / `drawPreview`.
- **Images des sub areas** : 156 captures du Super Mario Wiki (image d'une lune située dans la zone, tirée des pages "List of Power Moons in the ..." ; artwork du boss pour les tableaux de Peach), noms des lunes par zone depuis `LocalizedData/USen/MessageData/StageMessage.szs`. Redimensionnées 384x216 (letterbox), ASTC 8x8 linéaire (`astcenc -cl`) dans `romfs/ImGuiData/SubAreas/<stage>.astc` (~20 Ko chacune). `src/menu/SubAreaImages.cpp` : chargement à la demande, 8 emplacements en cache, jamais libérés s'ils ont servi dans les 6 dernières images (GPU).
