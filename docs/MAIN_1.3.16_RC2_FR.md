# Historique d'intégration — passerelle 1.3.16-rc2 (25/11/2026)

> Document historique : les mesures ci-dessous concernent les essais
> `1.3.16-rc2`. La version actuelle du composant sur `main` est désormais
> **1.4**. Voir la [note de version 1.4](MAIN_1.4_FR.md).

Les cinq commits de la branche `fix/tcp-preempt-psram-web-20261125` ont été
intégrés sur `main` par fast-forward. L'exemple ESPHome cible maintenant :

```yaml
external_components:
  - source: github://stephdust/modbus-rtu-ip-gateway@main
    refresh: 0s
    components: [modbus_gateway]
```

**Attention :** les configurations ESPHome déjà installées dans Home Assistant
ne se mettent pas à jour automatiquement lors d'un commit GitHub. Vérifier
le YAML réellement compilé et recompiler. La version embarquée testée lors de cette intégration était
`1.3.16-rc2`. La version du composant sur `main` est passée ensuite à
**1.4** ; cela ne constitue pas une nouvelle release GitHub ni un nouveau tag.
Pour la reproductibilité, épingler un SHA de commit au lieu de `@main`.

## Correctifs TCP et protocole

- Les lectures RTU FC01–FC04 arrivant pendant un rafraîchissement TCP de fond
  peuvent être différées sans fermeture immédiate du socket persistant.
- Une seule lecture différée est conservée ; nouveau TID correct après le
  rafraîchissement, jamais de confusion avec la réponse TCP précédente.
- La fenêtre RTU reste bornée. Une lecture expirée ou supplantée ne reçoit
  pas de réponse tardive.
- Les écritures gardent leur priorité et leur fermeture explicite du socket
  de fond si nécessaire, sans rejeu automatique.
- Aucun changement du contrôle CRC, du cache SWR, du mode Silent ou des
  réglages de l'installation.

## Journal des trames et diagnostics

- Buffer fixe PSRAM de 256 entrées, sans `String` persistantes pour
  la capture. Export complet `/frames.txt` conservé.
- Affichage Web des 64 dernières entrées avec API incrémentale :
  `/frames?after=<seq>&epoch=<epoch>`.
- Les onglets masqués ne consultent plus le journal de trames en continu ;
  les effacements, redémarrages et dépassements de curseur sont détectés.
- `/diag.json` et `/diag.txt` conservent les métriques TCP, UART,
  Wi-Fi, mémoire, cache et HTTP. Le serveur HTTP est encore synchrone.

## Mesures sur l'ESP32-S3 en 1.3.16-rc2

| Indicateur | Observation (25/11/2026) |
| --- | --- |
| Session TCP persistante | `tcp_connected=true`, session 1, port local 50484 |
| Déconnexions Wi-Fi / pertes d'IP | 0 / 0 |
| Erreurs CRC / débordements RTU | 0 / 0 |
| Réponses RTU envoyées | 757 |
| Consultations `/frames` | 86 |
| Réponses complètes / différentielles | 7 / 79 |
| Lectures RTU expirées | 2 |
| Lectures différées abandonnées | 1 |
| RAM interne libre | 207 712 octets |
| PSRAM libre | 8 137 008 octets |
| Pic du traitement Web | 406 806 µs |

L'interface Web n'est utilisée que ponctuellement et reste fermée environ
**99 % du temps**. Le trafic différentiel est confirmé mais le Web synchrone
peut encore introduire des pauses sous charge. Ces observations sont issues
d'un essai court : la stabilité 12–24 heures, les écritures réelles et
les différents modes TCP/UDP doivent encore faire l'objet de tests dédiés.

## Documentation

- [Préemption TCP et PSRAM](TCP_PREEMPT_PSRAM_FIX_FR.md)
- [Affichage incrémental](WEB_INCREMENTAL_FRAMES_FR.md)
- [Diagnostic indépendant](RTU_TCP_FULL_DIAGNOSTICS_FR.md)
- [Fermetures TCP](TCP_CLOSE_DIAGNOSTICS.md)
- [Installation Home Assistant](INSTALL_HOME_ASSISTANT.md)
