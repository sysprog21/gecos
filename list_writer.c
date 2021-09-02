#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include "lock.h"
#include "test.h"

void free_node_later(int, node_t *);
node_t *init_new_node();


int delete (int key, node_t **head, lock_t *lock)
{
    node_t *cur, **prev;

    if (*head == NULL)
        return 0;

    lock_wlock(lock);
    prev = head;
    for (cur = *head; cur != NULL; cur = *prev) {
        write_compute();
        if (cur->key >= key) {
            if (cur->key != key)
                break;
            *prev = cur->next;
            // write_write_barrier(); done in the expected func
#ifdef GCLOCK  // increment the GC before liberating the node!
            lock_wunlock(lock);
#endif

            free_node_later(thread, cur);
            // show();
#ifdef GCLOCK
            return 1;
#else
            lock_wunlock(lock);
            return 1;
#endif
        }
        prev = &cur->next;
    }

    lock_wunlock(lock);
    return 0;
}


int insert(int key, node_t **head, lock_t *lock)
{
    int sucess = 0;
    node_t *new;
    node_t *cur, **prev;

    lock_wlock(lock);
    prev = head;
    cur = *head;

    while (cur) {
        write_compute();
        if (cur->key >= key) {
            if (cur->key == key)
                goto out;
            break;
        }
        prev = &cur->next;
        cur = cur->next;
    }

    new = init_new_node();
    new->key = key;
    new->next = cur;
    write_write_barrier();
    *prev = new;
    sucess = 1;
    // show();
out:
    lock_wunlock(lock);  // implement the correct barrier!
    return sucess;
}
