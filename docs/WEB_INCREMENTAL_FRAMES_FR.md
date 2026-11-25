# Correctif Web : journal des trames incrémental (ESPHome)

**Version test : 1.3.16-rc2** — branche
`fix/tcp-preempt-psram-web-20261125`, basée sur
`fix/tcp-preempt-psram-web-20261125`.

## Pourquoi

Les diagnostics du 25/11/2026 en `1.3.16-rc1` montrent
`stages.web.max_us=388796` (389 ms), 57 cycles Web lents et une
réponse `/frames` qui prenait généralement 60–100 ms et parfois plus
de 150 ms malgré le stockage des 256 trames en PSRAM. La page refaisait
la récupération des 64 dernières trames toutes les 1,5 secondes.

## Comportement corrigé

- La première requête `GET /frames` renvoie les **64 dernières trames**
  comme auparavant. Les champs historiques `time`, `dir`, `hex`
  sont inchangés.
- Le serveur ajoute `epoch` (identifiant d'instance de journal),
  `seq` (séquence de la dernière entrée) et `reset`.
- Les appels suivants envoient `GET /frames?after=<seq>&epoch=<epoch>`.
  Si le journal n'a pas changé : **aucune trame n'est retransmise**.
  Sinon seules les entrées nouvelles (maximum 64) sont renvoyées.
- Si le navigateur a pris du retard de plus de 64 entrées, si le journal
  a été vidé, si le firmware a redémarré ou si le curseur est invalide,
  le serveur répond `reset=true` avec les 64 entrées courantes.
- La page garde les 64 dernières trames en mémoire dans le navigateur,
  et ne refait le rendu de leur contenu que lorsqu'elles changent.
  Le bouton langue FR/EN reformate le journal sans réinterroger 64 lignes.
- Une requête `/frames` déjà en cours ne se superpose plus à une autre.
  Les onglets masqués cessent de solliciter le journal, et reprennent
  avec le curseur lorsque l'onglet redevient visible.
- Le statut des connexions est rafraîchi à 3 s et le cache à 5 s au lieu
  d'un appel supplémentaire à chaque rafraîchissement des trames.
- Le backend continue à envoyer du JSON par morceaux, avec un seul
  tampon de ligne réutilisé. La réponse HTTP est encore servie par
  `WebServer` synchrone, mais elle devient normalement beaucoup plus
  courte. Pas de promesse d'asynchronisme ni de disparition totale des
  pauses Web sous réseau ralenti.

Exemple :

```
GET /frames
{"frames":[{"time":"...","dir":"RTU RX","hex":"..."}],
 "epoch":2468183991,"seq":1202,"reset":true}

GET /frames?after=1202&epoch=2468183991
{"frames":[],"epoch":2468183991,"seq":1202,"reset":false}
```

## Non-régression

- 256 entrées (tampon fixe en PSRAM) conservées pour `/frames.txt`.
- 64 entrées maximum dans l'affichage Web, traduction et coloration CRC
  conservées.
- `/diag.txt`, `/diag.json`, callbacks ESP-IDF, Modbus RTU/TCP,
  cache SWR, gestion des écritures, garde-fou de délai RTU inchangés.
- Après un effacement `POST /clear`, `epoch` change : les anciens
  navigateurs ne peuvent pas fusionner d'anciennes et de nouvelles trames.
- Après 2^32 événements, un nouvel `epoch` est créé avec le passage
  du compteur de séquence à 1 ; les anciennes vues sont resynchronisées.

## Nouveaux compteurs `/diag.json`

- `frames_full_responses`
- `frames_delta_responses`
- `frames_empty_responses`
- `frames_rows_sent`
- `frames_cursor_resyncs`
- `frames_stream_epoch` et `frames_latest_seq`

À comparer au total `frames_poll_count`, puis observer
`stages.web.max_us` et `stages.web.slow` pendant 5–10 minutes avec
la page ouverte, ainsi que `rtu_buffered_frames`, `rtu_overflow`,
`rtu_expired_reads`, `free_internal` et `free_psram`.

## Compiler avec ESPHome

Dans le YAML **effectivement utilisé** par Home Assistant :

```yaml
external_components:
  - source: github://stephdust/modbus-rtu-ip-gateway@fix/tcp-preempt-psram-web-20261125
    refresh: 0s
    components: [modbus_gateway]
```

La version doit afficher `1.3.16-rc2`. Ne pas fusionner dans `main`
avant compilation ESPHome et essais sur l'ESP32-S3.

## Tests locaux

```sh
g++ -std=c++17 -Wall -Wextra -Werror -pedantic tests/test_frame_delta_policy.cpp -o /tmp/test_frame_delta && /tmp/test_frame_delta
node tests/check_gateway_web_delta.mjs
node tests/check_gateway_preempt_psram_fix.mjs
node tests/check_gateway_diag_instrumentation.mjs
node tests/check_tcp_close_diagnostics.mjs
node tests/check_sta_link_source.mjs
```

Ces tests contrôlent les fonctions natives et les contrats du source, pas
le fonctionnement de WebServer/lwIP ni la compilation ESPHome sur matériel.
