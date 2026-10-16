#include "menu/Lang.h"

#include <cstdint>
#include <cstring>

#include "src/settings/SettingsMgr.h"

namespace btt {

namespace {

    struct Entry {
        const char* en;
        const char* fr;
    };

    // English menu text -> French. Only Latin-1 characters (the menu font range): no "oe" ligature, no ellipsis.
    constexpr Entry cFrench[] = {
        // Categories
        { "Options", "Options" },
        { "Stages", "Niveaux" },
        { "Misc", "Divers" },
        { "Timer", "Chrono" },
        { "Input Display", "Affichage manette" },
        { "Keybinds", "Raccourcis" },
        { "Info", "Infos" },
        { "Settings", "Paramètres" },

        // Options
        { "Moon Refresh", "Lunes à volonté" },
        { "Gray Moon Refresh", "Lunes grises effacées" },
        { "Purple Coins Refresh", "Pièces violettes de retour" },
        { "Doors Refresh", "Portes refermées" },
        { "Moon Shards Refresh", "Fragments de lune de retour" },
        { "Kingdom Enter Refresh", "Arrivée au royaume rejouée" },
        { "Warp Text Refresh", "Explication des drapeaux" },
        { "Seeds Refresh", "Graines de retour" },
        { "Seeds Growing", "Pousse instantanée" },
        { "Cutscene Refresh", "Cinématiques rejouées" },
        { "Text Refresh", "Textes de Cappy" },
        { "Always Manual Cutscene", "Cinématiques manuelles" },
        { "Always Checkpoints", "Drapeaux toujours actifs" },
        { "Disable Autosave", "Sans sauvegarde auto" },
        { "Disable TP Puppet", "Sans placement auto" },
        { "Skip Cloud", "Bowser des Nuages vaincu" },
        { "All Checkpoints", "Tous les drapeaux" },
        { "Luigi Pixels", "Luigi 8-bit" },
        { "Spawn All Hint Art", "Toutes les peintures indices" },
        { "Pipe Maze Override", "Labyrinthe de tuyaux fixe" },
        { "No Checkpoint Touch", "Drapeaux non activés" },
        { "No Damage", "Pas de dégâts" },
        { "Disable Music", "Sans musique" },
        { "Disable Moon Lock", "Sans verrou de lunes" },
        { "Skip Broodals", "Broodals vaincus" },
        { "Always Odyssey Power Up", "Odyssée toujours rechargée" },
        { "Odyssey In All Kingdoms", "Odyssée dans tous les royaumes" },
        { "Night Metro Multi Moon", "Multi-lune de la Ville de nuit" },
        { "Open Outfit Doors", "Portes des salles à costume" },
        { "The guards of the outfit rooms let Mario in whatever outfit he wears.",
          "Les gardiens des salles à costume laissent entrer Mario, quelle que soit sa tenue." },
        { "Noclip", "Vol libre (noclip)" },
        { "Reload Scene at Pos", "Recharger sur place" },

        // Options: help
        { "Moons you collect still count (moon counter, story) but come back as new moons: you can collect the same moon again as many times as you want.",
          "Les lunes que tu prends comptent (compteur de lunes, histoire) mais reviennent comme neuves : tu peux reprendre "
          "la même lune autant de fois que tu veux." },
        { "Moons already collected look normal again (not gray), as if they were never collected.",
          "Les lunes déjà obtenues redeviennent normales (et non grises), comme si tu ne les avais jamais prises." },
        { "Purple coins come back when the scene is reloaded.",
          "Les pièces violettes réapparaissent quand tu recharges la scène." },
        { "Doors already opened are closed again when the scene is reloaded.",
          "Les portes déjà ouvertes se referment quand tu recharges la scène." },
        { "Moon shards (and their counter) come back, even when their moon is already collected.",
          "Les fragments de lune (et leur compteur) réapparaissent, même si leur lune est déjà prise." },
        { "The Odyssey arrival cutscene of the kingdom plays again.",
          "La cinématique d'arrivée de l'Odyssée dans le royaume se rejoue." },
        { "Cappy says his texts again (the checkpoint warp explanation when you touch a flag, his texts at the start of the kingdoms), and the camera with the story text at the start of a kingdom plays again (back at each reload).",
          "Cappy redit ses textes (l'explication de la téléportation quand tu touches un drapeau, ses textes du début des "
          "royaumes) et la caméra avec le texte de l'histoire au début d'un royaume se rejoue (à chaque rechargement)." },
        { "Seeds come back and pots are empty at each reload. The plant grows normally (with time or water).",
          "Les graines réapparaissent et les pots sont vides à chaque rechargement. La plante pousse normalement "
          "(avec le temps ou avec de l'eau)." },
        { "A planted seed grows to the end at once and its moon appears. Works with or without Seeds Refresh.",
          "Une graine plantée pousse d'un coup jusqu'au bout et sa lune apparaît. Marche avec ou sans « Graines de retour »." },
        { "Cutscenes and first talks of the characters (Toads, Peach, Captain Toad...) play again, with the camera with the story text at the start of a kingdom.",
          "Les cinématiques et les premiers dialogues des personnages (Toads, Peach, Capitaine Toad...) se rejouent, avec la caméra et le texte de l'histoire au début d'un royaume." },
        { "Cappy's texts (at the start of the kingdoms, the checkpoint flags explanation...) come back at each reload. Off: they never show.",
          "Les textes de Cappy (au début des royaumes, l'explication des drapeaux...) reviennent à chaque rechargement. Désactivé : ils ne s'affichent jamais." },
        { "Cutscenes never skip by themselves: you choose when to skip them.",
          "Les cinématiques ne se passent jamais toutes seules : c'est toi qui choisis quand les passer." },
        { "You can always warp to the flags from the map, even where the game normally forbids it.",
          "Tu peux toujours te téléporter aux drapeaux depuis la carte, même là où le jeu l'interdit normalement." },
        { "The game no longer saves by itself: your save file stays the same while you practice.",
          "Le jeu ne sauvegarde plus tout seul : ta sauvegarde ne bouge pas pendant que tu t'entraînes." },
        { "Mario is not placed automatically during the moon get animation.",
          "Mario n'est plus replacé automatiquement pendant l'animation où il prend une lune." },
        { "In the Cloud Kingdom, Bowser is beaten as soon as his intro cutscene ends: the battle end cutscene plays right away.",
          "Au Pays des Nuages, Bowser est vaincu dès la fin de sa cinématique d'entrée : la cinématique de fin du combat "
          "se lance tout de suite." },
        { "Every flag of the kingdom is available on the map, even the ones not touched yet.",
          "Tous les drapeaux du royaume sont disponibles sur la carte, même ceux que tu n'as pas encore touchés." },
        { "The 8-bit Luigi hidden in some bushes always comes out (10% chance in the normal game). Reload the scene after "
          "turning it on.",
          "Le Luigi 8-bit caché dans certains buissons sort à chaque fois (10 % de chances dans le jeu normal). "
          "Recharge la scène après avoir activé l'option." },
        { "Every hint art appears, without having to unlock it.",
          "Toutes les peintures d'indices apparaissent, sans avoir besoin de les débloquer." },
        { "Forces the path of the pipe mazes (the same layout every time).",
          "Impose le chemin des labyrinthes de tuyaux (toujours le même agencement)." },
        { "Touching a flag does not activate it (your tries are not changed by a flag touched by mistake).",
          "Toucher un drapeau ne l'active pas : un drapeau touché par erreur ne fausse pas tes essais." },
        { "Mario no longer loses health.", "Mario ne perd plus de vie." },
        { "Turns the music off: only the sound effects stay.", "Coupe la musique : il ne reste que les bruitages." },
        { "The Odyssey can leave without the number of moons the kingdom normally asks for.",
          "L'Odyssée peut partir sans avoir le nombre de lunes que le royaume demande normalement." },
        { "As soon as the intro of a Broodal fight ends (Hariet, Rango, Spewart, Madame Broode), the boss is beaten and its "
          "Multi Moon appears.",
          "Dès la fin de l'intro d'un combat contre les Broodals (Hariet, Rango, Spewart, Madame Broode), le boss est "
          "vaincu et sa Multi-lune apparaît." },
        { "Each time you go to the Odyssey, even without moons, it powers up and offers the next kingdom of the story.",
          "Chaque fois que tu vas à l'Odyssée, même sans lune, elle se recharge et te propose le royaume suivant de "
          "l'histoire." },
        { "The Odyssey is in every kingdom, even the ones your save has not reached yet, and you can warp to it from the map.",
          "L'Odyssée est présente dans tous les royaumes, même ceux que ta sauvegarde n'a pas encore atteints, et tu peux "
          "t'y téléporter depuis la carte." },
        { "At the start of Night Metro, the Mechawiggler Multi Moon falls next to Mario: grab it to go straight to day Metro.",
          "Au début de la Ville de nuit, la Multi-lune du Méca-Wiggler tombe à côté de Mario : prends-la pour passer "
          "directement à la Ville de jour." },
        { "Mario flies through everything. Left stick = move, ZR = up, ZL = down, X/Y = faster, A/B = slower. Turns off at "
          "each game launch.",
          "Mario vole et traverse tout. Stick gauche = avancer, ZR = monter, ZL = descendre, X/Y = plus vite, "
          "A/B = moins vite. Se désactive à chaque lancement du jeu." },
        { "When the scene is reloaded, Mario comes back exactly where he was.",
          "Quand tu recharges la scène, Mario revient exactement là où il était." },

        // Stages
        { "Kingdom", "Royaume" },
        { "Scenario", "Scénario" },
        { "Warp to Stage", "Se téléporter" },
        { "Entrance", "Entrée" },
        { "Inside", "Intérieur" },
        { "No picture", "Pas d'image" },
        { "Cap Kingdom", "Pays des Chapeaux" },
        { "Cascade Kingdom", "Pays des Chutes" },
        { "Sand Kingdom", "Pays des Sables" },
        { "Wooded Kingdom", "Pays des Forêts" },
        { "Lake Kingdom", "Pays des Lacs" },
        { "Cloud Kingdom", "Pays des Nuages" },
        { "Lost Kingdom", "Pays Oublié" },
        { "Metro Kingdom", "Pays de la Ville" },
        { "Snow Kingdom", "Pays des Neiges" },
        { "Seaside Kingdom", "Pays de la Mer" },
        { "Luncheon Kingdom", "Pays des Gourmets" },
        { "Ruined Kingdom", "Pays des Ruines" },
        { "Bowser's Kingdom", "Pays de Bowser" },
        { "Moon Kingdom", "Pays de la Lune" },
        { "Mushroom Kingdom", "Royaume Champignon" },
        { "Dark Side", "Face cachée de la Lune" },
        { "Darker Side", "Face sombre de la Lune" },
        { "Cap Sub Area", "Sous-zones (Chapeaux)" },
        { "Cascade Sub Area", "Sous-zones (Chutes)" },
        { "Sand Sub Area", "Sous-zones (Sables)" },
        { "Wooded Sub Area", "Sous-zones (Forêts)" },
        { "Lake Sub Area", "Sous-zones (Lacs)" },
        { "Cloud Sub Area", "Sous-zones (Nuages)" },
        { "Lost Sub Area", "Sous-zones (Oublié)" },
        { "Metro Sub Area", "Sous-zones (Ville)" },
        { "Snow Sub Area", "Sous-zones (Neiges)" },
        { "Seaside Sub Area", "Sous-zones (Mer)" },
        { "Luncheon Sub Area", "Sous-zones (Gourmets)" },
        { "Ruined Sub Area", "Sous-zones (Ruines)" },
        { "Bowser Sub Area", "Sous-zones (Bowser)" },
        { "Moon Sub Area", "Sous-zones (Lune)" },
        { "Mushroom Sub Area", "Sous-zones (Champignon)" },
        { "Dark Side Sub Area", "Sous-zones (Face cachée)" },
        { "Darker Side Sub Area", "Sous-zones (Face sombre)" },

        // Misc
        { "Outfit Body", "Tenue" },
        { "Outfit Cap", "Chapeau" },
        { "Apply Outfit", "Mettre la tenue" },
        { "Save Position", "Sauver la position" },
        { "Load Position", "Charger la position" },
        { "Restart Scene", "Recommencer la scène" },
        { "Reload at Pos", "Recharger sur place" },
        { "Restart at Pos", "Recommencer sur place" },
        { "Kill Mario", "Tuer Mario" },
        { "Damage Mario", "Blesser Mario" },
        { "Life Up Heart", "Coeur de vie" },
        { "Heal Mario", "Soigner Mario" },
        { "Add 1000 Coins", "Ajouter 1000 pièces" },
        { "Remove 1000 Coins", "Retirer 1000 pièces" },
        { "Reload Scene", "Recharger la scène" },
        { "Previous Scene", "Scène précédente" },
        { "Wiggler Pattern", "Motif du Méca-Wiggler" },
        { "Rename All Moons", "Renommer toutes les lunes" },
        { "Reset All Moon Names", "Noms des lunes d'origine" },
        { "Random", "Aléatoire" },
        { "Ghost", "Fantôme" },
        { "Nose", "Nez" },
        { "Medal", "Médaille" },
        { "Plane", "Avion" },
        { "Hangman", "Pendu" },
        { "Spanish", "Espagnol" },
        { "Siblings", "Frères et soeurs" },
        { "Snake", "Serpent" },
        { "Mushroom", "Champignon" },
        { "Ear", "Oreille" },
        { "Bomb", "Bombe" },
        { "Bird", "Oiseau" },
        { "Star", "Étoile" },

        // Timer
        { "Segment Timer", "Afficher le chrono" },
        { "Timer Split", "Temps intermédiaire" },
        { "Start Type", "Départ" },
        { "Start Count", "Nombre au départ" },
        { "End Type", "Arrivée" },
        { "End Count", "Nombre à l'arrivée" },
        { "Start Timer", "Lancer le chrono" },
        { "Stop Timer", "Arrêter le chrono" },
        { "Reset Timer", "Remettre le chrono à zéro" },
        { "Preset Pos", "Position prédéfinie" },
        { "Reset Position", "Position par défaut" },
        { "Size", "Taille" },
        { "Motion Controller", "Manette (Motion)" },
        { "Auto", "Auto" },
        { "Style \"Motion\": the controller whose motion is shown, and its shape. Auto: the controller in use (Pro Controller, or Joy-Con).",
          "Style « Motion » : la manette dont on voit les mouvements, et sa forme. Auto : la manette utilisée (manette Pro ou Joy-Con)." },
        { "Style \"Motion\": the key that makes the current orientation of the controller the front view.",
          "Style « Motion » : la touche qui fait de la position actuelle de la manette la vue de face." },
        { "Custom", "Perso" },
        { "Splits", "Splits" },
        { "Split", "Split" },
        { "Split Count", "Nombre de splits" },
        { "Edit Split", "Split à régler" },
        { "Split Type", "Fin du split" },
        { "The event that ends the split chosen in Edit Split. \"End Type\": the End Type of the timer.",
          "L'événement qui termine le split choisi dans « Split à régler ». « Arrivée » : le type de fin du timer." },
        { "Each End Type event (after End Count) goes to the next split, the last split ends the run. Under the timer, each split shows the time when it ended.",
          "Chaque événement du type de fin (après End Count) passe au split suivant, le dernier split termine le run. Sous le timer, chaque split affiche le temps auquel il s'est terminé." },
        { "Press the buttons...", "Appuie sur les touches..." },
        { "Center", "Centre" },
        { "Top Left", "En haut à gauche" },
        { "Top Center", "En haut au centre" },
        { "Top Right", "En haut à droite" },
        { "Bottom Left", "En bas à gauche" },
        { "Bottom Right", "En bas à droite" },
        { "None", "Aucun" },
        { "Press Start", "Nouvelle partie" },
        { "Odyssey Power Up", "Odyssée rechargée" },
        { "Black Screen", "Écran noir" },
        { "White Screen", "Écran blanc" },
        { "Capture", "Capture" },
        { "Damage", "Dégâts" },
        { "Escape Wire", "Fil de la fuite" },
        { "Moon Get", "Lune touchée" },
        { "Shine Tick", "Compteur de lunes" },
        { "2 Moons (Grab)", "2 lunes (touchées)" },
        { "2 Moons (Tick)", "2 lunes (compteur)" },
        { "Shine Grab", "Lune brandie" },

        // Input Display
        { "2P Mode", "Mode 2 joueurs" },
        { "Style", "Style" },
        { "Classic", "Classique" },
        { "P2 Preset Pos", "J2 : position prédéfinie" },
        { "P2 Position X", "J2 : position X" },
        { "P2 Position Y", "J2 : position Y" },
        { "P2 Reset Position", "J2 : position par défaut" },
        { "Button Color", "Couleur des boutons" },
        { "Pressed Color", "Couleur appuyée" },
        { "Stick Color", "Couleur des sticks" },
        { "Ring Color", "Couleur des contours" },
        { "BG Color", "Couleur du fond" },
        { "White", "Blanc" },
        { "Gray", "Gris" },
        { "Black", "Noir" },
        { "Red", "Rouge" },
        { "Blue", "Bleu" },
        { "Yellow", "Jaune" },
        { "Green", "Vert" },
        { "Orange", "Orange" },
        { "Purple", "Violet" },
        { "Pink", "Rose" },
        { "LightBlue", "Bleu clair" },
        { "Multicolor", "Multicolore" },
        { "Player 1", "Joueur 1" },
        { "Player 2", "Joueur 2" },

        // Keybinds
        { "Open Menu", "Ouvrir le menu" },
        { "Life Up", "Coeur de vie" },
        { "Prev Scene", "Scène précédente" },
        { "Warp Last CP", "Dernier drapeau" },
        { "Timer Start", "Lancer le chrono" },
        { "Timer End", "Arrêter le chrono" },
        { "Timer Reset", "Chrono à zéro" },
        { "Reset Motion", "Recentrer les mouvements" },
        { "Motion reset", "Mouvements recentrés" },
        { "Wheel Key", "Touche de la roue" },
        { "Wheel Size", "Taille de la roue" },
        { "Wheel Slot 1", "Roue : case 1" },
        { "Wheel Slot 2", "Roue : case 2" },
        { "Wheel Slot 3", "Roue : case 3" },
        { "Wheel Slot 4", "Roue : case 4" },
        { "Wheel Slot 5", "Roue : case 5" },
        { "Wheel Slot 6", "Roue : case 6" },
        { "Wheel Slot 7", "Roue : case 7" },
        { "Wheel Slot 8", "Roue : case 8" },
        { "Wheel Slot 9", "Roue : case 9" },
        { "Wheel Slot 10", "Roue : case 10" },
        { "Wheel Slot 11", "Roue : case 11" },
        { "Wheel Slot 12", "Roue : case 12" },
        { "Disabled", "Désactivé" },
        { "L + DLeft", "L + croix gauche" },
        { "L + DRight", "L + croix droite" },
        { "R + DLeft", "R + croix gauche" },
        { "R + DRight", "R + croix droite" },
        { "D-Pad Left", "Croix gauche" },
        { "D-Pad Right", "Croix droite" },
        { "D-Pad Down", "Croix bas" },
        { "D-Pad Up", "Croix haut" },
        { "L + D-Pad Left", "L + croix gauche" },
        { "L + D-Pad Right", "L + croix droite" },
        { "L + D-Pad Down", "L + croix bas" },
        { "L + D-Pad Up", "L + croix haut" },
        { "ZL + D-Pad Left", "ZL + croix gauche" },
        { "ZL + D-Pad Right", "ZL + croix droite" },
        { "ZL + D-Pad Down", "ZL + croix bas" },
        { "ZL + D-Pad Up", "ZL + croix haut" },
        { "R + D-Pad Left", "R + croix gauche" },
        { "R + D-Pad Right", "R + croix droite" },
        { "LStick", "Clic stick gauche" },
        { "RStick", "Clic stick droit" },
        { "L + LStick", "L + clic stick gauche" },
        { "L + RStick", "L + clic stick droit" },
        { "Off", "Désactivée" },
        { "L3 or R3", "L3 ou R3" },
        { "L (hold)", "L (maintenu)" },
        { "R (hold)", "R (maintenu)" },
        { "Empty", "Vide" },

        // Quick wheel
        { "Release: OK", "Relâcher : valider" },
        { "Release the stick: OK", "Relâcher le stick : valider" },
        { "ON", "activé" },
        { "OFF", "désactivé" },

        // Info / Settings
        { "Info Window", "Fenêtre d'infos" },
        { "Player Info", "Infos sur Mario" },
        { "No Game Data Holder", "Pas de données de jeu" },
        { "Jumps: %d", "Sauts : %d" },
        { "Cap Throws: %d", "Lancers de Cappy : %d" },
        { "Total Coins: %d", "Pièces au total : %d" },
        { "Play Time Total: %lu", "Temps de jeu total : %lu" },
        { "Play Time Across File: %lu", "Temps de jeu de la sauvegarde : %lu" },
        { "No Player", "Pas de Mario" },
        { "No Pose", "Pas de position" },
        { "Menu Corner", "Coin du menu" },
        { "Language", "Langue" },

        // Messages
        { "Position Saved", "Position sauvegardée" },
        { "Position Loaded", "Position chargée" },
        { "No Saved Position", "Aucune position sauvegardée" },
        { "Reloaded at Position", "Rechargé sur place" },
        { "Outfit Applied", "Tenue mise" },
        { "Tp Index: %d", "Emplacement de position : %d" },
        { "Wiggler Pattern: %s", "Motif du Méca-Wiggler : %s" },
        { "Timer started", "Chrono lancé" },
        { "Timer stopped", "Chrono arrêté" },
        { "Timer reset", "Chrono remis à zéro" },
        { "Every moon renamed", "Toutes les lunes sont renommées" },
        { "Moon names reset", "Les lunes ont retrouvé leur nom" },
        { "All moon names reset", "Toutes les lunes ont retrouvé leur nom" },
        { "Moon renamed (reload the scene)", "Lune renommée (recharge la scène)" },
        { "%s: %s", "%s : %s" },

        // On-screen keyboard
        { "Name of every moon", "Nom de toutes les lunes" },
        { "Space", "Espace" },
        { "Caps", "Maj" },
        { "Caps: ON", "Maj : activé" },
        { "Caps: off", "Maj : désactivé" },
        { "Erase", "Effacer" },
        { "OK (+)", "Valider (+)" },
        { "Cancel (-)", "Annuler (-)" },
        { "D-Pad: move   A: type   B: erase   Y: caps   +: OK   -: cancel",
          "Croix : déplacer   A : taper   B : effacer   Y : majuscules   + : valider   - : annuler" },
    };

