# Version 1.4 — Passerelle Modbus RTU ↔ TCP/IP
**Intégration GitHub : 25 novembre 2026 — branche `main`**

## Objet de la version

La version du composant ESPHome passe de `1.3.16-rc2` à **`1.4`**.
Il s'agit d'une consolidation et d'un changement de numéro de version :
**aucun nouvel algorithme Modbus ni aucun nouveau correctif n'a été ajouté
dans le commit de version 1.4**. Les fonctionnalités avaient été intégrées
sur `main` avec le code précédemment testé en `1.3.16-rc2`.

La mise à jour du code source sur GitHub ne constitue ni une release
GitHub avec tag, ni le flash automatique d'une passerelle existante.

## Corrections incluses

### Connexion TCP persistante

- Réception d'une lecture RTU FC01–04 pendant un rafraîchissement TCP
  de fond : une lecture peut être différée sans fermer systématiquement
  la connexion persistante.
- Conservation d'une seule lecture différée, corrélée à un nouvel
  identifiant de transaction TCP.
- Protection contre les réponses RTU tardives (délai borné) et contre
  le rejeu involontaire des écritures.
- Maintien de la priorité des écritures lorsque le cache SWR fonctionne
  avec des rafraîchissements en arrière-plan.

### PSRAM et journalisation

- Journal circulaire de **256 entrées en PSRAM** avec stockage fixe.
- Export TXT intégral disponible sur `/frames.txt`.
- Diagnostic indépendant sur `/diag.txt` et compteurs sur `/diag.json`.
- Les erreurs CRC restent filtrées, sans transfert ni mise en cache
  des trames corrompues.

### Interface Web (outil de diagnostic ponctuel)

- Affichage de 64 entrées et réponses incrémentales avec
  `/frames?after=<seq>&epoch=<epoch>`.
- Synchronisation du curseur après réinitialisation du journal
  ou redémarrage.
- Pas de consultation périodique du journal lorsque l'onglet est
  masqué ; pas de superposition de requêtes déjà en cours.
- L'interface reste **synchrone** et peut encore occasionner des
  pauses de traitement pendant les exports et gros rafraîchissements.
  Elle est utilisée ponctuellement, pas comme superviseur permanent.

## Validation et limites

Les relevés expérimentaux du **25/11/2026**, collectés sur la version
**1.3.16-rc2**, ont montré une connexion TCP persistante, zéro perte
de connexion Wi-Fi ou d'adresse IP, zéro erreur CRC et zéro débordement
UART durant la période observée. Sur 86 requêtes `/frames`,
79 réponses étaient incrémentales. Ces résultats ne constituent pas
une preuve de stabilité sur 12–24 heures.

Le passage en `1.4` est uniquement un changement de version et de
documentation ; il ne constitue pas une nouvelle compilation ni un
nouveau test sur matériel.

## Installer ou mettre à jour depuis Home Assistant / ESPHome

Dans le **YAML réellement utilisé** par ESPHome Device Builder :

```yaml
external_components:
  - source: github://stephdust/modbus-rtu-ip-gateway@main
    refresh: 0s
    components: [modbus_gateway]
```

Puis recompiler et installer le firmware. La version de composant
affichée dans l'interface Web sera **1.4** une fois ce code installé.
Un commit GitHub ne remplace pas automatiquement le code embarqué.
Pour figer une version reproductible, utiliser un SHA de commit à la
place de `@main`.

## Références

- [Journal complet des modifications](../CHANGELOG.md)
- [Observation des essais en 1.3.16-rc2](MAIN_1.3.16_RC2_FR.md)
- [Préemption TCP et utilisation de la PSRAM](TCP_PREEMPT_PSRAM_FIX_FR.md)
- [Journal des trames Web incrémental](WEB_INCREMENTAL_FRAMES_FR.md)
- [Installation via Home Assistant](INSTALL_HOME_ASSISTANT.md)

**Statut :** code intégré sur `main` ; aucune release ou tag 1.4
créé à cette étape. Les essais de stabilité sur une longue durée
et les tests complets des différents modes TCP/UDP restent à effectuer.
