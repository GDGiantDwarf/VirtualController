# VirtualController v2.0

Système de contrôleurs virtuels multi-joueurs utilisant ViGEmBus pour Windows.

## 📁 Structure

```
VirtualController/
├── src/
│   ├── core/         # Application principale et fenêtre
│   ├── ui/           # Composants UI (tabs, fenêtres de test)
│   ├── input/        # LocalInputSource (temporaire - sera remplacé par l'app mobile)
│   ├── managers/     # MultiControllerManager (gestion ViGEm)
│   ├── interfaces/   # IInputSource (abstraction des sources d'input)
│   └── scanner/      # Découverte de jeux dans ./games/
├── games/            # Dossiers de jeux (format: nom_jeu/nom_jeu.exe)
└── docs/             # Documentation technique détaillée
```

## 🔧 Prérequis

- Windows 10/11 (64-bit)
- Visual Studio 2019+ avec C++
- Qt 6.6+ (Core, Widgets, Gui)
- CMake 3.16+
- [ViGEmBus Driver](https://github.com/nefarius/ViGEmBus/releases)

## 🚀 Build

```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

Exécutable : `build/bin/Release/GameLibraryLauncher.exe`

## 🎮 Utilisation

1. Lancer GameLibraryLauncher.exe
2. Onglet "Local Controller Management" → Ajouter des contrôleurs (max 4)
3. Tester avec les fenêtres de contrôle (provisoires)
4. Lancer un jeu depuis l'onglet "Game Library"

### Format des Jeux

Les jeux doivent être dans `./games/` avec cette structure :
```
games/
└── nom_jeu/
    └── nom_jeu.exe    # Exécutable (nom doit correspondre au dossier)
```

L'icône `.ico` est optionnelle. Le scanner cherche uniquement les `.exe` correspondants.

## 🏗️ Architecture

### Modules Clés

**`MultiControllerManager`** : Gère jusqu'à 4 contrôleurs ViGEm avec retry logic (3 tentatives)

**`IInputSource`** : Interface abstraite pour les sources d'input
- Actuel : `LocalInputSource` (UI de test, temporaire)
- Future : Source réseau depuis application mobile

**`GameScanner`** : Découverte automatique des jeux
- Scan de `./games/` pour trouver les exécutables
- Préparé pour téléchargement distant futur

### Workflow

```
LocalInputSource → MultiControllerManager → ViGEm → Jeux
(fenêtre test)     (gère 4 contrôleurs)     (driver)
```

## 📝 Développement

### Ajouter un Module

1. Créer le dossier : `src/mon_module/`
2. Ajouter fichiers `.h` et `.cpp`
3. Mettre à jour `CMakeLists.txt` :
```cmake
set(MON_MODULE_SOURCES src/mon_module/MaClasse.cpp)
set(MON_MODULE_HEADERS src/mon_module/MaClasse.h)

# Ajouter à ALL_SOURCES et include_directories
```

### Conventions

- Headers/Sources : PascalCase (`MaClasse.h`, `MaClasse.cpp`)
- Dossiers : snake_case (`mon_module/`)
- Includes : Pas de chemins relatifs grâce aux include directories CMake

```cpp
// ✅ Bon
#include "IInputSource.h"
#include "MultiControllerManager.h"

// ❌ Éviter
#include "../interfaces/IInputSource.h"
```

## 🎯 Roadmap

- [x] Support 4 contrôleurs simultanés
- [x] Architecture modulaire
- [x] Stick analogique + D-Pad 8 directions
- [ ] Application mobile (remplacement LocalInputSource)
- [ ] Système de téléchargement de jeux distant
- [ ] Tests unitaires

## 📚 Documentation Détaillée

- `docs/ARCHITECTURE_MODULAIRE.md` : Architecture complète
- `docs/modifications_documentation.md` : Changements techniques
- `docs/guide_cicd_basique.md` : CI/CD et automatisation

## ⚠️ Notes

- **LocalInputSource** : Interface de test temporaire, sera remplacée par l'app mobile
- **Windows uniquement** : ViGEmBus est Windows-only, pas de portabilité prévue
- **Build folder** : Exclu du repo (.gitignore), ne pas commiter

---

**Version** : 2.0.0  
**License** : Voir projet original VirtualController