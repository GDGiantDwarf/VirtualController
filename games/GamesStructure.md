# Structure exemple pour tester le launcher

Pour tester le launcher, créez cette structure de dossiers :

```
    games/
    ├── snake/
    │   ├── snake.exe       # Votre jeu Snake
    │   └── snake.ico       # Icône du jeu
    │
    ├── pong/
    │   ├── pong.exe        # Votre jeu Pong
    │   └── pong.ico        # Icône du jeu
    │
    └── breakout/
        ├── breakout.exe    # Votre jeu Breakout
        └── breakout.ico    # Icône du jeu
```


## Note importante

Le launcher cherche spécifiquement :
- Un dossier dans `./games/`
- Un fichier `.exe` avec le MÊME nom que le dossier
- Un fichier `.ico` avec le MÊME nom que le dossier

Exemple valide :
- Dossier : `games/myGame/`
- Exécutable : `games/myGame/myGame.exe`
- Icône : `games/myGame/myGame.ico`

Exemple invalide :
- Dossier : `games/myGame/`
- Exécutable : `games/myGame/game.exe` ❌ (nom différent)
- Icône : `games/myGame/icon.ico` ❌ (nom différent)