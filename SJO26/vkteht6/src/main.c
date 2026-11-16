// Tavoittelen tästä tehtävästä 1 pistettä

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/timing/timing.h>
#include <stdlib.h>
#include <string.h>

#define STACKSIZE 500
#define PRIORITY 5
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)

#define TIME_LEN_ERROR   -1
#define TIME_NULL_ERROR  -2
#define TIME_VALUE_ERROR -3

static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

static const struct gpio_dt_spec red   = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec blue  = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

int time_parse(char *time)
{
	if (time == NULL) {
		return TIME_NULL_ERROR;
	}

	if (strlen(time) != 6) {
		return TIME_LEN_ERROR;
	}

	for (int i = 0; i < 6; i++) {
		if (time[i] < '0' || time[i] > '9') {
			return TIME_VALUE_ERROR;
		}
	}

	int values[3];
	values[2] = atoi(time + 4);
	time[4] = 0;
	values[1] = atoi(time + 2);
	time[2] = 0;
	values[0] = atoi(time);

	if (values[0] < 0 || values[0] > 23 ||
	    values[1] < 0 || values[1] > 59 ||
	    values[2] < 0 || values[2] > 59) {
		return TIME_VALUE_ERROR;
	}

	return values[0] * 3600 + values[1] * 60 + values[2];
}

int init_uart(void)
{
	if (!device_is_ready(uart_dev)) {
		return 1;
	}
	return 0;
}

int init_led(void);
void red_led_task(void *, void *, void *);
void green_led_task(void *, void *, void *);
void yellow_led_task(void *, void *, void *);
void debug_task(void *, void *, void *);

void red_off_handler(struct k_timer *t)
{
	gpio_pin_set_dt(&red, 0);
}
K_TIMER_DEFINE(red_off_timer, red_off_handler, NULL);

void alarm_handler(struct k_timer *t)
{
	gpio_pin_set_dt(&red, 1);
	k_timer_start(&red_off_timer, K_SECONDS(1), K_NO_WAIT);
}
K_TIMER_DEFINE(alarm_timer, alarm_handler, NULL);