    // Pointer -> translation cache (the strings are literals / static tables): open addressing, written once per key
    constexpr int cCacheSize = 2048;
    const char* sCacheKeys[cCacheSize] = {};
    const char* sCacheValues[cCacheSize] = {};

    const char* lookup(const char* en) {
        for (const Entry& e : cFrench)
            if (strcmp(e.en, en) == 0) return e.fr;
        return en;
    }

} // namespace

bool isFrench() {
    SettingsMgr::Settings* s = SettingsMgr::getSettings();
    return s && s->mLanguage == 1;
}

const char* tr(const char* en) {
    if (!en || !en[0] || !isFrench()) return en;
    uintptr_t h = (reinterpret_cast<uintptr_t>(en) >> 3) * 2654435761u;
    for (int probe = 0; probe < 16; probe++) {
        int i = int((h + probe) & (cCacheSize - 1));
        const char* key = __atomic_load_n(&sCacheKeys[i], __ATOMIC_ACQUIRE);
        if (key == en) return sCacheValues[i];
        if (!key) {
            const char* fr = lookup(en);
            sCacheValues[i] = fr;
            __atomic_store_n(&sCacheKeys[i], en, __ATOMIC_RELEASE);
            return fr;
        }
    }
    return lookup(en);
}

const char* trColon() {
    return isFrench() ? " : " : ": ";
}

} // namespace btt
