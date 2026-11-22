# Modbus RTU/IP — lecture expirée et désengorgement après attente TCP

Date : 22 novembre 2026
Portée : passerelle `stephdust/modbus-rtu-ip-gateway`, mode
**CLIENT TCP avec cache SWR activé**.

## Symptôme observé

Pendant des mises à jour OTA de Flow.io, le serveur TCP port 502 peut
cesser d'écouter. Dans la capture du 22 novembre vers 16:59:48, la
passerelle a ignoré environ 31 réponses RTU dont la fenêtre de 700 ms
était dépassée (jusqu'à 15,6 s), bien que les données fussent en cache.
Le maître RTU continue de demander les mêmes trois blocs pendant que
la transaction TCP de premier plan empêche `parse_rtu_()` de les
servir. Au déblocage, le retard se manifeste par une rafale de
`CACHE HIT` + `RTU DEADLINE`.

Il ne faut pas « sauver » ces réponses tardives : la transmission
série (pouvant dépasser 200 ms pour 97 registres à 9600 bauds)
risquerait de heurter la requête Modbus suivante.

## Correctif ciblé (lectures uniquement)

- Pendant une attente TCP, avant `parse_rtu_()`, vérifier la tête
  des trames RTU déjà bufferisées. Si elle forme une trame **complète,
  CRC correct, Unit ID 1–247, FC01/02/03/04**, et si son dernier
  octet a été observé il y a au moins la fenêtre RTU configurée,
  la retirer sans émission RTU ni transfert TCP. Garder l'original
  dans le log `RTU RX`. Ne jamais sauter une trame plus récente ou
  une écriture : ordre strictement conservé.
- Nettoyer au maximum **16 trames** par passage de la boucle ESPHome.
  Les timestamps restent alignés avec les octets reçus. Si
  l'alignement n'est pas garanti, ne rien supprimer.
- En mode SWR, une **lecture TCP de premier plan** (FC01–04)
  qui a déjà dépassé la fenêtre RTU est abandonnée, avec fermeture
  de la socket pour ignorer sa réponse tardive. Cela libère la
  file sans réémettre une lecture annulée et sans générer de
  réponse RTU ambiguë.
- Les **écritures FC05, FC06, FC15, FC16, FC23** ne sont pas
  annulées par ce nouveau garde-fou, pas plus que les rafraîchissements
  de cache en arrière-plan. Le code des écritures, le serveur TCP/UDP,
  les délais configurés et la validation CRC/PDU existante restent
  inchangés.
- La règle est **inactive sans cache SWR client** : aucun changement
  pour les modes hérités.
- Le compteur cumulatif `rtu_expired_queued_reads` mesure les
  lectures retirées du buffer et `rtu_expired_foreground_reads`
  mesure les transactions TCP de premier plan annulées.
  `rtu_pending_bytes` expose l'occupation RX dans `/connections`.
  `RTU EXPIRED` produit une synthèse limitée à 5 s pour les purges.

## Limites connues

Le timestamp UART représente le moment où ESPHome récupère
l'octet, pas nécessairement celui de son arrivée physique dans le
FIFO si le code a été bloqué par un `connect()` synchrone.
La règle est donc **conservatrice** : elle ne garantit pas une limite
de bout en bout à 700 ms en présence d'un `connect()` bloquant.
Le timeout TCP de 3 secondes et le mécanisme de reconnexion sont
inchangés. Ne pas confondre ce désengorgement avec une réparation
du réseau Wi-Fi ou du serveur Flow.io.

Les trames de lecture déjà expirées sont ignorées (timeout pour le
maître), ce qui est cohérent avec la protection anti-réponse tardive
préexistante. Le maître doit continuer à interroger les registres.

## Tests

```sh
g++ -std=c++11 -Wall -Wextra -Werror -pedantic \
  -Icomponents/modbus_gateway tests/test_gateway_policy.cpp \
  -o /tmp/test_gateway_policy
/tmp/test_gateway_policy
node tests/check_rtu_expiry_source.mjs
```

Après compilation ESPHome sur le matériel :

1. Vérifier la réception des lectures normales FC03 de 52/97, 500/28
   et 1000/3 ; toutes doivent recevoir leur réponse dans le délai.
2. Arrêter temporairement le listener Modbus TCP de Flow.io pendant
   que le maître RTU continue à interroger la passerelle.
3. Confirmer les messages `RTU EXPIRED`, la progression de
   `rtu_expired_queued_reads` ou `rtu_expired_foreground_reads`
   et l'absence de réponse série tardive.
4. Redémarrer le listener 502 : les connexions et la mise à jour
   du cache doivent reprendre. Aucun paquet d'écriture ne doit être
   rejoué automatiquement.
5. Si possible, tester une écriture contrôlée dans un environnement
   de test, jamais sur les actionneurs en production pour valider ce
   changement. Vérifier CRC, ordre, trames partielles et millis rollover.

Le patch n'a **aucun lien** avec le moteur OTA de Flow.io et ne
le modifie pas.