K_THREAD_DEFINE(red_thread,    STACKSIZE, red_led_task,    NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(green_thread,  STACKSIZE, green_led_task,  NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(yellow_thread, STACKSIZE, yellow_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);

K_FIFO_DEFINE(dispatcher_fifo);
K_FIFO_DEFINE(data_fifo);

K_MUTEX_DEFINE(red_mutex);
K_CONDVAR_DEFINE(red_signal);
K_MUTEX_DEFINE(green_mutex);
K_CONDVAR_DEFINE(green_signal);
K_MUTEX_DEFINE(yellow_mutex);
K_CONDVAR_DEFINE(yellow_signal);

K_MUTEX_DEFINE(release_mutex);
K_CONDVAR_DEFINE(release_signal);

struct data_t {
	void *fifo_reserved;
	char msg[20];
};

struct debug_data_t {
	void *fifo_reserved;
	char color;
	uint64_t time;
};

static void send_debug(char color, uint64_t time_us)
{
	struct debug_data_t *d = k_malloc(sizeof(struct debug_data_t));
	if (d == NULL) {
		return;
	}
	d->color = color;
	d->time = time_us;
	k_fifo_put(&data_fifo, d);
}

uint64_t sequencetotal = 0;

int main(void)
{
	timing_init();
	timing_start();

	int ret = init_uart();
	if (ret != 0) {
		return ret;
	}

	ret = init_led();
	if (ret != 0) {
		return ret;
	}

	while (true) {
		k_msleep(100);
	}

	return 0;
}

static void uart_task(void *unused1, void *unused2, void *unused3)
{
	char rc = 0;
	char uart_msg[20];
	memset(uart_msg, 0, 20);
	int uart_msg_cnt = 0;

	while (true) {
		if (uart_poll_in(uart_dev, &rc) == 0) {
			if (rc != 'X') {
				if (uart_msg_cnt < 19) {
					uart_msg[uart_msg_cnt++] = rc;
				}
			} else {
				if (uart_msg_cnt == 0 ||
				    (uart_msg[0] >= '0' && uart_msg[0] <= '9') ||
				    uart_msg[0] == '-') {
					int secs = time_parse(uart_msg);
					printk("%dX", secs);
					if (secs >= 0) {
						k_timer_start(&alarm_timer, K_SECONDS(secs), K_NO_WAIT);
					}
				} else {
					struct data_t *buf = k_malloc(sizeof(struct data_t));
					if (buf == NULL) {
						return;
					}
					memcpy(buf->msg, uart_msg, sizeof(uart_msg));
					k_fifo_put(&dispatcher_fifo, buf);
				}

				uart_msg_cnt = 0;
				memset(uart_msg, 0, 20);
			}
		}
		k_msleep(10);
	}
}

static void dispatcher_task(void *unused1, void *unused2, void *unused3)
{
	while (true) {
		struct data_t *rec_item = k_fifo_get(&dispatcher_fifo, K_FOREVER);
		char sequence[20];
		memcpy(sequence, rec_item->msg, 20);
		k_free(rec_item);

		sequencetotal = 0;
		int64_t seq_start = k_uptime_get();

		for (int cnt = 0; cnt < strlen(sequence); cnt++) {
			char color = sequence[cnt];

			if (color == 'R' || color == 'r') {
				k_mutex_lock(&red_mutex, K_FOREVER);
				k_condvar_broadcast(&red_signal);
				k_mutex_unlock(&red_mutex);
			}

			if (color == 'Y' || color == 'y') {
				k_mutex_lock(&yellow_mutex, K_FOREVER);
				k_condvar_broadcast(&yellow_signal);
				k_mutex_unlock(&yellow_mutex);
			}

			if (color == 'G' || color == 'g') {
				k_mutex_lock(&green_mutex, K_FOREVER);
				k_condvar_broadcast(&green_signal);
				k_mutex_unlock(&green_mutex);
			}

			k_mutex_lock(&release_mutex, K_FOREVER);
			k_condvar_wait(&release_signal, &release_mutex, K_FOREVER);
			k_mutex_unlock(&release_mutex);
		}

		uint64_t seq_us = (uint64_t)(k_uptime_get() - seq_start) * 1000;
		send_debug('S', seq_us);

		printk("Sequencetotal: %llu us\n", sequencetotal);
	}
}

void red_led_task(void *p1, void *p2, void *p3)
{
	while (true) {
		k_mutex_lock(&red_mutex, K_FOREVER);
		k_condvar_wait(&red_signal, &red_mutex, K_FOREVER);
		k_mutex_unlock(&red_mutex);

		timing_t start_time = timing_counter_get();

		gpio_pin_set_dt(&red, 1);
		k_sleep(K_SECONDS(1));

		gpio_pin_set_dt(&red, 0);
		k_sleep(K_SECONDS(1));

		timing_t end_time = timing_counter_get();
		uint64_t time_us = timing_cycles_to_ns(
					   timing_cycles_get(&start_time, &end_time)) / 1000;
		sequencetotal += time_us;
		send_debug('R', time_us);

		k_condvar_broadcast(&release_signal);
	}
}

void green_led_task(void *p1, void *p2, void *p3)
{
	while (true) {
		k_mutex_lock(&green_mutex, K_FOREVER);
		k_condvar_wait(&green_signal, &green_mutex, K_FOREVER);
		k_mutex_unlock(&green_mutex);

		timing_t start_time = timing_counter_get();

		gpio_pin_set_dt(&green, 1);
		k_sleep(K_SECONDS(1));

		gpio_pin_set_dt(&green, 0);
		k_sleep(K_SECONDS(1));

		timing_t end_time = timing_counter_get();
		uint64_t time_us = timing_cycles_to_ns(
					   timing_cycles_get(&start_time, &end_time)) / 1000;
		sequencetotal += time_us;
		send_debug('G', time_us);

		k_condvar_broadcast(&release_signal);
	}
}

void yellow_led_task(void *p1, void *p2, void *p3)
{
	while (true) {
		k_mutex_lock(&yellow_mutex, K_FOREVER);
		k_condvar_wait(&yellow_signal, &yellow_mutex, K_FOREVER);
		k_mutex_unlock(&yellow_mutex);

		timing_t start_time = timing_counter_get();

		gpio_pin_set_dt(&green, 1);
		gpio_pin_set_dt(&red, 1);
		k_sleep(K_SECONDS(1));

		gpio_pin_set_dt(&green, 0);
		gpio_pin_set_dt(&red, 0);
		k_sleep(K_SECONDS(1));

		timing_t end_time = timing_counter_get();
		uint64_t time_us = timing_cycles_to_ns(
					   timing_cycles_get(&start_time, &end_time)) / 1000;
		sequencetotal += time_us;
		send_debug('Y', time_us);

		k_condvar_broadcast(&release_signal);
	}
}

void debug_task(void *p1, void *p2, void *p3)
{
	while (true) {
		struct debug_data_t *received = k_fifo_get(&data_fifo, K_FOREVER);

		if (received->color == 'S') {
			printk("Debug: whole sequence took %llu us\n", received->time);
		} else {
			printk("Debug: %c task took %llu us\n", received->color, received->time);
		}

		k_free(received);
	}
}

K_THREAD_DEFINE(dis_thread,   STACKSIZE, dispatcher_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(uart_thread,  STACKSIZE, uart_task,       NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(debug_thread, 1024,      debug_task,      NULL, NULL, NULL, PRIORITY, 0, 0);

int init_led(void)
{
	int ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_configure_dt(&blue, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	gpio_pin_set_dt(&red, 0);
	gpio_pin_set_dt(&green, 0);
	gpio_pin_set_dt(&blue, 0);

	return 0;
}
