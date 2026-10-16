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

### PUBLIÉ — zips sans guide ni PeepaUserConfig (parent f6558de)
- Les 6 zips v1.9.4 n'ont plus GUIDE_ET_RACCOURCIS_*.txt ni PeepaUserConfig.bin (demande utilisateur). Les prochains mkzipsNNN.py ne doivent plus chercher / modifier le guide. Site : menu interactif (docs/menu-data.js + menu.js, section #options), parties Features et Comparison retirées.

### PUBLIÉ — site en anglais (parent 8ac67c2)
- docs/index.html entièrement traduit en anglais (lang="en"), bouton Copy -> "Copied!". L'avertissement ne se ferme plus tout seul : bouton "I understand" (#disclaimer-close, app.js). ATTENTION : les prochains edit_siteNNN.py doivent chercher les textes ANGLAIS ("New in v…", "Install the Mod v…", "Version x • y MB", "Download (…)").

### PUBLIÉ — v1.9.4 sur le compte GitHub mezfndu045-star (parent 9d3dac3)
- v1.9.4 : Options > Text Refresh (mIsEnableRefreshWarpText, remis au menu ; la migration vers Cutscene Refresh est retirée) : textes de Cappy (GameDataFunction::isFlagOnTalkMessageInfo, liste de la scène) et explication des drapeaux reviennent ; désactivé = jamais affichés (déjà dits / explication désactivée). Cutscene Refresh ne les gère plus. Broodals : StateGateKeeperBattleStartDemo::isFirstDemo (+ thunk Thn48 ; 1.3 0xb7a30 / 0xb7a50) faux avec Moon / Gray Moon / Cutscene Refresh (Restart Scene remettait "cinématique jamais vue" = skip impossible). Timer Odyssey Power Up après "Odyssée réparée" (ShineTowerRocket::exeDemoInformRepairHome -> décollage direct ; custom.sym OdysseyInformRepairHome 1.0 0x310628 / 1.3 0x2d34f0 = execute du nerf, inline en 1.3) : au premier écran noir, sinon au StageScene::init suivant. Logo d'origine (TitleLogo*.szs retirés du romfs et des zips). Site : avertissement 5 s à l'ouverture (#disclaimer, style.css, app.js). Scripts `mkzips194.py` / `edit_site194.py`.

### PUBLIÉ — v1.9.3 sur le compte GitHub mezfndu045-star (parent 30734ab)
- v1.9.3 : les modèles 3D de HOS-InputDisplay (GPL-3.0, publiés en v1.9.2) sont remplacés par des manettes faites pour le mod : `tools/subareas/gen_motion_models.py` (contours arrondis extrudés avec biseau, cylindres, boîtes arrondies ; aperçu PNG) -> include/menu/MotionModels.h (même format). glb2model.py supprimé. Rendu en 2 passes (MotionDisplay.cpp drawMesh) : corps / rails / gâchettes triés par profondeur, puis les marques de la face avant (MotionPartKind Face / Button / Stick) par-dessus, seulement si la face avant est vue (sinon une grande face du corps triée par son milieu les cachait). Scripts `mkzips193.py` / `edit_site193.py`.

### PUBLIÉ — v1.9.2 sur le compte GitHub mezfndu045-star (parent f8acb1d)
- v1.9.2 : logo de l'écran titre "Super Lazyr Odyssey" + "by lazyr11" (romfs/LayoutData/TitleLogoParts.szs et TitleLogoSmallParts.szs, textures CmTitleLogo^w 760x470 / CmTitleLogoSmall^w 240x150 ASTC 4x4 sRGB remplacées EN PLACE dans l'archive du jeu, Yaz0 alignement 0x2000 ; reconstruire la SARC = softlock), style d'Input Display "Motion" (dernier de sInputDisplayStyleNames, cMotionInputDisplayStyle ; src/menu/MotionDisplay.cpp : capteurs nn::hid GetSixAxisSensorHandles / StartSixAxisSensor / GetSixAxisSensorState (syms/nn/hid.sym, @sdk), orientation relative à une référence ; disposition à plat comme HOS-InputDisplay ; modèles 3D de HOS-InputDisplay (GPL-3.0) simplifiés par tools/subareas/glb2model.py (pyfqmr) -> include/menu/MotionModels.h, triangles triés par profondeur). Réglages ajoutés en fin de Settings : mIsEnableMotionDisplay (inutilisé), mResetMotionKey (Custom L + R par défaut via mIsMotionKeyInit, CustomKeySlot ResetMotion), mMotionController (Auto / Pro / Joy-Con). Scripts `mkzips192.py` / `edit_site192.py`.

### PUBLIÉ — v1.9.1 sur le compte GitHub mezfndu045-star (parent 2c7dd6e)
- v1.9.1 : type de fin par split (Settings mSplitTypes[30] : 0 = End Type du timer, sinon 1 + index dans Menu::VisibleTimerHookTypes ; Timer::event lit celui du split en cours ; menu Edit Split + Split Type ; Timer est friend de Menu), WipeSkip (cinématique passée) compté comme écran noir. Scripts `mkzips191.py` / `edit_site191.py`.

