# teams-mirror

Projet Epitech en **C/C++** (build via **Makefile**).

> Ce dépôt ne contenait pas de `README.md` au moment de l’écriture : ce fichier sert de page d’accueil et de guide de prise en main.

## Sommaire

- [À propos](#à-propos)
- [Prérequis](#prérequis)
- [Récupération](#récupération)
- [Compilation](#compilation)
- [Exécution](#exécution)
- [Nettoyage](#nettoyage)
- [Structure du dépôt](#structure-du-dépôt)
- [Débogage](#débogage)
- [Contribuer](#contribuer)

## À propos

`teams-mirror` est un projet en C/C++ destiné à être compilé et exécuté en local.

## Prérequis

- Linux / macOS (Windows via WSL recommandé)
- `make`
- Un compilateur C/C++ (`gcc/g++` ou `clang/clang++`)

Optionnel (recommandé) :
- `gdb` / `lldb`
- `valgrind`

## Récupération

```bash
git clone https://github.com/Loic-ally/teams-mirror.git
cd teams-mirror
```

## Compilation

### Build standard

```bash
make
```

### Rebuild complet

```bash
make re
```

> Si ton Makefile propose d’autres règles (ex: `debug`, `tests_run`…), consulte le `Makefile` ou essaye `make help` si la cible existe.

## Exécution

Le nom du binaire dépend du `Makefile`.

1) Repère le binaire généré (souvent à la racine) :

```bash
ls -la
```

2) Lance-le :

```bash
./<nom_du_binaire>
```

Si le programme accepte une aide :

```bash
./<nom_du_binaire> --help
```

## Nettoyage

```bash
make clean      # supprime les .o et fichiers intermédiaires
make fclean     # supprime aussi le binaire
```

## Structure du dépôt

À compléter selon l’arborescence :

- `src/` : sources
- `include/` : headers
- `tests/` : tests

## Débogage

### gdb

```bash
gdb --args ./<nom_du_binaire> <args>
```

### valgrind

```bash
valgrind --leak-check=full --show-leak-kinds=all ./<nom_du_binaire> <args>
```

## Contribuer

- Une branche par feature/fix
- `make` doit compiler proprement
- Mettre à jour la doc si l’usage change
