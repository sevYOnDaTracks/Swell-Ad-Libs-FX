<h1 align="center">Swell Ad-lib FX</h1>

<p align="center">
  Le tout-en-un pour les effets créatifs de la voix et des ad-libs — <b>Standalone</b>, <b>VST3</b> (Windows / Mac) et <b>AU</b> (Mac)<br>
  par <b>Swell</b> · <a href="https://github.com/sevYOnDaTracks">@sevyondatracks</a>
</p>

---

## Le principe

Tu choisis un preset, tu touches aux 4 gros macros, et ta voix sonne tout de suite.
Pour les plus curieux, chaque effet de la chaîne se règle en détail : un clic dessus ouvre ses réglages.

## La chaîne d'effets

La voix traverse les 7 effets dans cet ordre. Chaque effet s'allume ou s'éteint indépendamment.

| Effet | Modes | Réglages |
|---|---|---|
| **CLEAN** | High-pass, Low-pass, Band-pass | coupe basses, coupe aigus, de-esser |
| **PUNCH** (compresseur) | Smooth, Punchy, Smash | quantité, attaque, volume |
| **PITCH** | -12, -7, +7, +12 demi-tons, Libre | transposition, mix, réglage fin |
| **COLOR** | Tape, Warm, Fuzz, Crush, Phone, Radio, Megaphone | drive, tone, mix |
| **MOD** | Doubler, Chorus, Wide | profondeur, vitesse, mix |
| **DELAY** | 1/4, 1/8, 1/16, 1/4 pointée, Ping-pong | feedback, tone, mix |
| **REVERB** | Room, Plate, Hall, Dark, Reverse | taille, pre-delay, mix |

## Les macros

| Macro | Action |
|---|---|
| **SPACE** | quantité de delay + reverb |
| **DIRT** | saturation (module COLOR) |
| **PITCH** | transposition de -12 à +12 demi-tons (module PITCH) |
| **WIDTH** | largeur stéréo (0 = mono, 50 % = d'origine, 100 % = très large) |

Toucher un macro allume automatiquement l'effet qu'il pilote.

## Sortie

- **THROW** : le delay ne part que sur la fin des mots (parfait pour les « yeah », « skrt »…).
- **MIX** : dosage voix d'origine / voix traitée.
- **GAIN** : volume de sortie.
- **A / B** : compare deux réglages. **BYPASS** : coupe tous les effets.

## Presets

13 presets d'usine en 4 catégories : **Trap** (Astro Space, Demon Low, Throw It Back), **R&B** (6AM Toronto, Silk Stack, Queen Choir), **Rage** (Rage Scream, Blown Out, Bullhorn), **FX** (Phone Call, Radio Days, Ghost Reverse, Chipmunk).

## Compiler

**Windows** (Visual Studio 2022 Build Tools + CMake) :

```powershell
.\build.ps1
```

**Mac** (Xcode command line tools + CMake) :

```bash
./build-mac.sh
```

Le delay suit le tempo du DAW (120 BPM en standalone).
