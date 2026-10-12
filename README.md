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
rows.

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
`--login` is not a systemd command. If a refresh is rejected,
stop the service, run `--login` again, then start it. `oauth-scope` is
optional; when omitted, the read scopes used by the sync are requested,
including `offline_access`.

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
Les jetons sont écrits dans `_configuration`. Ensuite le démon ne fait que les
renouveler. `--login` ne se lance pas via systemd. Si un
renouvellement est refusé, arrêter le service, relancer `--login`, puis
redémarrer. `oauth-scope` est optionnel ; sans cette clé, les scopes de
lecture utilisés par la synchro sont demandés, y compris `offline_access`.

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

