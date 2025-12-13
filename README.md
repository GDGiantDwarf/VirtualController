# Game Library Launcher

Un launcher de jeux avec support de contrôleur virtuel, créé en Qt6 et C++.

## Fonctionnalités

### Onglet "Connect a Controller"
- Interface pour connecter un contrôleur
- Placeholder pour QR code (fonctionnalité future)
- Bouton pour ouvrir une fenêtre de contrôleur virtuel avec des boutons cliquables
- La fenêtre du contrôleur virtuel simule une manette avec :
  - D-Pad (↑↓←→)
  - Boutons d'action (A, B, X, Y)
  - Boutons shoulder (LB, RB)
  - Boutons système (Start, Select)

### Onglet "Game Library"
- Scanne automatiquement le dossier `./games/`
- Affiche tous les jeux valides trouvés
- Un jeu est valide s'il contient :
  - Un dossier avec le nom du jeu
  - Un fichier `.exe` avec le même nom
  - Un fichier `.ico` avec le même nom
- Cliquer sur un jeu lance son exécutable
- Bouton "Refresh" pour recharger la liste

## Structure des dossiers de jeux

```
./games/
├── snake/
│   ├── snake.exe
│   └── snake.ico
├── pong/
│   ├── pong.exe
│   └── pong.ico
└── breakout/
    ├── breakout.exe
    └── breakout.ico
```

## Prérequis

- CMake 3.16 ou supérieur
- Qt 6 (Core, Widgets, Gui)
- Compilateur C++17 compatible (MSVC, GCC, Clang)

### Installation de Qt6

**Windows:**
```bash
# Via Qt Installer
# Télécharger depuis https://www.qt.io/download-qt-installer

# Ou via vcpkg
vcpkg install qt6-base:x64-windows
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get update
sudo apt-get install qt6-base-dev qt6-base-dev-tools
```

**macOS:**
```bash
brew install qt@6
```

## Compilation

### Windows (Visual Studio)

```bash
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### Linux / macOS

```bash
mkdir build
cd build
cmake ..
make
```

## Exécution

```bash
# Windows
.\build\bin\Release\GameLibraryLauncher.exe

# Linux / macOS
./build/bin/GameLibraryLauncher
```

## Structure du projet

```
game_library/
├── CMakeLists.txt
├── README.md
├── src/
│   ├── main.cpp
│   ├── MainWindow.h/cpp          # Fenêtre principale avec onglets
│   ├── GameLibraryTab.h/cpp      # Onglet bibliothèque de jeux
│   ├── ControllerTab.h/cpp       # Onglet connexion contrôleur
│   ├── VirtualControllerWindow.h/cpp  # Fenêtre contrôleur virtuel
│   ├── GameScanner.h/cpp         # Scanner de jeux
│   └── GameInfo.h                # Structure de données
└── games/                        # Dossier des jeux (à créer)
```

## Utilisation

1. **Lancer l'application**
2. **Onglet "Connect a Controller"** :
   - Cliquez sur "Open Virtual Controller (Test)"
   - Une fenêtre s'ouvre avec des boutons simulant une manette
   - Cliquez sur les boutons pour tester (les événements sont loggés dans la console)
3. **Onglet "Game Library"** :
   - La liste des jeux se remplit automatiquement
   - Cliquez sur un jeu pour le lancer
   - Utilisez "Refresh" pour recharger la liste après avoir ajouté des jeux

## Prochaines étapes

Pour intégrer ViGEm (manette virtuelle réelle) :

1. Installer ViGEmBus : https://github.com/nefarius/ViGEmBus/releases
2. Ajouter ViGEmClient au projet :
   ```cmake
   find_package(ViGEmClient REQUIRED)
   target_link_libraries(GameLibraryLauncher ViGEmClient)
   ```
3. Implémenter dans `ControllerTab::onControllerButtonPressed/Released` :
   ```cpp
   #include <ViGEm/Client.h>
   
   // Créer un contrôleur Xbox 360 virtuel
   PVIGEM_CLIENT client = vigem_alloc();
   PVIGEM_TARGET controller = vigem_target_x360_alloc();
   
   // Envoyer les inputs
   XUSB_REPORT report;
   report.wButtons = XUSB_GAMEPAD_A; // Exemple
   vigem_target_x360_update(client, controller, report);
   ```

## Notes de développement

- Les événements des boutons du contrôleur virtuel sont actuellement loggés dans la console
- Le système est prêt pour l'intégration avec ViGEm
- L'architecture permet d'ajouter facilement la connexion smartphone via réseau local
- Les jeux s'exécutent dans des processus séparés pour éviter les crashes du launcher

## TODO

- [ ] Implémenter ViGEm pour créer une vraie manette virtuelle
- [ ] Ajouter la génération de QR code pour connexion smartphone
- [ ] Implémenter le serveur réseau pour recevoir les inputs du smartphone
- [ ] Ajouter des métadonnées de jeux (description, screenshot, etc.)
- [ ] Créer un système de favoris
- [ ] Ajouter la détection automatique de nouveaux jeux

## Licence

Projet étudiant - Libre d'utilisation à des fins éducatives.