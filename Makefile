#CFLAGS += -O3 -DONE_WRITER
CFLAGS += -DONE_WRITER -O2 
#CFLAGS += -DONE_WRITER -O1
#CFLAGS += -DONE_WRITER -O0
#CFLAGS += -DONE_WRITER -g -rdynamic -DDEBUG -O0
#CFLAGS +=-g -rdynamic -DDEBUG
LIB_CFLAGS += -pthread -DPOPULATE #-DCHECK_CCRT

RCU = -DRCU 
RCU_S = -DRCU_SIGNAL -lurcu-signal 

MAIN = mrow.c

all: gecos sigecos spin rwlock hp rc #rcu

gc_based: gecos gecos_back gecos_relink sigecos

gecos: lock.h test.h list_gecos.c list_writer.c
	gcc $(LIB_CFLAGS) $(CFLAGS) allocator_malloc.c  $(MAIN) list_gecos.c list_writer.c -o list_gecos.exe
gecos_back: lock.h test.h list_gecos.c list_writer.c
	gcc $(LIB_CFLAGS) $(CFLAGS) -DGECOS_B allocator_malloc.c  $(MAIN) list_gecos.c list_writer.c -o list_gecos_back.exe
gecos_relink: lock.h test.h list_gecos.c list_writer.c
	gcc $(LIB_CFLAGS) $(CFLAGS) -DGECOS_R allocator_malloc.c  $(MAIN) list_gecos.c list_writer.c -o list_gecos_relink.exe
sigecos: lock.h test.h list_sgecos.c
	gcc $(LIB_CFLAGS) $(CFLAGS)  -DSGECOS allocator_malloc.c  $(MAIN) list_sgecos.c -o list_sigecos.exe

locks: spin rwlock

spin: lock.h test.h
	gcc $(LIB_CFLAGS) $(CFLAGS) -DLSPIN allocator_malloc.c  $(MAIN) list_lock.c list_writer.c -o list_lock.exe
rwlock: lock.h test.h
	gcc $(LIB_CFLAGS) $(CFLAGS) -DRWLOCK allocator_malloc.c  $(MAIN) list_lock.c list_writer.c -o list_rwlock.exe

hp: lock.h test.h
	gcc $(LIB_CFLAGS) $(CFLAGS) allocator_malloc.c  $(MAIN) list_hp.c list_writer.c hp.c -o list_hp.exe

rc: lock.h test.h
	gcc $(LIB_CFLAGS) $(CFLAGS) allocator_malloc.c  $(MAIN) list_writer.c list_rc.c rc.c -o list_rc.exe

rcu: lock.h test.h
	gcc $(LIB_CFLAGS) $(CFLAGS) $(RCU) $(RCU_S) allocator_malloc.c  $(MAIN) list_rcu.c list_writer.c rcu.c -o list_rcu.exe

clean:
	rm -f *.exe
