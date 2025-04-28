#ifndef DATABASE_H
#define DATABASE_H

#include <zephyr/sys/atomic.h>
#include <zephyr/types.h>
#include <zephyr/drivers/can.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*transform_fn_t)(const struct can_frame *frame, atomic_t *out_val);

/**
 * Register a transform on a given CAN ID. You may call multiple times for the same ID
 * to extract multiple signals from one frame.
 * @param id CAN message ID
 * @param fn transformation function
 * @return pointer to atomic variable storing this signal's value
 */
atomic_t *db_register(uint32_t id, transform_fn_t fn);

/**
 * Process one received CAN frame: applies all transforms registered for its ID.
 */
void db_process_frame(const struct can_frame *frame);

/**
 * Iterate all stored signals and invoke cb(id, value) for each registration.
 */
void db_iterate(void (*cb)(uint32_t id, int64_t value));

#ifdef __cplusplus
}
#endif
#endif /* DATABASE_H */