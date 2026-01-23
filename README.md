# VirtualController

Système de contrôleurs virtuels multi-joueurs utilisant ViGEmBus pour Windows.

##  Structure

```
VirtualController/
├── src/
│   ├── core/         # Application principale et fenêtre
│   ├── ui/           # Composants UI (tabs, fenêtres de test)
│   ├── input/        # LocalInputSource (temporaire - sera remplacé par l'app mobile)
│   ├── managers/     # MultiControllerManager (gestion ViGEm)
│   ├── interfaces/   # IInputSource (abstraction des sources d'input)
│   ├── scanner/      # Découverte de jeux dans ./games/
│   ├── networks/     # Gestions WS des virtual controllers 
│   └── qrcode/       # lib de génération de QR codes pour la connection mobile (Copyright (c) 2023 Alex Spataru <https://github.com/alex-spataru>)
├── games/            # Dossiers de jeux (format: nom_jeu/nom_jeu.exe)
└── docs/             # Documentation technique détaillée
```

##  Prérequis

- Windows 10/11 (64-bit)
- Visual Studio 2019+ avec C++
- Qt 6.6+ (Core, Widgets, Gui)
- CMake 3.16+
- [ViGEmBus Driver](https://github.com/nefarius/ViGEmBus/releases)

##  Build

```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

Exécutable : `build/bin/Release/GameLibraryLauncher.exe`

##  Utilisation

1. Lancer GameLibraryLauncher.exe
2. Ouvrir L'appli sur le téléphone
3. Connecter le téléphone grace au QRcode
4. Lancer un jeu depuis l'onglet "Game Library"

### Format des Jeux

Les jeux doivent être dans `./games/` avec cette structure :
```
games/
└── nom_jeu/
    └── nom_jeu.exe    # Exécutable (nom doit correspondre au dossier)
    └── nom_jeu.ico    # Icone (optionnelle)
```

L'icône `.ico` est optionnelle. Le scanner cherche uniquement les `.exe` correspondants.
Les `.ico` trouvés sont utilisés pour l'icone du jeu associé

##  Architecture

### Modules Clés

**`MultiControllerManager`** : Gère jusqu'à 4 contrôleurs ViGEm avec retry logic (3 tentatives)

**`IInputSource`** : Interface abstraite pour les sources d'input
-  `LocalInputSource` (UI de test, temporaire)
-  `WebSocketInputSource` Source réseau depuis application mobile

**`GameScanner`** : Découverte automatique des jeux
- Scan de `./games/` pour trouver les exécutables
- Préparé pour téléchargement distant futur

**`UI`**: Fenetres QT pour l'interface utilisateur

## Développement

### Ajouter un Module

1. Créer le dossier : `src/mon_module/`
2. Ajouter fichiers `.h` et `.cpp`
3. Mettre à jour `CMakeLists.txt` :
```cmake
set(MON_MODULE_SOURCES src/mon_module/MaClasse.cpp)
set(MON_MODULE_HEADERS src/mon_module/MaClasse.h)

# Ajouter à ALL_SOURCES et include_directories
```


##  Roadmap

- [x] Support 4 contrôleurs simultanés
- [x] Stick analogique + D-Pad 8 directions
- [x] Application mobile (remplacement LocalInputSource)
- [ ] Système de téléchargement de jeux distant
- [ ] Tests unitaires

## ⚠️ Notes

- **LocalInputSource** : Interface de test et de démo, a vocation à etre remplacé par l'app mobile
- **Windows uniquement** : ViGEmBus est Windows-only, pas de portabilité prévue
