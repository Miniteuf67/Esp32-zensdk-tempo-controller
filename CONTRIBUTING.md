# Contribuer

Les contributions sont bienvenues.

Principes du projet :
- fonctionnement autonome sur ESP32 ;
- aucune dépendance Home Assistant ou MQTT obligatoire ;
- API locales privilégiées ;
- aucune donnée personnelle codée en dur ;
- écritures vers les équipements désactivées par défaut ;
- toute nouvelle commande énergétique doit avoir un fail-safe documenté.

Avant une pull request :
1. compiler le projet PlatformIO ;
2. décrire le matériel et firmware testés ;
3. tester sans écritures Zendure puis avec une puissance faible ;
4. ne jamais publier de SN, mots de passe ou IP privées réelles dans les exemples.
