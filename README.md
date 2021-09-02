# GECOS

`GECOS` is a lock-free synchronization mechanism, and this repository compares
various well-known mechanisms such as RCU and HP (Hazard Pointers).

Note: for `make rcu`, you will need to install [URCU](https://liburcu.org/).

To compile:
```shell
$ make
```

Example for running `GECOS`:
```shell
./list_gecos.exe 10 1 300 3
```

## License

`GECOS` is freely redistributable under GNU GPL version 2 or later.
