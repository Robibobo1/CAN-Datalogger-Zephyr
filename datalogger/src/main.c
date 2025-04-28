#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/logging/log.h>
//#include <zephyr/drivers/spi.h>
//#include <zephyr/fs/fs.h>
//#include <zephyr/fs/fat_fs.h>
#include "database.h"
//#include <sys/printk.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define CAN_THREAD_STACK 1024
#define LOG_THREAD_STACK 1024
#define LOG_PERIOD_MS 1000

CAN_MSGQ_DEFINE(can_rx_queue, 10);
const struct device *const can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

//static struct fs_file_t file;
//static struct fs_mount_t mp = {0};

/* Example transforms: extract two signals from one CAN ID */
void transform_speed(const struct can_frame *f, atomic_t *out) {
    // assume byte0 = speed
    atomic_set(out, f->data[0]);
}

void transform_rpm(const struct can_frame *f, atomic_t *out) {
    // assume bytes1-2 = rpm
    uint16_t rpm = (f->data[1] << 8) | f->data[2];
    atomic_set(out, rpm);
}

void can_thread(void) {
	int ret;

    if (!device_is_ready(can_dev)) 
    {
		LOG_ERR("CAN: Device %s not ready.\n", can_dev->name);
		return -1;
	}
	ret = can_start(can_dev);
	if (ret != 0) 
	{
		LOG_ERR("Error starting CAN controller [%d]", ret);
		return -1;
	}

	struct can_filter filter = {0, 0, 0}; // Filters nothing
	can_add_rx_filter_msgq(can_dev, &can_rx_queue, &filter);

    struct can_frame frame;
    while (1) {
        k_msgq_get(&can_rx_queue, &frame, K_FOREVER);
		LOG_DBG("Can received: %u\n",
			sys_be16_to_cpu(UNALIGNED_GET((uint16_t *)&frame.data)));
        db_process_frame(&frame);
    }
}

/*
void write_csv_line(uint32_t id, int64_t value) {
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "%03X,%lld,", id, value);
    fs_write(&file, buf, len);
}

void log_thread(void) {
    fs_mount(&mp);
    fs_open(&file, "/SDCARD/log.csv", FS_O_CREATE | FS_O_WRITE);
    fs_seek(&file, 0, FS_SEEK_END);

    while (1) {
        fs_write(&file, "\n", 1);
        db_iterate(write_csv_line);
        fs_sync(&file);
        k_msleep(LOG_PERIOD_MS);
    }
}
	*/

K_THREAD_DEFINE(can_tid, CAN_THREAD_STACK, can_thread, NULL, NULL, NULL, 7, 0, 0);
//K_THREAD_DEFINE(log_tid, LOG_THREAD_STACK, log_thread, NULL, NULL, NULL, 5, 0, 0);

void main(void) {
    db_init();
    // register two signals on ID 0x100
    db_register(0x100, transform_speed);
    db_register(0x100, transform_rpm);
}