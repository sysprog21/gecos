# GECOS

Implement GECOS, a lock-free synchronisation mechanism along with other well known mechanisms: RCU, HP (Hazard Pointers), etc, for comparison purpose.

Note: for "make rcu" you will need to install URCU (https://liburcu.org/)

To compile:
```shell
$ make
```

Example for running GECOS:
```shell
./list_gecos.exe 10 1 300 3
```

## License

`GECOS` is freely redistributable under GNU GPL version 2 or later.
