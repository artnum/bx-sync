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

