# Checklist v0.3b

## Build

- [ ] `pio run` termine sans erreur.
- [ ] RAM et flash restent avec une marge confortable.
- [ ] Aucun `undefined reference`.

## Premier démarrage

- [ ] Portail AP accessible.
- [ ] Scan Wi-Fi fonctionnel.
- [ ] Enregistrement SSID + mot de passe Admin fonctionnel.
- [ ] Redémarrage puis connexion au LAN.

## Tempo

- [ ] `/now` correspond au tarif actif.
- [ ] `/today` correspond à la journée Tempo courante.
- [ ] `/tomorrow` peut être `Non défini` après la bascule tant que J+1 n'est pas publié.
- [ ] Timeout réseau sur `/tomorrow` conserve l'ancienne valeur.
- [ ] Réponse valide `tomorrow` non publié donne `UNKNOWN`.
- [ ] 22:00→06:00 + demain Rouge déclenche la précharge.
- [ ] `tomorrow = UNKNOWN` ne déclenche jamais la charge forcée.

## Zendure

- [ ] Découverte et lecture `/properties/report`.
- [ ] Écritures laissées OFF par défaut.
- [ ] Activation volontaire des écritures testée.
- [ ] Reboot pendant un ancien mode forcé : réconciliation AUTO au boot.
- [ ] Appareil hors ligne lors d'un changement de mode : retry après son retour.
- [ ] Répartition de puissance sur les appareils configurés/activés.

## Web / sécurité

- [ ] Client sans contrôle d'écriture.
- [ ] Admin login/logout.
- [ ] F1ATB ON/OFF protégés.
- [ ] Reboot, factory reset et OTA protégés.
- [ ] `/api/reboot` absent.
- [ ] Config/IP privées absentes de `/api/status` non authentifié.

## Mémoire / historique

- [ ] Diagnostic heap/flash affiché dans Admin.
- [ ] `min_free_heap` reste stable après plusieurs heures.
- [ ] `/api/history` streamé sans gros pic de heap.
- [ ] Graphe 24 h fonctionnel avec 2880 points.

## Matériel

- [ ] Test au moins une vraie transition Tempo.
- [ ] Test une nuit avec changement de couleur.
- [ ] Test coupure/reconnexion Wi-Fi.
- [ ] Test Zendure temporairement hors ligne.
