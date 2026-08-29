# Changelog

## v1.0.0 — 2026-08-30

- Première version publique GitHub.
- Base firmware issue de la branche v15.
- F1ATB : forçage générique par NumAction, affichage du forçage courant et bouton Annuler.
- Tempo : API RTE directe avec synchronisation NTP.
- Météo : Open-Meteo HTTPS.
- Historique : 24 h conservées, API Web optimisée.
- Paramètres persistants et portail Wi-Fi de secours.

## v15 - F1ATB Annuler hors admin
- Ajout du bouton `Annuler` : envoie `/ForceAction?Force=0&NumAction=X` pour supprimer le forçage et revenir au fonctionnement automatique F1ATB.
- Les trois commandes F1ATB (Allumer 30 min / Éteindre 30 min / Annuler) ne nécessitent plus de session admin.
- NumAction reste entièrement configurable et générique.


## v0.3b-v14 - Forçage F1ATB générique
- Lecture du forçage réel directement dans `ajax_etatActions` (`data[3]` de F1ATB V17.29b).
- Fonctionne avec tout `NumAction` configuré (0 à 15), sans logique spécifique à Relais 1.
- Valeur positive = forçage ON, négative = forçage OFF, zéro = aucun forçage.
- Affichage du temps de forçage courant sous les boutons du dashboard.
- Logs enrichis avec `force=+/-XXmin`.
- Relecture post-commande retardée à 800 ms pour laisser F1ATB appliquer l’état.
# v0.3b-v13 — commandes F1ATB /ForceAction

- Bouton **Allumer 30 min** : `/ForceAction?Force=30&NumAction=N`.
- Bouton **Éteindre 30 min** : `/ForceAction?Force=-30&NumAction=N`.
- Utilise l’endpoint natif `/ForceAction` présent dans le firmware F1ATB V17.29b fourni.
- Log du retour brut de `ForceAction` (tronqué à 240 caractères) pour diagnostiquer le compteur de forçage.
- Relit l’action 150 ms après la commande pour actualiser son état.
- Aucun changement Tempo, météo, Shelly, graphe ou paramètres.

# v0.3b-v11 — F1ATB action parser robuste

- Ne suppose plus un offset fixe dans `/ajax_etatActions`.
- Recherche `NumAction` dans tous les blocs GS/RS.
- Fallback automatique sur `/ajax_etatActionX?NumAction=N` si le format global varie.
- Utilise le nom configuré si l’endpoint spécifique ne fournit pas le titre.
- Log échappé de la réponse F1ATB une seule fois si aucun parseur ne reconnaît l’action.
- Après une commande ON/OFF, relit explicitement l’action au lieu de parser aveuglément la réponse de commande.
- Aucun changement du dashboard, Tempo, météo, Shelly ou historique.

## v0.3b-v10 — F1ATB live sync

- Poll F1ATB indépendant toutes les 5 s (au lieu d'être lié au refresh Tempo 15 min).
- Statut F1ATB En ligne / Synchronisé / Hors ligne sur le dashboard.
- Affichage de l'action réellement détectée, numéro d'action et âge de la dernière synchro.
- Boutons de forçage désactivés si l'action configurée n'est pas synchronisée.
- Logs série F1ATB sur changement d'état, commande et heartbeat 60 s.
- Conserve les correctifs v8 HTTPS/NTP et v9 graphe rapide 24 h.

## v0.3b-v9 - graph fast load

- Historique interne conserve a 30 s / 2880 points.
- `/api/history` limite a ~720 points pour l affichage 24 h, par moyennes de buckets.
- Envoi HTTP groupe en blocs d environ 4 kB au lieu d un `sendContent()` par point.
- Rafraichissement complet du graphe passe de 30 s a 120 s; le statut temps reel reste a 5 s.
- Aucun changement du rendu verrouille ni des corrections Tempo/meteo v8.

## v0.3b-v8 — NTP fiable + parsing HTTPS robuste

- Tempo : attente explicite de la synchronisation NTP avant le premier appel RTE.
- Tempo : retry automatique toutes les 5 s tant que l'heure n'est pas disponible / qu'aucune requête n'a réussi.
- HTTP/HTTPS : lecture du corps complet avant `deserializeJson()` au lieu du parsing direct du stream.
- HTTP/HTTPS : HTTP/1.0 + `Accept-Encoding: identity` pour éviter chunking/compression problématiques sur ESP32.
- Diagnostic : taille du JSON et aperçu du corps en cas d'erreur de parsing.
- Dashboard v5 verrouillé inchangé.

## v0.3b-v7 — Tempo RTE/F1ATB + HTTPS météo

- Tempo basculé sur la même source que F1ATB : `https://www.services-rte.com/cms/open_data/v1/tempoLight`.
- Client HTTPS/TLS dédié via `WiFiClientSecure`.
- Parsing direct des clés datées `YYYY-MM-DD` et couleurs `BLUE/WHITE/RED`.
- Journée Tempo gérée de 06:00 à 06:00 avec conservation sûre du cache avant 06:00.
- J+1 passe à `Non défini` tant que RTE ne l'a pas publié.
- Open-Meteo et le géocodage passent en HTTPS.
- Conservation des corrections v6 de persistance et préremplissage des paramètres.

## v0.3b-v6 — persistance paramètres + démarrage Tempo/Météo

- Page Paramètres chargée depuis un endpoint admin dédié `/api/config` (NVS réelle).
- Ajout du bouton **Enregistrer et redémarrer**.
- Sauvegarde NVS confirmée dans les logs série.
- Affichage des paramètres rechargés au boot (IP/DNS/météo).
- Météo interrogée immédiatement après la connexion Wi‑Fi, sans attendre 15 min.
- Tempo interrogé immédiatement et diagnostics HTTP/DNS explicites.
- Diagnostics Wi‑Fi : IP, passerelle et DNS réellement utilisés.
- Aucun changement du dashboard v5 verrouillé.

# Changelog

## v0.3b — affichage v5 verrouillé

Interface principale verrouillée :
1. Date/heure + Tempo aujourd'hui/demain + météo actuelle/J+1
2. Graphe 24 h
3. Jauges instantanées
4. Bloc batterie détaillé
5. Énergies
6. F1ATB
7. Appareils SolarFlow
8. État système
9. Lien Paramètres en bas

Graphe :
- Production PV en barres jaunes
- Conso maison
- Batterie signée uniquement dans le graphe (+ décharge / - charge)
- Routage
- EDF issu directement du canal réseau Shelly

Jauges :
- PV, maison, routage, EDF, batterie
- 6e jauge automatique si le canal Shelly personnalisé est activé

Batterie :
- Bloc non signé : En charge / En décharge / Au repos
- Puissance affichée en valeur absolue
- SOC moyen et nombre de SolarFlow en ligne

Énergies :
- PV jour + total via compteur logiciel ESP persistant
- Import/export EDF jour + total via Shelly
- Routage jour + total via Shelly
- Canal personnalisé jour + total si activé

Réseau :
- Ajout d'une configuration IP fixe dans Paramètres
- IP, passerelle, masque, DNS1, DNS2
- DHCP conservé lorsque l'option IP fixe est désactivée
- Changement effectif au redémarrage

## v0.3b-v12
- F1ATB: remplace la tuile Tempo F1ATB par les commandes Allumer 30 min / Éteindre 30 min.
- Les boutons F1ATB sont désormais toujours visibles sur le dashboard (plus de masquage CSS hors session admin).
- Boutons disponibles dès que le F1ATB est joignable; retour visuel de commande et message explicite si authentification admin requise.
