#include "database.h"
#include <zephyr/sys/atomic.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(db, LOG_LEVEL_INF);

#define MAX_ENTRIES 32

struct db_entry {
    uint32_t id;
    atomic_t val;
    transform_fn_t fn;
    bool used;
};
static struct db_entry entries[MAX_ENTRIES];
static struct k_mutex db_mutex;

void db_init(void) {
    k_mutex_init(&db_mutex);
}

// Register a new signal extractor for a CAN ID
atomic_t *db_register(uint32_t id, transform_fn_t fn) {
    k_mutex_lock(&db_mutex, K_FOREVER);
    for (int i = 0; i < MAX_ENTRIES; i++) {
        if (!entries[i].used) {
            entries[i].used = true;
            entries[i].id = id;
            entries[i].fn = fn;
            atomic_set(&entries[i].val, 0);
            k_mutex_unlock(&db_mutex);
            return &entries[i].val;
        }
    }
    k_mutex_unlock(&db_mutex);
    return NULL; // no space
}

// Apply all transforms matching this frame's ID
void db_process_frame(const struct can_frame *frame) {
    k_mutex_lock(&db_mutex, K_FOREVER);
    for (int i = 0; i < MAX_ENTRIES; i++) {
        if (entries[i].used && entries[i].id == frame->id) {
            entries[i].fn(frame, &entries[i].val);
        }
    }
    k_mutex_unlock(&db_mutex);
}

// Walk through all registered signals
void db_iterate(void (*cb)(uint32_t id, int64_t value)) {
    k_mutex_lock(&db_mutex, K_FOREVER);
    for (int i = 0; i < MAX_ENTRIES; i++) {
        if (entries[i].used) {
            int64_t v = atomic_get(&entries[i].val);
            cb(entries[i].id, v);
        }
    }
    k_mutex_unlock(&db_mutex);
}