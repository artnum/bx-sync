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

