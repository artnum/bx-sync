# BX-SYNC

Aims to keep [Bexio](https://bexio.com) data sync with a local database.

## Todo

* Clean bx_database and make it a separated library
* The bx_net part should handle the list differently, with pthread_cond_wait. Differentiate in queue and out queue (even on out queue per-thread)
* Use more generic function for endpoint manipulation. Maybe even some lua scripting

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

