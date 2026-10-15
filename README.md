# BX-SYNC

Aims to keep [Bexio](https://bexio.com) data sync with a local database. You
can backup your Bexio data in your own premise and be ready to jump away if 
they end up like Winbiz (which I am more and more confident that they will).

## Features

 - Synchronise many endpoints (using Grok to add more endpoint quickly)
 - Follow rate limiting of the API
 - Fallback in case Bexio break rate limiting again (like in july 2026).
 - Continuous, you are backing up in near real time
 - Use MySQL to store in a relationnal fashion with foreign key and all

`make test` checks invoice JSON decoding and chain resume without MySQL or the network.

## Configuration

`mysql-host`, `mysql-user`, `mysql-password` and `mysql-database` stay in the
JSON file. Every other setting can live in the `_configuration` table. A row
overrides the same name in the file. Names starting with `mysql-` are ignored.
The table is created on startup when it is missing.

```sql
INSERT INTO _configuration (name, type, cval)
  VALUES ('bexio-endpoint', 'string', 'https://api.bexio.com/');
INSERT INTO _configuration (name, type, ival)
  VALUES ('log-level', 'integer', 255);
INSERT INTO _configuration (name, type, ival)
  VALUES ('prune-drift', 'integer', 10);
INSERT INTO _configuration (name, type, fval)
  VALUES ('some-ratio', 'float', 1.5);
```

`type` is `string` (`cval`), `integer` (`ival`), `bool` (`ival`, 0 or 1),
`float` (`fval`, stored as DOUBLE) or `blob` (`bval`). The file is read first,
then the table, before logging, daemon setup and Bexio login. OAuth tokens
live in the same table: `oauth-access-token`, `oauth-refresh-token` (`string`)
and `oauth-expires-at` (`integer`). `--login` and each refresh write those
rows in one transaction. bx-sync and bx-proxy share them. A refresh takes
the MariaDB named lock `bx-oauth-refresh` for up to 20 seconds, re-reads the
rows, and posts only when the stored access token still needs replacing.
The process that finds a token stored by the other one logs
`OAuth access token was refreshed by the other process`.

## Authentication

Bexio access is OAuth 2.0 authorization code, through liboauth2. Put
`oauth-client-id`, `oauth-client-secret`, and `oauth-redirect-uri` in the
config (see `conf.example.json`). Register that redirect URI on the Bexio app.
The default is `http://127.0.0.1:8765/callback`.

The sync runs on a server without a desktop. Log in once over SSH, as a user
that can write `_configuration`:

```
bxsync --login /usr/local/etc/bxsync.json
```

The command prints an authorize URL. On your laptop, forward the callback port
and open the URL in a browser:

```
ssh -L 8765:127.0.0.1:8765 user@the-server
```

If the browser cannot reach the forwarded port, copy the full address-bar URL
(it still contains `?code=...`) and paste it into the SSH session. Tokens are
written to `_configuration`. After that, the daemon only refreshes them.
`--login` is not a systemd command. A rejected refresh is logged
(`Refresh token was rejected. Run: bxsync --login <config>`) and the
process keeps running. Run `--login` while the daemon is up. The daemon
loads the new rows on its next refresh. `oauth-scope` is
optional; when omitted, the read scopes used by the sync are requested,
including `offline_access`.

## Resume

One thread runs active contacts, archived contacts, projects, the invoice
list, then missed invoice ids. The position is one `_configuration` value,
`sync-chain`, so a restart continues at the next page instead of listing
every contact again. The value is `phase:offset`, for example
`invoice-list:500`. Phases are `contact-active`, `contact-archived`,
`project`, `invoice-list`, and `invoice-missed`. Offset `0` is the first
page. Missed-ids keep offset `0`.

This is runtime progress, not a setting. A `_configuration` row overrides
the JSON file, so do not put `sync-chain` in the config. Rows named
`sync-chain-step`, `sync-chain-offset`, and `sync-chain-cycle` are read
only when `sync-chain` is absent, then the next save writes `sync-chain`.
No migration: the table already exists (created at startup if it is missing).

After each page whose rows all synced, the offset becomes the next request.
A failed page does not move it. When missed invoice ids finish, the state
returns to `contact-active:0`. The thread sleeps 250 ms after archived
contacts, after projects, and after invoices.

A phase resumed in the middle is not pruned. Rows from pages synced before
the restart were not marked seen in this process, and prune would delete
them. They are pruned on the next walk of that resource that starts at
offset 0. A resumed invoice list also skips missed-id fetches: that GET
would request every cached invoice the skipped pages did not show. A list
that starts at offset 0 still fetches missed ids. Paths and limits are
unchanged.

## Todo
 
 - Add proxy support : you can request like if it is Bexio, goes to local
   database first and goes to Bexio then.
 - Add a SQLite as possible backend.
 - Persistent caching with LMDB, stop re-inventing the wheel 

## Ready to be used ?

Yes, the code has been running for 3 years, keeping a copy well alive for a 
Bexio customer. The july 2026 bexio bug (no more rate-limiting header) did
revive the project a bit. And I could have some time to fix some bugs, use
Grok to expand quickly the scope of the duplication.

## Why ?

Bexio grew a lot because of a catastrophic failure of another big player in the
same market. Between my past experiences with them (they are very difficult to
work with) and the fact that they are reducing access with token, it seems to
me that they will slowly close their eco-system (and that aligns with the
general mentality in Switzerland).
And some people I know asked me to prepare something so they can begin to move
away from Bexio : it seems that the product is not really good for users.

## Why in C ?

C is fun.

# BX-SYNC en français

Permet de maintenir une copie local de [Bexio](https://bexio.com). Que ce soit
les conditions générales, qui indiquent que vous êtes responsable de 
sauvegarder les données, ou que vous ayez peur que Bexio fasse une Winbiz (ce
dont je suis de plus en plus convaincu que ça va arriver), avoir une copie 
des données sur votre serveur, ou machine, local n'est pas du luxe.

## Fonctionnalités
 - Pas mal de terminaisons dupliquée
 - Respecte la limitation des requêtes pour ne pas impacter votre travail
   quotidien
 - Plus résistant aux bugs de l'API Bexio (celui sur les limitation de juillet
   2026 par exemple).
 - Sauvegarde en temps quasi-réel.
 - Stoque les données dans une base de données relationnelles en respectant la
   structure (MySQL), permet donc une meilleure intégration à votre ERP.

`make test` vérifie le décodage JSON des factures et la reprise de la chaîne, sans MySQL ni réseau.

## Configuration

`mysql-host`, `mysql-user`, `mysql-password` et `mysql-database` restent dans
le fichier JSON. Tout le reste peut être dans la table `_configuration`. Une
ligne remplace le même nom dans le fichier. Les noms qui commencent par
`mysql-` sont ignorés. La table est créée au démarrage si elle n'existe pas.

```sql
INSERT INTO _configuration (name, type, cval)
  VALUES ('bexio-endpoint', 'string', 'https://api.bexio.com/');
INSERT INTO _configuration (name, type, ival)
  VALUES ('log-level', 'integer', 255);
```

`type` vaut `string` (`cval`), `integer` (`ival`), `bool` (`ival`, 0 ou 1),
`float` (`fval`, stocké en DOUBLE) ou `blob` (`bval`). Le fichier est lu en
premier, puis la table, avant les logs, le passage en démon et la connexion
Bexio. Les jetons OAuth sont dans la même table : `oauth-access-token`,
`oauth-refresh-token` (`string`) et `oauth-expires-at` (`integer`). `--login`
et chaque renouvellement écrivent ces lignes.

## Authentification

L'accès Bexio passe par OAuth 2.0 (authorization code) via liboauth2. Le
fichier de config contient `oauth-client-id`, `oauth-client-secret` et
`oauth-redirect-uri` (voir `conf.example.json`). Cette URI de redirection doit
être enregistrée sur l'application Bexio. Par défaut :
`http://127.0.0.1:8765/callback`.

La synchro tourne sur un serveur sans bureau. La connexion se fait une fois,
en SSH, avec un utilisateur qui peut écrire `_configuration` :

```
bxsync --login /usr/local/etc/bxsync.json
```

La commande affiche une URL d'autorisation. Depuis le portable, ouvrir un
tunnel puis cette URL dans un navigateur :

```
ssh -L 8765:127.0.0.1:8765 user@le-serveur
```

Si le navigateur ne joint pas le port transféré, copier l'URL complète de la
barre d'adresse (elle contient `?code=...`) et la coller dans la session SSH.
Les jetons sont écrits dans `_configuration`. bx-sync et bx-proxy les
partagent. Un renouvellement prend le verrou MariaDB `bx-oauth-refresh`
(20 secondes), relit les lignes, et ne poste un nouveau jeton que si le
jeton d'accès stocké doit encore être remplacé. `--login` ne se lance pas
via systemd. Un renouvellement refusé est journalisé
(`Refresh token was rejected. Run: bxsync --login <config>`) et le
processus continue. Lancer `--login` pendant que le démon tourne. Il
charge les nouvelles lignes à son prochain renouvellement. `oauth-scope`
est optionnel ; sans cette clé, les scopes de
lecture utilisés par la synchro sont demandés, y compris `offline_access`.

## Reprise

Un thread enchaîne les contacts actifs, les contacts archivés, les
projets, la liste des factures, puis les ids de facture manqués. La
position est une valeur de `_configuration`, `sync-chain` : un redémarrage
continue à la page suivante au lieu de relister tous les contacts. La
valeur est `phase:offset`, par exemple `invoice-list:500`. Les phases sont
`contact-active`, `contact-archived`, `project`, `invoice-list` et
`invoice-missed`. L'offset `0` est la première page. Les ids manqués
gardent l'offset `0`.

Ce n'est pas un réglage. Une ligne de `_configuration` remplace le fichier
JSON : ne pas y mettre `sync-chain`. Les lignes `sync-chain-step`,
`sync-chain-offset` et `sync-chain-cycle` sont lues seulement si
`sync-chain` est absent ; l'enregistrement suivant écrit `sync-chain`.
Aucune migration : la table existe déjà (créée au démarrage si besoin).

Après chaque page dont toutes les lignes ont été synchronisées, l'offset
devient la prochaine requête. Une page en erreur ne le fait pas avancer.
Quand les ids manqués sont chargés, l'état revient à `contact-active:0`.
Le thread dort 250 ms après les contacts archivés, après les projets et
après les factures.

Une phase reprise au milieu n'est pas purgée. Les lignes des pages déjà
faites avant l'arrêt n'ont pas été vues dans le cache de ce processus, et
le prune les supprimerait. Elles le sont au prochain parcours de cette
ressource qui part de l'offset 0. Une liste de factures reprise saute aussi
les ids manqués : ce GET demanderait chaque facture en cache que les pages
sautées n'ont pas montrée. Une liste qui part de l'offset 0 charge toujours
les ids manqués. Chemins et limites ne changent pas.

## À faire

 - Faire un véritable proxy, votre ERP questionne le proxy, celui-ci va
   chercher soit sur Bexio soit dans la base locale.
 - Une version avec SQLite en backend.
 - Le cache des IDs avec LMDB.

## Utilisable aujourd'hui ?

 Oui, depuis 2023 un client Bexio garde un copie de Bexio avec un usage interne
 pour son ERP sans que des problèmes majeures aient été détecté. Le bug de
 l'API Bexio (plus d'information sur les limitations de requête) a permis de
 retravailler un peu sur le project, corriger des bugs connus et exploiter 
 Grok pour augmenter rapidemment le nombre de terminaison dupliquées.

