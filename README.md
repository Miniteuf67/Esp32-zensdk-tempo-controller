# Zendure Tempo Controller

**Micro-HEMS autonome et ultra-léger sur ESP32**, pensé pour superviser et piloter une installation photovoltaïque résidentielle sans Home Assistant, Raspberry Pi, Docker ni cloud obligatoire.

Le contrôleur agrège localement les données **Zendure SolarFlow**, **Shelly Pro 3EM**, **F1ATB**, **Tempo RTE** et **Open-Meteo** dans une interface Web embarquée avec historique 24 h et commandes locales.

> Version publique initiale : **v1.0.0**

## Pourquoi ce projet ?

L'objectif est simple : disposer d'un petit contrôleur énergétique dédié, autonome et maintenable, qui continue de fonctionner même si Home Assistant ou le serveur domestique sont arrêtés.

## Fonctionnalités

- ESP32 autonome avec interface Web locale responsive ;
- supervision de plusieurs appareils Zendure SolarFlow sur le LAN ;
- lecture Shelly Pro 3EM ;
- intégration F1ATB avec détection d'action configurable ;
- commandes F1ATB **Allumer 30 min**, **Éteindre 30 min** et **Annuler** ;
- lecture du forçage F1ATB courant pour l'action configurée ;
- Tempo directement depuis l'API publique RTE utilisée par F1ATB ;
- gestion de la journée Tempo de **06:00 à 06:00** ;
- météo via Open-Meteo en HTTPS ;
- historique 24 h en RAM ;
- graphique Web optimisé pour un chargement rapide ;
- configuration persistante en NVS/Preferences ;
- DHCP ou IP fixe, passerelle et DNS configurables ;
- portail Wi-Fi de secours **Zendure-Tempo-Setup** si le réseau configuré est indisponible ;
- mDNS ;
- interface Admin pour les réglages système ;
- mise à jour OTA Web par fichier `.bin` ;
- aucun historique périodique écrit en flash.

## Architecture

```text
                ┌────────────────────┐
RTE Tempo ─────►│                    │
Open-Meteo ────►│                    │
Shelly 3EM ────►│       ESP32        │◄────► Zendure SolarFlow
F1ATB ─────────►│                    │
                │  Web UI + moteur   │
                └─────────┬──────────┘
                          │
                     Navigateur
```

Tout le fonctionnement principal est embarqué dans l'ESP32.

## Matériel / services actuellement ciblés

- ESP32 compatible `esp32dev` ;
- Zendure SolarFlow compatibles avec l'API LAN utilisée par le projet ;
- Shelly Pro 3EM ;
- routeur solaire F1ATB ;
- connexion Internet uniquement pour RTE Tempo et Open-Meteo.

Les équipements locaux restent accessibles et supervisables sur le LAN indépendamment de Home Assistant.

## F1ATB

Le numéro d'action est configurable : aucune logique n'est codée en dur pour un triac ou un relais précis.

Exemples :

```text
NumAction=0  → action 0
NumAction=1  → action 1
NumAction=2  → action 2
...
```

Les commandes utilisent l'endpoint natif F1ATB :

```text
/ForceAction?Force=30&NumAction=X    # Allumer 30 min
/ForceAction?Force=-30&NumAction=X   # Éteindre 30 min
/ForceAction?Force=0&NumAction=X     # Annuler / retour auto
```

Le statut de liaison, l'action détectée, son état et le forçage courant sont rafraîchis indépendamment du cycle Tempo.

## Tempo RTE

Source :

```text
https://www.services-rte.com/cms/open_data/v1/tempoLight
```

Le contrôleur synchronise son heure via NTP avant de traiter les dates Tempo. La logique respecte le basculement de journée à 06:00.

## Météo

Open-Meteo est interrogé directement en HTTPS. La localisation est configurable depuis l'interface.

## Historique 24 h

Le moteur conserve :

- **2880 échantillons** maximum ;
- **1 échantillon toutes les 30 s** ;
- soit **24 h** d'historique en RAM.

Pour éviter de saturer le navigateur et l'ESP32, l'API Web sert une représentation décimée d'environ **720 points sur les mêmes 24 h**. La durée d'historique n'est donc pas réduite.

## Premier démarrage

Si aucun Wi-Fi n'est configuré ou si la connexion échoue, l'ESP32 lance automatiquement :

```text
Zendure-Tempo-Setup
```

Depuis le portail de configuration, renseignez :

1. le réseau Wi-Fi ;
2. le mot de passe Wi-Fi ;
3. le mot de passe administrateur ;
4. les équipements locaux.

Les paramètres sont ensuite conservés en NVS.

## Compilation avec PlatformIO

Prérequis : VS Code + PlatformIO, ou PlatformIO CLI.

```bash
pio run
```

Flash USB :

```bash
pio run -t upload
```

Moniteur série :

```bash
pio device monitor -b 115200
```

### Flash factory

Pour repartir complètement à zéro :

```bash
pio run -t erase
pio run -t upload
```

Cela efface notamment les paramètres NVS enregistrés.

## Structure

```text
include/          Headers et modèles
src/              Firmware
platformio.ini    Configuration PlatformIO
CHANGELOG.md      Historique des versions
SECURITY.md       Politique de sécurité
CONTRIBUTING.md   Contributions
LICENSE           Licence MIT
```

## Sécurité

Aucun SSID, mot de passe Wi-Fi, mot de passe Admin, token API ou numéro de série personnel n'est fourni dans le dépôt.

L'interface de configuration sensible reste protégée par une session Admin. Les commandes F1ATB de forçage sont volontairement disponibles sur le dashboard local.

Ce projet est destiné à un réseau local de confiance. N'exposez pas directement l'ESP32 sur Internet.

## État du projet

Le firmware est fonctionnel sur l'installation ayant servi au développement, mais reste un projet communautaire en évolution. Testez toute nouvelle version sur votre propre matériel avant de lui confier une automatisation critique.

## Licence

MIT — voir [LICENSE](LICENSE).

## Marques

Zendure, Shelly, F1ATB, RTE et Open-Meteo appartiennent à leurs propriétaires respectifs. Ce projet communautaire n'est pas affilié ni sponsorisé par ces entités.
