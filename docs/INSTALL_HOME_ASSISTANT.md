# Installation et compilation avec Home Assistant / ESPHome

**Objectif : installer la passerelle Modbus RTU ↔ TCP/UDP sur un ESP32-S3 directement avec ESPHome Device Builder dans Home Assistant, sans cloner le dépôt GitHub et sans copier les composants C++ dans Home Assistant.**

Ce guide utilise le fichier réel [`my-modbus-gateway.yaml`](../my-modbus-gateway.yaml), la configuration des secrets décrite dans [`secrets.yaml.example`](../secrets.yaml.example) et la release [v1.3.2](https://github.com/stephdust/my-modbus-rtu-ip-gateway/releases/tag/v1.3.2).

## Prérequis

- Home Assistant **OS ou Supervised**, disposant des applications/add-ons, et l'application officielle **ESPHome Device Builder** (autrefois appelée ESPHome Dashboard).
- Une carte **Waveshare ESP32-S3-RS485-CAN** ou un matériel ESP32-S3 RS485 équivalent. Le YAML fourni vise la Waveshare avec **16 Mo de flash et 8 Mo de PSRAM octale**, ESPHome en **framework Arduino**.
- Un réseau Wi-Fi **2,4 GHz**, et un accès Internet **depuis l'hôte Home Assistant** au moment de compiler pour télécharger le composant GitHub et les outils de compilation.
- Un câble USB de **données** et, pour la première programmation, un ordinateur avec Chrome/Edge et WebSerial, ou une liaison USB directement accessible au serveur Home Assistant. Pour une passerelle déjà équipée d'ESPHome et joignable, la mise à jour peut être faite par OTA.

L'intégration **ESPHome** dans *Appareils et services* de Home Assistant et l'application **ESPHome Device Builder** sont différentes : **c'est le Device Builder qui édite le YAML et compile le firmware**. Home Assistant Container sans gestionnaire d'applications ne peut pas suivre exactement ces étapes ; utiliser dans ce cas un ESPHome Device Builder indépendant.

## 1. Installer et ouvrir ESPHome dans Home Assistant

1. Ouvrir **Home Assistant → Paramètres → Applications** (ou *Modules complémentaires / Add-ons* selon la version), puis installer **ESPHome Device Builder** si nécessaire.
2. Démarrer l'application et ouvrir **ESPHome Device Builder** depuis Home Assistant.
3. Cliquer sur **New Device / Nouvel appareil** et créer un appareil, par exemple `my-modbus-gateway`. Si l'assistant propose de programmer immédiatement le matériel, reporter cette étape : nous devons d'abord remplacer sa configuration par le YAML du projet.
4. Ouvrir **Edit / Modifier** sur l'appareil créé.

Les intitulés exacts des boutons peuvent varier selon la version du Device Builder.

## 2. Copier le seul YAML du projet

1. Ouvrir le [YAML du projet sur GitHub](https://github.com/stephdust/my-modbus-rtu-ip-gateway/blob/main/my-modbus-gateway.yaml), puis **Raw / Brut** pour récupérer tout le contenu.
2. Dans **Edit / Modifier** de l'appareil ESPHome, **remplacer entièrement** le contenu généré par l'assistant avec ce YAML (ne pas concaténer deux blocs `esphome:`, `wifi:` ou `api:`).
3. En haut du fichier, adapter au besoin :

   ```yaml
   substitutions:
     name: my-modbus-gateway

   esphome:
     name: ${name}
     friendly_name: My Modbus Gateway
   ```

   Le nom ESPHome doit être unique sur le réseau. Le fichier du Device Builder et la valeur `substitutions.name` peuvent être harmonisés pour éviter les confusions. Conserver le `board: esp32-s3-devkitc1-n16r8`, le `framework: type: arduino` et la configuration `psram:` du modèle fourni **pour la carte Waveshare de référence** ; ne les changer qu'en cas de matériel différent.

4. Dans `external_components:`, préférer **la version stable v1.3.2** :

   ```yaml
   external_components:
     - source: github://stephdust/my-modbus-rtu-ip-gateway@v1.3.2
       components: [modbus_gateway]
   ```

   Cette section **remplace** le bloc `external_components:` fourni dans le YAML (qui pointe vers `@main`). Ne pas conserver deux blocs `external_components:` parallèles.

   Pour tester les futures modifications, utiliser `@main` et éventuellement `refresh: 0s` à la place de `@v1.3.2`. La release fixe le code source à une version reproductible ; `main` évolue.

**Important :** conserver dans le YAML l'entrée suivante :

```yaml
modbus_gateway:
  id: gateway
```

ESPHome **récupère automatiquement** `components/modbus_gateway/` depuis le dépôt GitHub pendant la validation/compilation et utilise ce composant externe. **Ne pas copier** `components/`, `*.h`, `*.cpp`, les images ou l'intégralité du dépôt dans `/config/esphome/`. Le fichier YAML est suffisant côté configuration, avec ses secrets.

## 3. Renseigner `secrets.yaml`

Le YAML du projet contient exactement ces références :

```yaml
wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

api:
  encryption:
    key: !secret api_key
```

Dans **ESPHome Device Builder**, ouvrir l'éditeur des **Secrets** (ou utiliser un éditeur de fichiers Home Assistant) et compléter le fichier **`/config/esphome/secrets.yaml`**, situé à côté des configurations ESPHome :

```yaml
wifi_ssid: "MON_WIFI_2_4_GHZ"
wifi_password: "MON_MOT_DE_PASSE_WIFI"
api_key: "MA_CLE_API_ESPHOME_BASE64"
```

- Remplacer les trois valeurs. `api_key` doit être une **véritable clé de chiffrement API ESPHome**, soit 32 octets encodés en Base64, **pas** le texte d'exemple. On peut réutiliser une clé API générée par le Device Builder lors de la création de l'appareil, ou en générer une nouvelle et la conserver.
- Si `secrets.yaml` existe déjà, **ajouter uniquement les clés manquantes** ou réutiliser les entrées existantes ; ne pas écraser les secrets des autres appareils.
- Le `secrets.yaml` principal de Home Assistant (`/config/secrets.yaml`) **n'est pas automatiquement le même fichier** que celui d'ESPHome (`/config/esphome/secrets.yaml`). Pour mutualiser volontairement les secrets, la [FAQ ESPHome](https://esphome.io/guides/faq/#how-do-i-use-my-home-assistant-secretsyaml-file) documente l'inclusion du fichier parent.
- **Ne jamais publier** les vraies valeurs Wi-Fi/API dans GitHub, un ticket ou une capture d'écran.

**À modifier pour la sécurité :** dans le YAML public, le point d'accès de secours `Modbus-Gateway-Setup` utilise un mot de passe d'exemple `modbus1234`. Le remplacer par un mot de passe privé robuste avant de flasher.

## 4. Vérifier et compiler directement dans Home Assistant

1. **Enregistrer** le YAML dans Device Builder.
2. Si le bouton est disponible, choisir **Validate / Valider** : ESPHome doit accepter la configuration et récupérer le composant externe via GitHub.
3. Cliquer sur **Install / Installer** (dans la fiche ou le menu de l'appareil) et choisir la méthode adaptée :
   - **Première installation, USB sur l'ordinateur où le navigateur est ouvert** : choisir l'installation via navigateur / **Web Serial**, si proposée. Utiliser Chrome/Edge et un câble USB de données.
   - **USB connecté directement à l'hôte Home Assistant** : choisir l'installation par connexion USB côté serveur, si le port est détecté.
   - **Compiler sans flasher immédiatement** : choisir **Manual download / Téléchargement manuel** et récupérer le firmware. Il pourra être installé ensuite avec [ESPHome Web](https://web.esphome.io/) sur un ordinateur relié en USB.
   - **Firmware ESPHome déjà installé et accessible en Wi-Fi** : choisir **Wirelessly / OTA / Sans fil**. Vérifier que les paramètres d'authentification et la configuration OTA correspondent à l'appareil déjà installé.

L'application ESPHome s'occupe de la compilation sur Home Assistant, de PlatformIO et du téléchargement des composants. **Pas besoin de lancer PlatformIO ni d'installer Python/ESPHome localement sur un PC**.

En cas de problème de compilation après un changement de branche ou de version, essayer **Clean Build Files** dans le menu de l'appareil, puis relancer la compilation. Si le téléchargement GitHub échoue, vérifier Internet/DNS depuis le serveur Home Assistant.

## 5. Après le premier démarrage

Après connexion de l'ESP32 au Wi-Fi, ouvrir :

| Service | Adresse | Rôle |
|---|---|---|
| **Interface de la passerelle Modbus** | `http://<IP_ESP32>/` (port **80**) | TCP/UDP, RS485, GPIO, cache et logs de trames |
| **Diagnostics ESPHome** | `http://<IP_ESP32>:8080/` | Capteurs réseau, état, mémoire et maintenance |
| **Serveur Modbus TCP distant** | Par défaut `192.168.1.100:502` | À remplacer dans **l'interface Web de la passerelle** selon votre serveur |

Le capteur texte **Interface web** du YAML affiche également l'URL du port 80. La page ESPHome sur 8080 utilise ses ressources intégrées : **aucun JavaScript personnalisé ou CDN externe n'est nécessaire**.

Les paramètres IP du serveur Modbus, baudrate, parité, ports et fonctionnement du cache se règlent ensuite **sur l'interface Web du port 80** et sont enregistrés en NVS. Ils ne nécessitent pas de modifier le YAML pour chaque ajustement. Le nouveau cache **avec rafraîchissement en arrière-plan** de v1.3.2 reste **désactivé par défaut** : l'activer uniquement si son ancienneté maximale des données est compatible avec les besoins de l'installation. Voir [Documentation du cache](CACHE.md).

L'intégration Home Assistant/ESPHome peut être ajoutée pour superviser l'appareil, mais elle **n'est pas requise pour faire fonctionner la passerelle Modbus**.

## Aide rapide

| Symptôme | À vérifier |
|---|---|
| `Secret 'wifi_ssid' not defined` | Ajouter la clé dans `/config/esphome/secrets.yaml` ; même chose pour `wifi_password` et `api_key` |
| Erreur liée à `api.encryption.key` | Utiliser une vraie clé ESPHome Base64 de 32 octets |
| `Component modbus_gateway not found` | Vérifier le bloc `external_components`, l'accès GitHub du serveur et la référence `@v1.3.2` |
| Erreur de carte/framework | Conserver la configuration ESP32-S3/Arduino/PSRAM du YAML d'origine sur la Waveshare |
| Aucun port USB trouvé | Câble USB de données, port BOOT/RESET si nécessaire, ou téléchargement manuel puis ESPHome Web |
| Passerelle joignable, pas de réponses Modbus | Vérifier les réglages du serveur TCP/UDP et du RS485 sur le **port 80**, puis exporter les trames |
| Première compilation lente ou échouée | Vérifier l'espace disque, la mémoire et l'accès Internet du serveur Home Assistant ; utiliser **Clean Build Files** si nécessaire |

## Sources et références

- [YAML complet du projet](../my-modbus-gateway.yaml) et [exemple de secrets](../secrets.yaml.example)
- [Release officielle v1.3.2](https://github.com/stephdust/my-modbus-rtu-ip-gateway/releases/tag/v1.3.2)
- [Installer ESPHome Device Builder dans Home Assistant (documentation officielle)](https://esphome.io/install/)
- [Composants externes ESPHome depuis GitHub](https://esphome.io/components/external_components/)
- [Guide général / comportement de la passerelle](QUICK_START.md) et [cache Modbus](CACHE.md)
