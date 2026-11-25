# Diagnostic étendu ESPHome ↔ Flow.io (25/11/2026)

Instrumentation initialement développée sur `diag/tcp-close-wifi-reasons-20261125`,
intégrée à `main` avec **1.3.16-rc2**, puis conservée dans la version
**1.4**. Ces mesures sont diagnostiques :
elles ne changent pas les délais de requête, le mode Silent ni les règles
fondamentales du cache et du bus RTU.

## Collecte unique après une pause

La passerelle fournit désormais :

- `http://IP-PASSERELLE/diag.txt` : **journal de vol indépendant**, 128
  événements rares conservés en PSRAM (~28 KiB), horodatés en heure locale et
  uptime, plus l'intégralité du résumé JSON. Les rafraîchissements des trames
  RTU ne l'effacent pas. La valeur `overwritten` indique les anciens
  événements diagnostiques écrasés si les 128 places sont toutes utilisées.
- `http://IP-PASSERELLE/diag.json` : compteurs cumulés depuis le boot,
  maxima, durées et états instantanés (station ESP-IDF, ESPHome, socket, cache,
  UART, disponibilité mémoire interne et PSRAM).
- `http://IP-PASSERELLE/frames.txt` : capture complète habituelle (256 entrées).
- `http://IP-FLOWIO/` : récupérer les journaux Flow.io. Les builds
  instrumentés pouvaient exposer `[MODBUS-RX] peer=IP:port` ; le code Flow.io
  nettoyé ne conserve plus ce log détaillé par défaut. Utiliser les vrais
  avertissements `MODBUS-ERR`, les compteurs et les événements restants.

**Conseil** : après un blocage apparent de l'écoute des trames, relever
`diag.txt`, `diag.json` et `frames.txt` *sans redémarrer la passerelle*,
puis les logs Flow.io. `/diag.txt` contient déjà les compteurs JSON pour un
envoi réduit à deux fichiers côté passerelle.

## Balises ajoutées (uniquement sur anomalie ou événement réseau)

| Balise | Signification |
| --- | --- |
| `DIAG LOOP GAP` | Intervalle entre deux exécutions `loop()` >250 ms, observé hors de la fonction ; compteurs dès 100 ms |
| `DIAG STAGE SLOW` | Temps excessif dans `web`, `tcp_processing`, `uart_drain`, `rtu_parse_tx`, `cache_schedule` ou `tcp_supervisor` |
| `DIAG UART BACKLOG` | Au moins 64 octets déjà dans le FIFO ou 128 en attente dans le parseur |
| `DIAG UART BUFFERED` | Une trame RTU complète est horodatée en moins du tiers de son temps physique théorique : les octets attendaient probablement en mémoire |
| `DIAG UART TX SLOW` | Envoi UART > durée théorique selon baud/8N1 + 80 ms (distinction des 97 registres normaux à 9600) |
| `DIAG RTU RX GAP` | Silence entre deux lectures d'octets >500 ms ; **ne prouve pas** que la boucle se bloquait, le maître peut aussi être silencieux |
| `DIAG TCP CONNECT` | Handshake TCP >100 ms, y compris succès ou échec et contexte (superviseur, foreground, cache, manuel) |
| `DIAG TCP WAIT` | Réponse TCP attendue >1 s avec TID, FC, adresse, état MBAP/PDU, nombre d'octets déjà reçus |
| `DIAG PREEMPT` | Lecture RTU différée pendant le rafraîchissement TCP de fond **sans fermer le socket** ; les écritures conservent une préemption spécifique |
| `DIAG CACHE SLOW` | Rafraîchissement des valeurs Modbus >1 s |
| `DIAG WEB RENDER` | Construction JSON de `/frames` >50 ms |
| `DIAG WEB EXPORT` | Export `/frames.txt` ou `/diag.txt` >100 ms |
| `DIAG MONITOR GAP` | Plus de 3 s entre deux consultations de `/frames`. Peut venir d'un navigateur fermé, **pas forcément** d'un gel ESPHome |
| `DIAG HEARTBEAT`, `DIAG STAGES` | Résumés périodiques des maxima et compteurs pour ne pas perdre le contexte |
| `DIAG START`, `DIAG CONFIG`, `DIAG REBOOT` | Démarrage de la sonde, configuration active et redémarrage demandé |
| `DIAG RTU LISTEN`, `DIAG FRAME CLEAR` | Bascule d'écoute RTU par l'interface, remise à zéro du journal ordinaire (le journal indépendant est conservé) |

Les événements du correctif précédent sont également copiés dans le journal
indépendant : `TCP OPEN`, `TCP CLOSE` (cause, fd, port local, TID, état du
rafraîchissement), `TCP ERROR`, `TCP RETRY`, `TCP DOWN`, `WIFI STA DOWN`
(raison ESP-IDF, RSSI, BSSID), `WIFI STA ASSOCIATED`,
`WIFI STA IP LOST` et `WIFI STA GOT IP`.

## Lire les compteurs

- `outside_loop_max_us` : délai d'ordonnancement **hors** composant
  ESPHome, à comparer à `loop_max_us`.
- `stages.web.max_us` ou `stages.tcp_supervisor.max_us` : temps maximum
  passé dans une fonction précise de la boucle passerelle.
- `uart_fifo_peak`, `rtu_pending_peak`, `rtu_buffered_frames` : données
  présentes dans l'UART alors que la boucle était occupée.
- `rtu_rx_gap_max_ms` et `rtu_rx_last_age_ms` : pause entre données reçues.
- `frames_poll_gap_max_ms`, `frames_poll_last_age_ms`, `frames_poll_count` :
  distinguer arrêt du navigateur et poursuite des trames UART.
- `tcp_connect_max_us`, `tcp_wait_max_ms`, `cache_refresh_max_ms` :
  délais réseau/caches ; `crc_errors`, `rtu_expired_reads` : qualité Modbus.
- `sta_association_count`, `sta_got_ip_count`, `sta_disconnect_count`,
  `ip_lost_count` : transitions du pilote ESP32/IP. Le drapeau
  `esphome_wifi_flag` est **diagnostique seulement** et ne décide pas de
  fermer la connexion.
- `free_internal`, `free_psram` : pression mémoire.

Les métriques sont cumulées jusqu'au prochain redémarrage. Les Warnings
répétitifs sont limités dans le temps (3 à 10 s selon la balise). Aucun log
nouveau par octet ni par trame RTU normale ; le journal standard
`/frames.txt` conserve son fonctionnement précédent.

## Compiler sous ESPHome

Dans le YAML **réellement installé dans Home Assistant**, pointer vers :

```yaml
external_components:
  - source: github://stephdust/modbus-rtu-ip-gateway@main
    refresh: 0s
    components: [modbus_gateway]
```

Le fichier `my-modbus-gateway.yaml` sur `main` utilise la même référence.
Toutefois, son changement sur GitHub ne modifie pas automatiquement le YAML
déjà installé dans Home Assistant.

Tests dans le dépôt :

```sh
g++ -std=c++17 -Wall -Wextra -Werror -pedantic tests/test_gateway_diagnostic_policy.cpp -o /tmp/gateway_diag && /tmp/gateway_diag
node tests/check_gateway_diag_instrumentation.mjs
node tests/check_tcp_close_diagnostics.mjs
node tests/check_sta_link_source.mjs
```

Ces tests ne remplacent **pas** une compilation ESPHome et un essai sur
l'ESP32-S3. Le profil normal de l'installation demeure inchangé.
