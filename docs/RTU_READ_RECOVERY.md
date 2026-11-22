# ESPHome Modbus RTU/IP — lecture RTU bloquée par TCP pendant l'OTA

Version 1.3.15 (22 novembre 2026) — `stephdust/modbus-rtu-ip-gateway`.

## Déclencheur

Lorsqu'une OTA Flow.io ferme le port Modbus TCP 502, la passerelle peut
rester dans une transaction TCP de premier plan avec un timeout
configurable allant jusqu'à 30 secondes. Un seul `connect()`
synchrone peut bloquer la boucle ESPHome. Les requêtes RTU reçues
ensuite s'empilent, puis arrivent au cache bien après leur
fenêtre de réponse (700 ms observés), générant en rafale
`RTU DEADLINE` et des réponses supprimées.

Ce diagnostic concerne uniquement les opérations de lecture en
**client TCP avec cache SWR activé**, pas la disponibilité Wi-Fi ni
le mécanisme OTA de Flow.io.

## Correction conservatrice

1. **Connect borné, lectures SWR seulement.** Le délai d'attente
   d'un nouveau socket TCP pour une FC01/02/03/04 de premier plan
   devient `min(timeout_configuré, clamp(fenêtre_RTU/2, 100, 500) ms)`.
   Exemple : fenêtre RTU 700 ms, timeout TCP 30 s, délai de connect
   **350 ms**. Le timeout des écritures et des autres modes est intact.
   Le rafraîchissement cache en arrière-plan conserve son timeout de
   connexion à 250 ms.
2. **Lecture de premier plan devenue trop vieille.** Si le délai RTU
   expire pendant l'attente du MBAP ou de la PDU TCP, la lecture
   déjà envoyée est *convertie* en rafraîchissement de cache :
   réponse TCP validée enregistrée dans le cache, **aucune seconde
   réponse sur RS485**, socket TCP préservée. Les lectures RTU
   suivantes peuvent être satisfaites immédiatement depuis le cache.
   Si un cache miss prioritaire arrive, la logique existante annule
   ce rafraîchissement, sans mélange de transaction.
3. **Tampon RTU.** Jusqu'à 16 trames de lecture **complètes**, CRC
   valide, adresse 1–247, fonction 01–04, déjà hors délai, sont
   supprimées depuis la tête de file par passage de boucle. Les
   octets/timestamps restent alignés et chaque trame reste visible
   en `RTU RX` dans l'export TXT. Impossible d'enjamber une trame
   écriture, une trame CRC invalide ou incomplète. La voie ordinaire
   dispose également d'un garde-fou contre les lectures expirées.
4. **Aucune écriture touchée.** FC05/06/15/16/23 et les autres
   fonctions non lecture conservent leur traitement et les
   protections anti-rejeu existantes. Les modes non-SWR restent
   inchangés.
5. **Mesures.** Dans `/connections`,
   `rtu_expired_queued_reads`, `rtu_expired_foreground_reads`
   et `rtu_pending_bytes`. Dans `/cache-stats`,
   `expired_queued_reads` et `expired_tcp_reads`.
   Les événements `RTU EXPIRED` des lectures en file sont
   agrégés à un maximum d'un événement par cinq secondes.

## Interface Web : protection 1.3.14

**Ne pas revenir** au générateur `String page_()` : il avait
tronqué la page autour de 32 Ko. La version 1.3.15 conserve
strictement l'intégralité de l'HTML statique (68 521 caractères,
29 segments), transmise en blocs HTTP de 1 Ko. Aucun champ,
style, script ou route d'interface n'est modifié par ce patch.

## Limites et tests

Le connect TCP reste une API synchrone : un timeout demandé n'est
pas une garantie stricte de durée de blocage sous toutes les pannes
réseau. Les octets encore présents dans le FIFO UART à la sortie
du blocage sont horodatés lors de leur lecture et non forcément à
leur arrivée physique. Le garde-fou des réponses RTU tardives reste
actif. Les connexions lors des écritures utilisent toujours le timeout
réseau configuré.

Tests de politique pur C++ :
```sh
g++ -std=c++11 -Wall -Wextra -Werror -pedantic \
  -Icomponents/modbus_gateway tests/test_gateway_policy.cpp \
  -o /tmp/test_gateway_policy && /tmp/test_gateway_policy
node tests/check_rtu_read_recovery.mjs
node tests/check_webui_html_stream.mjs
```

Validation matérielle : démarrer Flow.io avec 502 ouvert et un maître RTU
qui lit les registres 52/97, 500/28, 1000/3 ; fermer ensuite le port 502
durant une OTA ; poursuivre les lectures ; vérifier un délai de
connexion limité et l'absence de réponse RTU hors fenêtre ; rouvrir
502 et vérifier le cache et les réponses après reconnexion. Vérifier
en parallèle l'interface Web complète et les exports TXT.

Ni compilation complète ESPHome ni test ESP32-S3 ne sont implicites
dans les contrôles statiques/host.
