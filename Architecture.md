# Architecture du Projet

## Vue d'ensemble

Ce projet est un launcher de jeux avec support de manette virtuelle, conçu pour permettre l'utilisation d'un smartphone comme contrôleur de jeu.

## Architecture en couches

```
┌─────────────────────────────────────────────────────────────┐
│                    Interface Utilisateur                     │
│                        (Qt Widgets)                          │
├─────────────────────────────────────────────────────────────┤
│  Onglet Controller    │         Onglet Bibliothèque         │
│  - QR Code (futur)    │         - Liste de jeux             │
│  - Bouton test        │         - Lancement de jeux         │
│  - Status             │         - Rafraîchissement          │
└───────────┬───────────┴────────────────┬────────────────────┘
            │                            │
            │                            │
┌───────────▼───────────┐    ┌──────────▼──────────┐
│  Virtual Controller   │    │    Game Scanner     │
│      Window           │    │  - Scan ./games/    │
│  - Boutons simulés    │    │  - Valide structure │
│  - Émission signaux   │    │  - Crée GameInfo    │
└───────────┬───────────┘    └─────────────────────┘
            │
            │
┌───────────▼───────────────────────────────────────┐
│          Virtual Gamepad Manager                  │
│              (Futur - ViGEm)                      │
│  - Crée contrôleur Xbox 360 virtuel               │
│  - Traduit signaux → inputs contrôleur            │
│  - Communique avec driver ViGEmBus                │
└───────────┬───────────────────────────────────────┘
            │
            │
┌───────────▼───────────────────────────────────────┐
│              ViGEmBus Driver                      │
│       (Contrôleur virtuel système)                │
└───────────┬───────────────────────────────────────┘
            │
            │
┌───────────▼───────────────────────────────────────┐
│                    Jeux                           │
│  - Lisent inputs contrôleur standard              │
│  - Ignorent que c'est virtuel                     │
│  - Exécutés dans processus séparé                 │
└───────────────────────────────────────────────────┘
```

## Flux de données - MVP actuel

```
Utilisateur clique bouton
         │
         ▼
VirtualControllerWindow::onButtonPressed
         │
         ▼
emit buttonPressed("button_name")
         │
         ▼
ControllerTab::onControllerButtonPressed
         │
         ▼
[Affichage status - LOG pour l'instant]
```

## Flux de données - Avec ViGEm (prochaine étape)

```
Utilisateur clique bouton
         │
         ▼
VirtualControllerWindow::onButtonPressed
         │
         ▼
emit buttonPressed("button_name")
         │
         ▼
ControllerTab::onControllerButtonPressed
         │
         ▼
VirtualGamepadManager::pressButton(XUSB_BUTTON)
         │
         ▼
vigem_target_x360_update(...)
         │
         ▼
ViGEmBus Driver crée input système
         │
         ▼
Jeu reçoit input comme manette réelle
```

## Flux de données - Avec smartphone (futur)

```
Smartphone (React Native)
         │ WebSocket/UDP
         ▼
NetworkReceiver (C++ Server)
         │
         ▼
ControllerTab::onNetworkInput
         │
         ▼
VirtualGamepadManager::pressButton
         │
         ▼
ViGEmBus Driver
         │
         ▼
Jeu
```

## Composants principaux

### 1. MainWindow
**Responsabilité** : Fenêtre principale avec système d'onglets
- Gère les onglets (Controller, Library)
- Barre de menu
- État global de l'application

### 2. ControllerTab
**Responsabilité** : Interface de connexion du contrôleur
- Affiche QR code (futur)
- Gère la fenêtre de test du contrôleur
- Reçoit les événements boutons
- Communique avec VirtualGamepadManager (futur)

### 3. VirtualControllerWindow
**Responsabilité** : Interface de test du contrôleur
- Affiche boutons cliquables (D-Pad, ABXY, LB/RB, Start/Select)
- Émet des signaux Qt pour chaque bouton
- Feedback visuel (bordure rouge sur pression)

### 4. GameLibraryTab
**Responsabilité** : Gestion de la bibliothèque de jeux
- Affiche liste des jeux disponibles
- Lance les jeux via QProcess
- Gère les erreurs de lancement
- Rafraîchissement de la liste

### 5. GameScanner
**Responsabilité** : Découverte automatique de jeux
- Scanne le dossier `./games/`
- Valide la structure (dossier/exe/ico)
- Crée objets GameInfo
- Logging des erreurs

### 6. GameInfo (struct)
**Responsabilité** : Données d'un jeu
- Nom du jeu
- Chemins (dossier, exe, ico)
- Icône chargée (QIcon)