### PUBLIÉ — v1.9.0 sur le compte GitHub mezfndu045-star (parent d6ff917)
- v1.9.0 : Timer > Splits / Split Count (Settings mIsEnableSplits, mSplitCount ; Timer::split() à chaque événement du type de fin quand Splits est activé, le dernier arrête le run ; drawSplits sous le timer : nom + temps du timer à la fin du split, sans écart ni record — demandé ainsi). Warp Text Refresh retiré du menu et fusionné dans Cutscene Refresh (mIsEnableRefreshWarpText reporté sur mIsEnableRefreshNpc au chargement du menu ; No Checkpoint Touch n'est plus contourné). Scripts `mkzips190.py` / `edit_site190.py`.

### PUBLIÉ — v1.8.3 sur le compte GitHub mezfndu045-star (parent 5e03119)
- v1.8.3 : positions des drapeaux de checkpoint gardées pendant la remise de la progression (ProgressSnapshot.cpp : GameDataFile +0x9e8 / 1.3 +0xa30 -> 20 mondes -> 16 entrées de 0x148, position à +0x134 ; relevées avant initializeData, remises si vides après tryReadByamlData). Sans ça, Restart Scene (Moon Refresh) dans une sous-zone perdait les drapeaux (All Checkpoints ignore un drapeau sans position). Scripts `mkzips183.py` / `edit_site183.py`.

### PUBLIÉ — v1.8.2 sur le compte GitHub mezfndu045-star (parent a717d60)
- v1.8.2 : hook GameDataHolder::changeNextStage : un changement de stage sans scénario (ChangeStageInfo +0x1cc <= 0 : portes, tuyaux, mort, restart) vers le royaume visité reçoit sVisitScenario. Sans ça, le placement des objets prenait le scénario de la sauvegarde (boules de feu = DamageBallGenerator de Salt Pile Isle, frères casseroles, à Luncheon) même si calcNextScenarioNo donnait celui de la visite. Scripts `mkzips182.py` / `edit_site182.py`.

### PUBLIÉ — v1.8.1 sur le compte GitHub mezfndu045-star (parent dd7a037)
- v1.8.1 : pas de cinématique de début de stage après une mort (TimerHooks wipeSetNerveHook : nerf StageSceneNrvMiss, custom.sym 1.0 0x1d8cbe8 / 1.3 0x12c5e18 -> btt::onStageMiss ; le rechargement suivant est traité comme un retour de sous-zone). Traces de scénario (setMainScenarioNo, "loaded ... visit") ajoutées, actives seulement avec trace. Boules de feu de Luncheon (Salt Pile Isle) : DamageBallGenerator du calque Common, allumé par 3 SwitchKeepOnArea, scénario resté 1 d'après la trace ; cause non trouvée, l'utilisateur a laissé tomber. Scripts `mkzips181.py` / `edit_site181.py`.

### PUBLIÉ — v1.8.0 sur le compte GitHub mezfndu045-star (parent a7f9d79)
- v1.8.0 : Keybinds > Custom (menu/CustomKeys.h/.cpp : masque des 16 boutons NpadButton 0..15 par raccourci dans Settings::mCustomKeys, KeyBindMenuComponent, enregistrement dans Menu::updateMenuInput ; isHotkey/isTpKey prennent le champ Settings par référence pour retrouver le raccourci ; Open Menu en Custom fait seulement de A/B/croix ne ferme pas le menu), Open Outfit Doors (getNextId de EventFlowNodeCheckCostume / Pair / MissMatchPart -> al::getCaseEventNextId(node, 0) ; 1.3 0x196c30 / 0x196e00 / 0x196f20 / 0x608cb0, trouvés par les vtables), roue sans A (L/R : relâcher ; L3/R3 : relâcher le stick), scénario de la visite d'un royaume gardé dans ses sous-zones (sVisitWorld/sVisitScenario, GameDataFile::setMainScenarioNo 1.3 0x4d12f0, scénario explicite +0xa28/+0xa24/+0xa0a ou 1.3 +0xa70/+0xa6c/+0xa52 respecté, remis au début de scène par Restart Scene). Scripts `mkzips180.py` / `edit_site180.py`.

### PUBLIÉ — v1.7.3 sur le compte GitHub mezfndu045-star (parent ffe2e94)
- v1.7.3 : timer Black/White Screen (TimerHooks.cpp : couleurs des WipeSimple gardées en LRU, seulement noires/blanches ; WipeResultGrand / WipeResultMain comptés noirs), attaque du dragon vers Ruined (sIsBossAttackOverride : GameDataFunction::isBossAttackedHome 1.3 0x4d8000 vrai pour un voyage vers BossRaidWorldHomeStage au scénario 1, jusqu'à repairHomeByCrashedBoss 1.3 0x4d8020 ou sortie de BossRaid*/Demo*). Zips depuis les v1.7.2, scripts `mkzips173.py` / `edit_site173.py`.

### PUBLIÉ — v1.7.2 sur le compte GitHub mezfndu045-star (parent c9ddd4b)
- v1.7.2 : skins regroupés en `romfs/ImGuiData/Skins/<id>.pack` (tools/subareas/pack_skins.py pack|unpack ; lus par btt::loadRomfsFile, SubAreaImages.cpp ; romfs 2586 -> 590 fichiers, installation SimpleModManager plus rapide), monde courant lu à GameDataFile +0x9f0 / 1.3 +0xa38 (currentWorldIdOf : l'en-tête OdysseyHeaders est faux en 1.3, la détection du retour de sous-zone ne marchait pas en 1.3). Canal de test : commandes opt input/input2p/style/size et img <chemin romfs> (dbg.ps1 capture maintenant avec PrintWindow). Zips depuis les v1.7.1, scripts `mkzips172.py` / `edit_site172.py`.

### PUBLIÉ — v1.7.1 sur le compte GitHub mezfndu045-star (parent 058e5b0)
- v1.7.1 : Input Display > Size (mInputDisplaySize, %, mise à l'échelle des sommets ImDrawList autour de la position, main.cpp), 103 styles OJD retirés (liste `tools/subareas/ojd_removed.txt`, migration de l'index du style via mInputDisplayStyleList), Slots (LavaBonus1Zone, entrée "town", images 384x216) dans les sous-zones de Luncheon, retour de sous-zone sans les refresh de début de stage (sIsSubAreaReturn : SceneInitInfo::initStageName, car le GameDataHolder a encore le stage précédent au début de StageScene::init ; royaume du stage précédent = world id lu après son init). Zips depuis les v1.7.0, scripts `mkzips171.py` / `edit_site171.py`.

### PUBLIÉ — v1.7.0 sur le compte GitHub mezfndu045-star (commit 058e5b0, parent ae14864)
- v1.7.0 : skins OJD, roue rapide, aides, langue, Moon Refresh (histoire, Broodals), refresh des arrivées, IL au scénario 1 (+ copie de progression par royaume), Cloud rejoué -> Lost, images Yaz0 + REF (romfs 20 -> 13 Mo). Zips `*-v1.7.0.zip` faits depuis les v1.6.0 (binaires, Skins/SubAreas du romfs, section 3h du guide), scripts `mkzips17.py` / `edit_site17.py` (scratchpad). Canal de test désactivé (cmake -DBTT_DEBUG_COMMANDS=ON pour le réactiver). Crédit OJD (BSD 4 clauses) dans README et pied du site. En ligne et vérifié le 14/10/2026.

### PUBLIÉ — v1.6.0 sur le compte GitHub mezfndu045-star (commit ae14864, parent bb7b9c4)
- v1.6.0 : téléportation depuis une nouvelle sauvegarde (préchargement de Cascade libéré + unlockWorld), Misc et Input Display sur 2 colonnes, réglages P2 de position. Zips `*-v1.6.0.zip` faits depuis les v1.5.0 (binaires + section 3g du guide), scripts `mkzips16.py` / `edit_site16.py` (scratchpad).

### PUBLIÉ — v1.5.0 sur le compte GitHub mezfndu045-star (commit bb7b9c4, parent 3f5ddf1)
- v1.5.0 : version pour SMO 1.3.0 (zips `BTT-Studio-1.3-*-v1.5.0.zip` : dossier `Practice mod BTT 1.3`, exefs de build130, romfs = ImGuiData seulement, guide `GUIDE_ET_RACCOURCIS_1.3.txt`), zips 1.0 v1.5.0 (472 images SubAreas + Skins, section 3f du guide), site + README (images royaumes / sous-zones, Rename All Moons, Style de l'Input Display, Skip Cloud). Scripts : `mkzips15.py`, `edit_site15.py` (scratchpad). `build130/` ajouté au .gitignore. Push à faire depuis PowerShell (gh n'est pas dans le PATH du bash devkitPro : le push y reste bloqué) : `git -c credential.helper= -c "credential.helper=!'C:/Program Files/GitHub CLI/gh.exe' auth git-credential" push <url> <sha>:refs/heads/main`. En ligne et vérifié le 14/10/2026.

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
- **Images des royaumes** : la ligne Stages > Kingdom (`KingdomMenuComponent`) affiche aussi une image (`drawStagePreview`, partagé avec les sub areas). 17 images = image principale de la page du royaume sur le Super Mario Wiki (`tools/subareas/fetch_kingdoms.py`) ; Pays des Champignons = capture du jeu "SMO - Mushroom Kingdom" (titre affiché) ; sous-zone Peach's Castle = "NM SMO Peachs Castle" (intérieur). Fichiers `romfs/ImGuiData/SubAreas/<Royaume>WorldHomeStage.astc` (173 images en tout). Pas encore publié sur GitHub.
- **Crash téléportation sous-zone d'un autre royaume** : aller directement dans une sous-zone d'un autre royaume plante (`sead::FrameHeap::tryAlloc` dans `al::ArchiveEntry::load` : ressources du royaume non chargées). `warpToStageSafe` (stage_warp.cpp) passe d'abord par la HomeStage du royaume, puis `updateStageWarp` (appelé chaque image) entre dans la sous-zone 30 images après le chargement. Utilisé par le menu (sous-zones) et la visite photo.
- **Visite photo** (`src/menu/PhotoTour.cpp`, liste `include/menu/PhotoTourList.h` générée par `tools/subareas/make_tour.py` + `gen_tour.py`) : démarrée par `sd:/BTT-Studio/tour_start.txt`, synchronisée avec le script `tour_capture.ps1` (Ctrl+P dans yuzu) via `tour_shot.txt` / `tour_ack.txt`. L'interface du mod est cachée pendant la visite.
- **Scénario lors d'un changement de royaume** : avec le scénario -1, le jeu n'ouvrait pas le royaume au scénario de la sauvegarde. `getSaveScenarioOf` (stage_warp.cpp : `tryFindWorldIdByMainStageName` + `getWorldScenarioNo`, ajoutés à GameDataFunction.sym) donne le scénario de la sauvegarde ; utilisé par `warpToStageSafe` (étape royaume) et `warpToKingdom` quand Scenario = 0.
- **Courses du Bound Bowl retirées** (SnowWorldRace000/001/Ex/Tutorial/HardEx) des sous-zones du menu et de la visite : une fois téléporté dans une course, le jeu y reste bloqué (changeNextStage ignoré) puis plante. 151 sous-zones.
- **Images entrée + intérieur des sous-zones** : 299 captures prises dans yuzu par la visite photo (dernier scénario de chaque royaume), recadrées (`crop_shots.py`) en 384x216, ASTC 8x8 → `romfs/ImGuiData/SubAreas/<stage>_in.astc` et `<stage>_out.astc`. `SubAreaMenuComponent::drawPreview` affiche Entrance (extérieur, tuyau/porte) au-dessus d'Inside (début de la zone) ; repli sur l'image wiki `<stage>.astc` s'il n'y a ni l'une ni l'autre. 43 captures refaites : après Secret Flower Field (ForestWorldBossStage) le jeu renvoyait les téléportations vers ce champ de fleurs ; correctif : pas de scénario forcé pour une sous-zone du royaume courant.
- **Styles d'input display** (`mInputDisplayStyle` u8 ajouté en fin de Settings, enum `InputDisplayStyle` dans `include/menu/InputDisplayStyles.h`, dessins vectoriels ImDrawList dans `src/menu/InputDisplayStyles.cpp`) : Classic (ancien code), Pro Controller, Joy-Con (Neon), Joy-Con (Gray), Joy-Con Grip, Switch Handheld, GameCube, N64, SNES, NES, Xbox, PlayStation, Compact. Couleur appuyée = Pressed Color ; boutons neutres = Button Color (Pro/Compact) ou couleurs d'origine ; sticks = Stick/Ring Color ; corps arc-en-ciel si BG Color = Multicolor. NES/SNES : le stick gauche allume la croix. N64 : boutons C = stick droit (ou X/Y). Même zone que Classic (centre = position - (125, 95)), déplaçable à la souris, 2P compris. Non testé en jeu.
- **Noms de lunes personnalisés** (`src/menu/MoonNames.cpp`) : trampolines sur `al::getStageMessageString` et `al::tryGetStageMessageString` (`syms/al/Library/Message/MessageUtil.sym`) : un label `ScenarioName_<objId>` d'une zone renvoie le nom choisi (UTF-16, 64 caractères max, 256 lunes max). Dernière lune prise = `GameDataFunction::setGotShine` (ajouté dans ShineRefreshHook : ShineInfo::getStageName/getObjId). Sauvegarde `sd:/BTT-Studio/MoonNames.txt` (zone TAB objId TAB nom UTF-8), chargée au démarrage. Misc : Rename Last Moon / Reset Last Moon Name / Reset All Moon Names.
- **Clavier à l'écran** (`src/menu/Keyboard.cpp`) : grille 10 colonnes (lettres, accents Latin-1, chiffres, ponctuation) + Space / Caps / Erase / OK / Cancel. Croix = déplacer, A = taper, B = effacer, Y = majuscules, + = valider, - = annuler. Prend toutes les entrées du menu tant qu'il est ouvert (`Menu::updateMenuInput`), dessiné à la fin de `Menu::draw`. Non testé en jeu.
- **Renommer toutes les lunes** : `include/menu/MoonList.h` (généré par `tools/subareas/gen_moonlist.py` : 775 lunes, ordre de ShineInfo puis tous les ScenarioName_* des zones du royaume). Misc : `Moon Kingdom` (17 royaumes), `MoonRenameMenuComponent` (Gauche/Droite = lune, A = clavier ; nom en vert si renommé), `Reset Moon Name`, `Reset All Moon Names` (Rename Last Moon retiré du menu). Écran « Lune obtenue » : `rs::trySetPaneStageMessageIfExist` intercepté (il vérifie le label et remplit le texte sans passer par getStageMessageString ; s'il échoue, le jeu affiche le texte « Moon Refresh Text », "BTT Studio" par défaut, écrit dans une chaîne vide de la rodata à 0x1832301).
- **Un seul nom pour toutes les lunes** (demande utilisateur, remplace le choix lune par lune dans le menu) : Misc > `Rename All Moons` (clavier) → `setAllMoonsName`, ligne `*\t*\tnom` dans MoonNames.txt. `findCustom` renvoie ce nom pour tout label ScenarioName_* (un nom par lune garde la priorité) ; le hook de `rs::trySetPaneStageMessageIfExist` l'affiche aussi quand le jeu ne trouve pas le label (au lieu du texte de secours « BTT Studio »). Valider un nom vide = noms d'origine. `MoonRenameMenuComponent` reste dans Menu.cpp mais n'est plus ajouté au menu.
- **« BTT Studio » sur l'écran Lune obtenue** : diagnostic (log MOONDBG, retiré) : `rs::trySetPaneStageMessageIfExist(layout, "TxtScenario", label="", stage="")`. Avec Moon Refresh, ShineRefreshHook n'appelle pas le vrai `setGotShine`, donc la lune obtenue n'est pas enregistrée et le label est vide → texte de secours. Correctif : label vide → `ScenarioName_<objId>` + zone de la dernière lune prise (`onMoonGot`), puis nom personnalisé ou vrai nom du jeu.
- **Skins photo d'input display** : moteur `src/menu/InputDisplaySkins.cpp` (skin.txt : `bg`, `btn`/`btnt` = image affichée quand une des touches "a|x" est tenue, `btnt` teintée avec Pressed Color, `stick L|R` + amplitude ; directions de stick lsu/lsd/lsl/lsr/rsu/...). Images via `getRomfsImage` (cache SubAreaImages passé à 40 emplacements, ASTC 4x4 ou 8x8 selon l'en-tête). Skins dans `romfs/ImGuiData/Skins/` générées par `tools/subareas/gen_skins.py` : Periscope (skin par défaut de periscope, CC BY-SA 4.0, fond rendu transparent) et RetroSpy (GPL-3.0) : Switch Pro, GameCube (violette), N64, NES, SNES, PS4, Xbox 360, Wii Classic (fond chroma-key retiré, max 640 px de large). Styles ajoutés en fin d'enum (`Periscope` … `WiiClassicPhoto`). Crédits : `romfs/ImGuiData/Skins/CREDITS.txt`.
- **Crash au changement de style (skins photo)** : `loadSlot` réservait la taille des données ASTC, mais la texture NVN (block linear) peut en demander plus (`TextureBuilder::GetStorageSize`) → débordement dans la mémoire (crash dans `nvn::Sampler::Initialize`). Maintenant la mémoire et le pool font max(données, GetStorageSize) (builder avec SetDevice avant la requête).
- **Noms des touches sur les skins** : ligne `label <texte> <x> <y> <taille>` dans skin.txt (ajoutée par `add_labels` de gen_skins.py au centre du premier bouton de l'entrée), dessinée en blanc avec ombre par-dessus tout (taille min 11 px). Periscope : A B X Y L R ZL ZR + - et flèches ; GameCube : A B X Y Z START ; N64 : A B S ; NES : A B. Les photos qui montrent déjà les lettres (Pro, Xbox 360, PS4, SNES, Wii Classic) n'en ont pas.
- **Manettes dessinées (remplacent les photos RetroSpy, refusées par l'utilisateur)** : `tools/subareas/gen_drawn_skins.py` dessine chaque manette (formes x4 + contour blanc calculé sur la silhouette, corps translucide, boutons appuyés blancs teintés en jeu, noms des touches en `label`) : ProController, JoyCons, GameCubeHD, N64HD, NESHD, SNESHD, PlayStationHD, XboxHD, et Minimal (reproduction de la capture d'écran envoyée : pastilles grises L/L2/R2/R, L3/R3, sticks en anneaux). Styles : Periscope, Pro Controller HD, GameCube HD, N64 HD, NES HD, SNES HD, PlayStation HD, Xbox HD, Joy-Con HD, Minimal. Aperçu : `preview_rest.py`.
- **Styles d'input display réduits (demande utilisateur)** : il ne reste que Classic (0), Periscope (1) et Minimal (2). Dessins vectoriels de `InputDisplayStyles.cpp` et skins HD/photo supprimés (code et romfs) ; `romfs/ImGuiData/Skins/` = Periscope + Minimal + CREDITS.txt. Une ancienne valeur de style hors liste repasse à Classic (dans `drawStyledInputDisplay` et à l'init du menu). Les entrées ci-dessus sur les autres styles sont obsolètes.

### A TESTER — Practice mod BTT 1.3 (étape 1 du portage vers SMO 1.3.0)
- Compilation séparée : `cmake -G Ninja -S . -B build130 -DBTT_V130=ON` puis ninja dans `build130` (définit `BTT_V130=1`). Dossier yuzu : `load/0100000000010000/Practice mod BTT 1.3` (exefs = build130 + romfs/ImGuiData ; PAS LayoutData/StageData de la 1.0).
- Désactivé en 1.3 (`#if !BTT_V130`) : Skip Broodals, Always Odyssey Power Up, Odyssey In All Kingdoms, Night Metro, Luigi Pixels, Seeds Growing, fragments (vérifs du watcher), noms de lunes / clavier, timers Press Start / Odyssey / Black-White Screen / Capture / Moon Get / 2 Moons Grab / Escape Wire, scénario de la sauvegarde au changement de royaume (-1). Options masquées dans le menu ; types de timer 1.3 = None, Damage, Shine Tick, 2 Moons (Tick), Shine Grab.
- `src/stubs130.cpp` : 18 fonctions du jeu sans adresse 1.3 définies localement (visibilité hidden) pour que le module charge (toute importation non résolue = abort au démarrage). Jamais appelées en 1.3.
- Outils : `port13.py` (scratchpad) cherche une fonction 1.0 dans `scratch/smo_130_main.bin` (1.3 sans symboles) par motif d'instructions ; fiable seulement pour les correspondances uniques et longues (validation : ~50 % des adresses 1.3 connues retrouvées). `check_hooks130.py` liste les recherches de symboles restantes dans la build 1.3.
### A TESTER — Practice mod BTT 1.3, étape 2 (groupe 1)
- Activés en 1.3 : Seeds Growing, Rename All Moons / Reset All Moon Names (hooks des noms de lunes), StageScene::init (remise à zéro des pots) et tous les types de timer (Press Start, Odyssey Power Up, Black/White Screen, Capture, Escape Wire, Moon Get, 2 Moons Grab).
- Adresses 1.3 ajoutées dans les blocs `@smo:130` : ShineTowerRocket.sym (exeDemoInformPowerUpMessage 0x2cfd30, exeGoToWorldMapWithFade 0x2d1ef0, PlayerHackKeeper::startHackStartDemo 0x3ebe00 / Puppetable 0x3ebe50, Shine::exeWaitRequestDemo 0x1b6320, getDirectWithDemo 0x1b6270), GameDataUtil.sym (get/addGrowFlowerGrowLevel 0x4dd2d0 / 0x4dd290), MessageUtil.sym (getStageMessageString 0x6a7a30, tryGet 0x6a7a80, trySetPaneStageMessageIfExist 0x51f6f0), Broodals.sym (StageScene::init 0x46e7a0), custom.sym (nerfs WipeSimpleNrvCloseEnd 0x130aeb0, SceneStateSelectModeNrvDecide 0x12c4a38, ElectricWireKoopaNrvInCharge 0x1237dd8).
- startHackStartDemo : trouvé via le bloc des fonctions `rs::*HackStartDemo` (0x511510.., même ordre qu'en 1.0) dont les `b` mènent à PlayerHackKeeper. Offset +0x268 (wipe) de ShineTowerRocket vérifié identique en 1.3. WipeSimple C2 n'existe pas en 1.3 (jamais appelé en 1.0) : seul C1 (0x70e1c0) est hooké.

### A TESTER — Practice mod BTT 1.3, étape 3 (Skip Broodals)
- Hooks BombTail/FireBlower/CapThrower/Breeda::control (0x94b0, 0x4ebb0, 0x485b0, 0x40580) + nerfs Start/End des 4 combats dans custom.sym @smo:130. Offsets PV vérifiés identiques (+0x108, CapThrower +0x158).
- En 1.3, BombTail::damageAndSetNerve et FireBlower::damageOrDie n'existent plus (inline) : réécrits dans SettingsHooks.cpp sous `#if BTT_V130` (PV à 0, killForceBeforeDemo 0x5dfe30 sur les pièces +0x168/+0x170, BombTailBombHolder::kill 0xfe40 sur +0x188, puis nerf BattleEndDemo). La 1.0 appelle toujours les fonctions du jeu. al::isNerve 0x6c7c40 ajouté (stub retiré).

### A TESTER — Practice mod BTT 1.3, étape 4 (options de l'Odyssée)
- Activés : Always Odyssey Power Up, Odyssey In All Kingdoms, scénario de la sauvegarde au changement de royaume (Stages).
- Adresses 1.3 (ShineTowerRocket.sym / custom.sym @smo:130) : decideWorldMap 0x2d2000, cancelWorldMap 0x2d1fa0, init 0x2c8a60, isFindKoopaNext 0x4dbf30 (City), isBossAttackedHomeNext 0x4dbfc0 (Sky), isExistHome 0x4d77c0, isUnlockedWorld 0x4d53d0 (cible des 3 sites UnlockCheck), getWorldScenarioNo 0x4d4ed0, OdysseyIsLevelUpByPay 0x2cdc20, OdysseyGetCurrentShineNum 0x2cc5e0, UnlockCheck1/2 0x2c96cc/0x2c9700, StageSceneInitUnlockCheck 0x471b1c, nerfs OdysseyNrvWait 0x1270328 / WaitLanding 0x1270338, StageSceneNrvPlay 0x12c5eb8.
- StageSceneStateWorldMap : tous les champs utilisés sont 8 octets plus bas en 1.3 (`cWorldMapOffsetShift`). GameDataFile : +0x20 (scénario principal) inchangé, GameProgressData passe de +0x6a8 à +0x6a0 (non utilisé par le mod). ShineTowerRocket +0x12c inchangé.
- En 1.3 les variantes LayoutActor de calcNextLockedWorld*ForWorldMap n'existent plus (hooks non installés). tryFindWorldIdByMainStageName est inline : en 1.3 l'index de monde = index de mainNames avec Snow/Seaside inversés (Sea 8, Snow 9).

### A TESTER — Practice mod BTT 1.3, étape 5 (dernières options : toutes les options de la 1.0 sont portées)
- Activés : Night Metro Multi Moon, Luigi Pixels, vérifications du watcher des fragments (Moon Shards Refresh), texte d'explication du checkpoint (GameDataFile 0x4cf7a0). `src/stubs130.cpp` n'a plus aucun stub.
- Night Metro : Mofumofu::init 0x98ac0 (+0x1b8 inchangé), Shine::endBossDemoAndStartFall 0x1b60a0. Shine::appearCommon est inline en 1.3 : réécrit sous `#if BTT_V130` avec les nerfs de `ShineNrvTable` (0x1204048 ; +0 et +0x30..+0x48, lien +0x2e0) et al::isAlive 0x65d590.
- Luigi Pixels : LuigiRareRoll 0x4f9e28 (seul appel de isPercentProbability 0x693bf0 en 1.3).
- Fragments : rs::isGotShine 0x4fa3f0, isCurrentShineChipWatcherTypeEmpty 0x1bab70, GotCheck1..8 = 0x1b98b8, 0x1b9d50, 0x1b9fac, 0x1b9774, 0x1b994c, 0x1b9b24, 0x1ba0d8 (updateHints 0x1ba0c0), 0x1ba28c + `ShineChipWatcherGotCheck3b` 0x1ba004 (1.3 duplique la vérif de fin d'initAfterPlacement, hooké seulement en 1.3).
- Seuls restent spécifiques 1.0 : hooks LayoutActor de calcNextLockedWorld* et WipeSimple C2 (absents de la 1.3).
- Réglages partagés avec la 1.0 (même sd:/BTT-Studio/Settings.bin) : d'où les gardes explicites (ex. Night Metro dans StageScene::control).

### A TESTER — Skip Cloud : combat de Bowser (Nuages) au dernier coup (1.0 et 1.3)
- Avant : le hook de `StageScene::isDefeatKoopaLv1` sautait le combat et l'Odyssée repartait vers Lost. Supprimé.
- Maintenant (demande : Bowser vaincu directement) : hook de `KoopaLv1::movement` (1.3 : 0x8b5c0, identique). Dans un stage "Cloud*", quand Bowser sort de son premier nerf `KoopaLv1NrvDemoStart` (cinématique de début) avec encore 3 coups (KoopaNumberCounter +0x120, +4) : coups = 0 et nerf `KoopaLv1NrvDeadAndDemoEnd` (celui du dernier coup de poing). custom.sym : DemoStart 0x1c71250 / 1.3 0x11bb0d8, DeadAndDemoEnd 0x1c71298 / 1.3 0x11bb120. Essais précédents : hook de init + test du niveau 0 (sans effet) ; PunchCounter forcé en phase 2 (plus de coups possibles) ; PV à 1 seulement (marchait, mais l'utilisateur voulait la victoire directe).

### A TESTER — Téléportation depuis une nouvelle sauvegarde (Cap / Cascade), 1.0 et 1.3
- Crash vu dans le journal yuzu : `sead::FrameHeap::tryAlloc` pendant `al::ArchiveEntry::load` en arrivant dans un autre royaume.
- Cause probable : le royaume visé n'est pas encore atteint par la sauvegarde, `getWorldScenarioNo` <= 0, `getSaveScenarioOf` renvoyait -1 et le jeu choisissait un scénario invalide.
- Correctif : `getSaveScenarioOf` (stage_warp.cpp) renvoie 1 pour un royaume sans scénario (utilisé par Warp to Stage, les sous-zones d'un autre royaume et warpToKingdom quand Scenario = 0).
- Ça ne suffisait pas (toujours bloqué). Vraie cause : dans le prologue (Cap scénario 1, puis Cascade jusqu'au départ de l'Odyssée), `WorldResourceLoader::requestLoadWorldHomeStageResource` (1.0 0x514540 / 1.3 0x4bddb0) crée un tas « Cap » (+0x18, 0x1f400000) et un tas résident (+0x20) où Cascade est préchargée. Tant que +0x20 existe, la fonction ne fait rien : un changement de royaume normal ne charge jamais le nouveau royaume. Dans le jeu, seul le voyage en Odyssée (demo world warp) appelle `tryDestroyWorldResource` (1.0 0x5143fc / 1.3 0x4bdc60).
- Correctif : hook de requestLoadWorldHomeStageResource (stage_warp.cpp, `installStageWarpHooks` appelé dans main.cpp). Si le changement de royaume vient du mod (`sIsModWorldChange`, mis par changeStage / warpToKingdom quand le royaume change), que +0x20 existe, que le monde demandé n'est pas celui chargé (+0x28) et que le thread de chargement (+8, octet +0x18) a fini : tryDestroyWorldResource avant l'original. Le drapeau est effacé quand la demande passe ou quand une scène finit de charger.
- Suite : après une téléportation vers un autre royaume depuis une sauvegarde neuve, payer une lune à l'Odyssée crashait (le royaume n'est pas débloqué dans la sauvegarde : la carte du monde ne le trouve pas). `markModWorldChange` appelle maintenant `GameDataFunction::unlockWorld` (1.0 0x52ae50 / 1.3 0x4d6170, ajouté à GameDataFunction.sym) si `isUnlockedWorld` est faux pour le royaume visé. unlockWorld -> GameProgressData::unlockNextWorld débloque tous les royaumes de l'histoire jusqu'à lui et règle le niveau de l'Odyssée (+0x38). L'état de l'Odyssée (lancée = 2) est déjà mis par GameDataFile::startStage dès Sand. Modifie la progression de la sauvegarde (comme un vrai voyage).

### A TESTER — Misc sur 2 colonnes (1.0 et 1.3)
- `mCategories[2].columnSplit` avant "Reload Scene" : gauche Outfit Body → Remove 1000 Coins (14), droite Reload Scene, Previous Scene, Wiggler Pattern, Rename All Moons, Reset All Moon Names.
- Input Display aussi sur 2 colonnes : gauche Input Display, 2P Mode, Style, Preset Pos, Position X/Y, Reset Position ; droite P2 Preset Pos, P2 Position X/Y, P2 Reset Position (mInputDisplayPosP2, déjà sauvegardé) puis les 5 couleurs.

### A TESTER — 212 styles d'Input Display d'Open Joystick Display (1.0 et 1.3)
- Source : `Downloads/open-joystick-display-2.8.0-x64-windows/*.exe` (installeur Electron, pas lancé) : archive 7z intégrée (offset 232629) -> `resources/app.asar` -> `app/themes/` (17 thèmes, 215 styles, BSD 4-Clause Attribution, Anthony Mattera / KernelZechs). Extraits dans le scratchpad (`ojd_app`, lecteur asar `asar.py`).
- `tools/subareas/gen_ojd_skins.py <app> <romfs Skins> <tmp> <astcenc>` : chaque style est affiché dans Edge sans fenêtre (Playwright, `channel='msedge'`, page écrite dans un fichier : depuis about:blank les CSS file:// ne se chargent pas). Il génère :
  - le fond sans les sticks (ASTC 6x6, max 360x300) ;
  - une image par bouton appuyé (classe `active` sur `[ojd-button=…]`, zone qui change, ASTC 4x4) ;
  - les sticks seuls (`[ojd-directional]`, amplitude = moitié du wrapper).
  - Sortie : `romfs/ImGuiData/Skins/OJD000…OJD211` + `OJD-LICENSE.txt`, et `include/menu/OjdSkins.inc` (macro `BTT_OJD_SKIN(id, nom)`, incluse deux fois dans InputDisplayStyles.h).
- Correspondance des boutons :
  - Xbox / PS / Scuf : par position (bas = B, droite = A…).
  - Nintendo : par nom.
  - Sega : A B C = Y B A, X Y Z = X L R ; Master System 1/2 = B/A.
  - N64 : Z = ZL|ZR. GameCube : Z = R.
- 3 styles sans rendu ignorés. Total des skins : 18 Mo. SubAreaImages accepte maintenant l'ASTC 6x6.
- Aperçu d'un skin : `tools/subareas/preview_ojd.py`.

### A TESTER — Roue rapide (Quick Wheel), 1.0 et 1.3
- `src/menu/QuickWheel.cpp` / `include/menu/QuickWheel.h`.
- Ouverture : clic de stick selon `Settings::mWheelKey` (0 Off, 1 L3, 2 R3 par défaut, 3 L3 ou R3).
- 8 emplacements : un stick vise, A lance, B ou la touche de la roue ferme. Les touches du jeu sont bloquées pendant la roue (`disableButtons`) et les raccourcis aussi.
- Actions possibles : toute option on/off (BoolMenuComponent) et tout bouton (ButtonMenuComponent) des catégories Options, Misc et Timer, plus "Warp Last CP" (`Menu::warpLastCheckpoint`, aussi utilisé par le raccourci).
  - `buildWheelActions` est appelé dans `initCategories` avant Keybinds.
  - `MenuComponent::getWheelName` / `getWheelToggle`.
- Réglages ajoutés à la fin de Settings : `mWheelKey`, `char mWheelSlots[8][32]`. Chaque emplacement est enregistré par nom d'action.
- Menu Keybinds sur 2 colonnes : à droite Wheel Key et Wheel Slot 1 à 8.
- Dessin : `prepareQuickWheel` + `drawQuickWheel` dans la fonction de dessin du menu (main.cpp). Popup « Nom: ON/OFF » après l'action.
- v2 de la roue :
  - **Taille** : `mWheelSize` (6/8/12), `mWheelSlots` passe à [12][32] (toujours en fin de Settings).
  - **Touches** : Wheel Key ajoute "L (hold)" / "R (hold)" : maintenir pour ouvrir, relâcher pour lancer. ZL/ZR, la croix ou l'autre gâchette pendant le maintien annulent la roue (combinaisons de raccourcis) ; les raccourcis restent actifs en mode maintien. L3/R3 : clic pour ouvrir, nouveau clic pour lancer.
  - **Mario peut bouger** : seul le stick droit est retiré au jeu pendant la roue (`disableButtons`) et sert à viser.
  - **A/B** ne servent plus à la roue.
- v3 : A valide la case visée dans tous les modes ; pendant la roue, seuls le stick droit et A sont retirés au jeu (disableButtons : `mButtons.Reset(A)`).

### A TESTER — Explications du Timer (1.0 et 1.3)
- Menu.cpp : `TimerTypeMenuComponent` (Start Type / End Type) et `TimerCountMenuComponent` (Start Count / End Count) affichent un panneau à droite (`drawPreview`). Le panneau contient un dessin ImGui de l'événement (`drawTimerTypeIcon` : écran noir/blanc, lune, cœur, chapeau, fil…), une phrase « Starts (from zero) / Stops when … » (`cTimerTypeHelp`) et la note quand Start et End sont du même type.
- Pour les counts : explication avec le type et le nombre actuels (ex. « the timer starts at the 3rd "Black Screen" »).
- Dessins retirés à la demande de l'utilisateur : le panneau ne montre plus que le titre et l'explication.
- Panneau d'aide généralisé : `MenuComponent::setHelp` + `drawPreview` par défaut (`drawMenuHelpPanel`, titre = `getHelpTitle`). `drawTwoColumnCategory` affiche maintenant aussi l'aperçu de l'élément sélectionné, à droite des 2 colonnes (x limité à 1600 - largeur). Les 29 options de la catégorie Options ont une explication (table `sOptionHelp` dans initCategories, appliquée par nom).

### A TESTER — Menu en français (1.0 et 1.3)
- `include/menu/Lang.h` / `src/menu/Lang.cpp` : `tr(en)` renvoie le texte français (table `cFrench`, ~330 entrées) quand `Settings::mLanguage == 1` (ajouté en fin de Settings). Cache par pointeur : à n'utiliser qu'avec des littéraux ou des tables statiques, jamais avec un tampon. `trColon()` = " : " en français.
- Branché dans :
  - les composants (titre, valeur, oui/non, panneau d'aide) ;
  - les noms des catégories, les popups (`setPopupText` traduit le format) ;
  - la roue, le clavier, la fenêtre d'infos, les étiquettes Joueur 1/2, les aperçus de Niveaux.
  - Aide du Timer : textes français dédiés (`whenFr`, phrases du nombre de départ / d'arrivée).
- Réglage : Paramètres > Language / Langue (English / Français).
- Non traduits : noms des sous-zones (noms propres anglais du wiki), costumes (noms internes), styles d'Input Display.
- Les actions de la roue restent enregistrées par nom anglais.

### A TESTER — Moon Refresh compte les lunes et fait avancer l'histoire (1.0 et 1.3)
- Avant, `ShineRefreshHook` (GameDataFunction::setGotShine) sautait l'original avec Moon Refresh. Résultat : `getCurrentShineNum` ne bougeait pas (ShineCounter::startCountAnim compte de getCurrentShineNum-1 à getCurrentShineNum) et les lunes d'histoire ne changeaient pas le scénario.
- Maintenant l'original est toujours appelé. Moon Refresh = la lune reste reprenable comme neuve : `GreyShineRefreshHook` (GameDataFunction::isGotShine(ShineInfo)) renvoie faux avec Moon Refresh OU Gray Moon Refresh.
- Limite : le jeu ne recompte pas une lune déjà enregistrée (GameDataFile::setGotShine teste lui-même isGotShine), donc la reprendre une 2e fois ne fait pas monter le compteur. La progression en mémoire change (Disable Autosave garde le fichier intact).
- Suite : Restart Scene remet la progression (lunes, histoire, pièces...) comme au début de la scène, avec Moon Refresh. `src/helpers/ProgressSnapshot.cpp` :
  - **Photo** : au `StageScene::init` (après l'original), `GameDataFile::writeToStream` (code de sauvegarde du jeu, format [s32 taille][byml], ~100-130 Ko) dans un tampon de 512 Ko via `sead::RamWriteStream` (mode 0), tas = GameDataHolder+0x90 (comme le jeu).
  - **Restauration** : `Menu::restartScene` : copie du stage (`GameDataFunction::getCurrentStageName`) et de l'entrée (`GameDataHolder::getNextPlayerStartId`), puis `GameDataFile::initializeData` (ne réalloue rien) + `tryReadByamlData`, puis `changeNextStage(stage, entrée, scénario -1)` avant le kill / Reload at Pos habituel.
  - Adresses 1.3 (GameDataFunction.sym) : RamWriteStream C2 0x960060 / D1 0x9600d0, writeToStream 0x4cc300, initializeData 0x4c2ef0, tryReadByamlData 0x4c3bf0, getNextPlayerStartId 0x4db1b0 (trouvés via readFromStream 0x4cc290 et l'appel de writeToStream à 0x4dbbf0).

### A TESTER — Animation d'entrée des sous-zones après une téléportation (1.0 et 1.3)
- Le menu envoyait dans les sous-zones par l'entrée « start » : pas d'animation d'arrivée (Mario qui sort en marchant), ni après un rechargement (le jeu reprend la même entrée).
- `SubAreaMenuComponent::onConfirm` utilise maintenant l'entrée de la sous-zone de `sPhotoTourShots` (`<stage>_in`, entrée vérifiée par la visite photo), avec repli sur « start », via `warpToStageSafe`.
- Les sous-zones ignorent le réglage Scenario du menu (`warpToStageSafe(stage, label, 0)`) : scénario courant dans le même royaume, scénario de la sauvegarde (ou 1) depuis un autre royaume.

### A TESTER — Kingdom Enter Refresh avec la téléportation du menu (1.0 et 1.3)
- Le hook `GameProgressData::isAlreadyGoWorld` ne sert qu'au Royaume Champignon dans `startStage` / `changeNextStage`. La cinématique d'arrivée (atterrissage de l'Odyssée + présentation du royaume) n'existe qu'avec le voyage de l'Odyssée : `GameDataFunction::tryChangeNextStageWithDemoWorldWarp` (fin de `ShineTowerRocket::exeDemoWorldTakeoff`).
- `warpToKingdom` (Stages > Warp to Stage) utilise maintenant ce voyage quand Kingdom Enter Refresh ou Cutscene Refresh est actif, que Scenario = 0 et que le royaume change. Adresse 1.3 0x4d2d30 (trouvée dans exeDemoWorldTakeoff 1.3 par la même suite d'appels : launchHome 0x4d7fb0, getMainStageName 0x4d5510, isForwardWorldWarpDemo 0x4d32f0).
- Avec un scénario choisi : le voyage en Odyssée est quand même utilisé, puis le scénario de la ChangeStageInfo préparée est remplacé (GameDataFile +0xa30 / 1.3 +0xa78, trouvé dans GameDataFile::changeNextStage 1.3 0x4c8560 avant ChangeStageInfo::copy 0x21c240 ; scénario +0x1cc).
- Correctif : Warp to Stage ne faisait plus rien avec Kingdom Enter Refresh. `GameDataHolder::changeNextStageWithDemoWorldWarp` remet le drapeau de changement de scène (+0x49) à 0 : la scène ne se termine pas toute seule (dans le jeu, c est la cinématique de décollage). `warpToKingdom` appelle maintenant `StageScene::kill` après avoir préparé le voyage.
- Le voyage en Odyssée de Warp to Stage ne dépend plus que de Kingdom Enter Refresh (plus de Cutscene Refresh) : option désactivée = téléportation directe sans cinématique.

### A TESTER — Warp Text Refresh : textes de Cappy du début des royaumes (1.0 et 1.3)
- Les messages de Cappy (début des royaumes, etc., `CapMessageKeeper`) ne sont dits qu'une fois : `GameDataFunction::isFlagOnTalkMessageInfo` (1.0 0x52cb04 / 1.3 0x4d81b0) lit le drapeau de GameDataFile, que `onFlagTalkMessageInfo` (1.0 0x52cb44 / 1.3 0x4d81f0) met.
- Avec Warp Text Refresh ou Cutscene Refresh, le hook ne renvoie vrai que pour les messages dits depuis le début de la scène (liste `sSceneTalkFlags`, vidée au `StageScene::init`). Les textes reviennent donc à chaque rechargement sans être répétés dans la même scène.
- Caméra de début de scénario (survol + texte de l histoire, ex. Sables, Ville de nuit) : `GameDataHolder::isPlayAlreadyScenarioStartCamera` (1.0 0x5306bc / 1.3 0x4dc060, tableau holder+0xd8 / 1.3 +0xe8) renvoie faux avec Warp Text Refresh, Cutscene Refresh ou Kingdom Enter Refresh.
- Cinématique d histoire à l arrivée (ex. Ville de nuit) = `StageTalkDemoNpcCap` placé dans le stage. Son init (1.0 0xd7954 / 1.3 0xc9730) l éteint (+0x128 = 0) si son SaveObjInfo (+0x140, `rs::isOnSaveObjInfo` 1.3 0x4dd190) dit qu elle a été vue. Hook après l original : démo de début de stage (config +0x120, octet +8) déjà vue -> +0x128 = 1, avec Cutscene Refresh ou Kingdom Enter Refresh. StageScene::init la choisit ensuite (placement "StageTalkDemoNpcCap", vtable+0xf0 = isEnableStageStartDemo).

### RESOLU — Cinématiques d'arrivée (Ville de nuit), Mécha-Wiggler, crash Moon Refresh (1.0 ; 1.3 compilé, non testé)
- Crash Moon Refresh au chargement d'un niveau : `takeProgressSnapshot` écrivait avec le FrameHeap de GameDataHolder+0x90 sans le vider. Le jeu le vide avant chaque sauvegarde (`writeToSaveDataBuffer`, vtable +0x58 = `FrameHeap::freeAll`) : le mod le fait aussi.
- Dialogue de Cappy à l'arrivée (StageTalkDemoNpcCap, démo de début de stage) : déjà vu -> son init saute la caméra d'event et tue l'acteur ; `tryStartDemo` (1.0 0xd7fdc / 1.3 0xc9f70) relit la sauvegarde. `rs::isOnSaveObjInfo` renvoie "pas vu" pendant ces deux fonctions (Cutscene Refresh / Kingdom Enter Refresh).
- Arrivée par l'Odyssey : après DemoAppearFromHome, le dialogue ne vient qu'après une caméra de scénario. Hook `StageScene::endDemoAndChangeNerve` (1.3 0x473ee0) : depuis AppearFromHome, si `isExistTalkDemoStageStart`, lance `startTalkDemoStageStart` + nerf DemoStageStartCapManHeroTalk (custom.sym).
- Caméra de scénario (Mécha-Wiggler) : liée à la quête 0 (`GameDataFile::isPlayScenarioCamera`, 1.3 0x4cfef0) ; quête finie -> aucune quête active. Hook : vrai pour la quête 0 quand aucune quête n'est active (options refresh).

### RESOLU — Rejouer le combat de Cloud : écrasement et arrivée à Lost scénario 1 (1.0 ; 1.3 compilé, non testé)
- `HakoniwaSequence::exePlayStage` ne fait `crashHome` + voyage vers Lost que si `isFindKoopa` (GameProgressData+0x34 == 3) et `isDefeatKoopaLv1` (StageScene+0x390, rempli dans StageScene::init seulement si isFindKoopa).
- Hook `GameDataFunction::isFindKoopa` (1.3 0x4d8050) : dans un stage Cloud, vrai pendant StageScene::init et quand Bowser est vaincu (scène prise même terminée : `getCurrentStageSceneEvenEnded`).
- La victoire met `sIsCrashHomeOverride` : `isCrashHome` vrai / `isRepairHome` faux (1.3 0x4d7fc0 / 0x4d7fe0) pendant les stages Demo* (DemoCrashHomeFallStage) et Clash*, jusqu'à `repairHome` (1.3 0x4d7ff0). La sauvegarde garde sa progression. Le dialogue de Cappy est rejoué sans option.
- Scénario 1 : `GameDataFunction::calcNextScenarioNo` (1.3 0x4d3040) renvoie 1 pour ClashWorldHomeStage pendant cet état (prochain stage : GameDataFile+0x870 / 1.3 +0x8b8).
- Attention : `dbgLog` écrit sur la SD ; ne jamais l'appeler dans une fonction appelée en boucle depuis le fil de chargement (abort nn::fs).
- Canal de test : commande `cloudwin` (DebugCommands) = Bowser K.O. au prochain `KoopaLv1::movement`. Traces de test à retirer avant publication.
