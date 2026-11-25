# Correctif passerelle ESPHome : TCP préemption + PSRAM Web

Correctif développé sur `fix/tcp-preempt-psram-web-20261125` (essai initial
`1.3.16-rc1`), puis intégré à `main` dans **1.3.16-rc2** le 25/11/2026. La
version actuelle du composant est **1.4** (code Modbus identique à la rc2).
La validation longue durée reste à effectuer.

## Cause démontrée dans les logs (25/11/2026)

À 14:34:15.189, la passerelle a fermé volontairement le socket
`192.168.1.72:55121` avec `reason=background_preempted` alors qu'un
rafraîchissement FC03/TID=1 était actif. Flow.io a aussi observé des
`ECONNRESET (104)` sur d'anciens sockets ; sa dernière erreur documentée
était sur le **port 63338**, pas 55121. Ces deux événements ne sont **pas**
une corrélation directe d'un même socket.

## Correctifs

**Lectures FC01–FC04 non présentes dans le cache pendant un rafraîchissement
TCP de fond :** la passerelle met **une seule** demande RTU en réserve dans
une structure bornée (~260 octets). Elle laisse le MBAP/PDU de fond se
terminer ou échouer dans le délai TCP normal, puis attribue un **nouveau TID**
à la demande réservée. Si la lecture RTU est trop ancienne (fenêtre 700 ms
configurée) ou si le maître envoie déjà la suivante, elle est abandonnée
sans émettre de trame tardive. Les autres requêtes restent dans la file UART
horodatée. Aucun ancien octet TCP n'est réinterprété comme réponse à la
nouvelle lecture. Le socket TCP reste ouvert si la transaction de fond
se termine correctement.

**Écritures FC05/06/15/16/23 :** la priorité d'exécution préexistante est
conservée : une écriture qui arrive pendant un rafraîchissement ferme
le socket de fond avec la cause `background_preempted_by_write` et démarre
la nouvelle transaction. Aucun différé arbitraire ni rejeu d'écriture.

**Journal des trames :** les 256 entrées sont stockées dans un tampon fixe
d'environ 212 KiB en PSRAM. Les `String` persistantes des trames sont
supprimées, l'hexadécimal est formaté sans allocation et `/frames` diffuse
les 64 dernières entrées par blocs de 4 KiB au lieu de créer une grosse
chaîne JSON temporaire. `/frames.txt`, `/diag.txt`, `/diag.json`,
l'interface web et les 256 entrées exportées sont conservés. Les guillemets
et antislashs sont échappés correctement dans le JSON. En cas
d'indisponibilité de la PSRAM, seul le journal de trames est désactivé ;
la communication Modbus continue.

## Compteurs à contrôler après installation

`/diag.json` expose les nouveaux champs :
`deferred_foreground_count`, `deferred_foreground_dispatched`,
`deferred_read_expired`, `deferred_max_wait_ms`, `deferred_active`,
`frame_log_psram`, `frame_log_psram_bytes`.

Les nouveaux événements dans `/diag.txt` :
`DIAG PREEMPT` (lecture différée **sans** fermeture),
`DIAG DEFER SEND`, `DIAG DEFER EXPIRED`,
`DIAG PREEMPT WRITE` (fermeture explicite sur écriture).

Une lecture différée qui expire reste un timeout RTU : ce correctif
privilégie l'intégrité des réponses et une connexion TCP stable. Il ne
promet pas la disparition de **tous** les resets TCP (redémarrage,
perte radio ou vrais resets réseaux restent possibles).

## Compilation ESPHome

Dans le YAML **réellement utilisé par Home Assistant** :

```yaml
external_components:
  - source: github://stephdust/modbus-rtu-ip-gateway@main
    refresh: 0s
    components: [modbus_gateway]
```

Le YAML d'exemple sur `main` pointe maintenant vers `@main`. ESPHome doit
indiquer la version **`1.4`** après recompilation. Les essais sur ESP32-S3
de la version précédente `1.3.16-rc2` sont encourageants,
mais un test 12–24 heures avec la page Web fermée reste prévu. Modifier la
configuration YAML réelle dans Home Assistant, puis recompiler.

## Tests hors ESPHome

```sh
g++ -std=c++17 -Wall -Wextra -Werror -pedantic tests/test_gateway_preempt_psram.cpp -o /tmp/gateway_fix && /tmp/gateway_fix
node tests/check_gateway_preempt_psram_fix.mjs
node tests/check_tcp_close_diagnostics.mjs
node tests/check_gateway_diag_instrumentation.mjs
node tests/check_sta_link_source.mjs
```

Ces tests ne remplacent pas la compilation ESPHome et une validation
matérielle sous trafic RTU réel. Le timeout TCP, le cache SWR, le comportement
Silent, les CRC et la logique de Flow.io restent inchangés.