### 7. VirtualGamepadManager (futur)
**Responsabilité** : Contrôleur virtuel ViGEm
- Initialise client ViGEm
- Crée contrôleur Xbox 360 virtuel
- Traduit inputs haut-niveau → XUSB_REPORT
- Gère la connexion/déconnexion

## Gestion des processus

### Launcher (Process principal)
- Interface Qt
- Gestion du contrôleur virtuel
- Scanner de jeux
- **Ne crash jamais** (isolation des jeux)

### Jeux (Processus séparés)
- Lancés via QProcess
- Indépendants du launcher
- Peuvent crasher sans affecter le launcher
- Working directory = dossier du jeu

## Structure des fichiers

```
game_library/
├── CMakeLists.txt           # Configuration build
├── README.md                # Documentation principale
├── VIGEM_INTEGRATION.md     # Guide ViGEm
├── GAMES_STRUCTURE.md       # Structure des jeux
├── build.sh / build.bat     # Scripts de build
│
├── src/
│   ├── main.cpp                        # Point d'entrée
│   ├── MainWindow.h/cpp                # Fenêtre principale
│   ├── GameLibraryTab.h/cpp            # Onglet bibliothèque
│   ├── ControllerTab.h/cpp             # Onglet contrôleur
│   ├── VirtualControllerWindow.h/cpp   # Interface test contrôleur
│   ├── GameScanner.h/cpp               # Scanner de jeux
│   └── GameInfo.h                      # Structure de données
│
└── games/                   # Jeux (créé par l'utilisateur)
    ├── snake/
    │   ├── snake.exe
    │   └── snake.ico
    └── ...
```

## Design Patterns utilisés

### Observer Pattern
- VirtualControllerWindow émet des signaux
- ControllerTab observe ces signaux
- Découplage entre UI et logique

### Strategy Pattern (futur)
- Différentes sources d'input (souris, smartphone, clavier)
- Même interface vers VirtualGamepadManager

### Facade Pattern
- VirtualGamepadManager cache la complexité de ViGEm
- Interface simple : pressButton(), releaseButton()

## Technologies

### Actuellement
- **Qt 6** : Framework GUI
- **C++17** : Langage principal
- **CMake** : Système de build

### Prochaines étapes
- **ViGEmClient** : Contrôleur virtuel
- **Boost.Asio** ou **Qt Network** : Serveur réseau
- **WebSocket/UDP** : Communication smartphone
- **React Native** : App smartphone

## Considérations de sécurité

### Actuel
- Validation des chemins de jeux
- Protection contre les injections de commandes
- Gestion propre des processus

### Futur (avec réseau)
- Authentification smartphone (code PIN, QR)
- Chiffrement des communications
- Validation des inputs réseau
- Rate limiting

## Performance

### Optimisations actuelles
- Scan des jeux une seule fois au démarrage
- Cache des icônes (QIcon)
- Processus séparés pour les jeux

### Optimisations futures
- Pool de connexions réseau
- Bufferisation des inputs contrôleur
- Compression des données réseau

## Tests suggérés

### Tests unitaires
- GameScanner : validation de structure
- GameInfo : intégrité des données

### Tests d'intégration
- Lancement de jeux
- Communication signals/slots
- Gestion erreurs QProcess

### Tests système
- Détection contrôleur dans Windows
- Fonctionnement avec vrais jeux
- Latence inputs smartphone → jeu

## Roadmap

### Phase 1 : MVP (Actuel) ✅
- Interface Qt fonctionnelle
- Scanner de jeux
- Lancement de jeux
- Interface test contrôleur

### Phase 2 : ViGEm (2-3 jours)
- Intégration ViGEmClient
- VirtualGamepadManager
- Tests avec jeux réels

### Phase 3 : Réseau local (1 semaine)
- Serveur C++ dans launcher
- App React Native
- Communication WebSocket
- QR code pour connexion

### Phase 4 : Jeux (4-5 semaines)
- 5 jeux simples (SFML)
- 2 jeux complexes (Raylib/SFML)
- Intégration dans launcher

### Phase 5 : Polish (dernière semaine)
- UI/UX amélioré
- Gestion erreurs complète
- Documentation utilisateur
- Vidéo de démonstration

## Contributions

Ce projet est conçu pour être modulaire :
- Votre collègue : App smartphone + réseau
- Vous : Launcher Qt + jeux
- Collaboration : Intégration ViGEm

Chaque composant peut être développé et testé indépendamment.