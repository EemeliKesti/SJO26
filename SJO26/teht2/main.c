//Tavoittelen tästä tehtävästä 1 pistettä, palautuksen myöhästymisestä huolimatta

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>


#define STACKSIZE 500
#define PRIORITY 5
#define BUTTON_0 DT_ALIAS(sw0)
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

int init_uart(void) {
    if (!device_is_ready(uart_dev)) {
        return 1;
    }
    return 0;
}

int tila = 0;

static const struct gpio_dt_spec red   = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec blue  = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);


int init_led(void);
void red_led_task(void *, void *, void *);
void green_led_task(void *, void *, void *);
void yellow_led_task(void *, void *, void *);


K_THREAD_DEFINE(red_thread,    STACKSIZE, red_led_task,    NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(green_thread,  STACKSIZE, green_led_task,  NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(yellow_thread, STACKSIZE, yellow_led_task, NULL, NULL, NULL, PRIORITY, 0, 0);

K_FIFO_DEFINE(dispatcher_fifo);

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

// Main program
int main(void)
{
    int ret = init_uart();
    if (ret != 0) {
        printk("UART initialization failed!\n");
        return ret;
    }
    ret = init_led();
    if (ret != 0) {
        printk("Led initialization failed!\n");
        return ret;
    }

    printk("Started serialread example\n");
    char rc = 0;
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
            if (rc != '\r') {
                uart_msg[uart_msg_cnt] = rc;
                uart_msg_cnt++;
            } else {
                printk("UART msg: %s\n", uart_msg);

                // FIFO Stuff begins
                struct data_t *buf = k_malloc(sizeof(struct data_t));
                if (buf == NULL) {
                    return;
                }
                // Copy UART message to dispatcher data
                snprintf(buf->msg, 20, "%s", uart_msg);
                memcpy(buf->msg, uart_msg, sizeof(uart_msg));
                k_fifo_put(&dispatcher_fifo, buf);

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
        // Receive dispatcher data from uart_task fifo
        struct data_t *rec_item = k_fifo_get(&dispatcher_fifo, K_FOREVER);
        char sequence[20];
        memcpy(sequence, rec_item->msg, 20);
        k_free(rec_item);

        printk("Dispatcher: %s\n", sequence);
        int cnt = 0;

        for (cnt = 0; cnt < strlen(sequence); cnt++) {
            char color = sequence[cnt];

            if (color == 'R' || color == 'r') {
                printk("RED ");
                k_mutex_lock(&red_mutex, K_FOREVER);
                k_condvar_broadcast(&red_signal);
                k_mutex_unlock(&red_mutex);
            }

            if (color == 'Y' || color == 'y') {
                printk("YELLOW ");
                k_mutex_lock(&yellow_mutex, K_FOREVER);
                k_condvar_broadcast(&yellow_signal);
                k_mutex_unlock(&yellow_mutex);
            }

            if (color == 'G' || color == 'g') {
                printk("GREEN ");
                k_mutex_lock(&green_mutex, K_FOREVER);
                k_condvar_broadcast(&green_signal);
                k_mutex_unlock(&green_mutex);
            }

            k_mutex_lock(&release_mutex, K_FOREVER);
            k_condvar_wait(&release_signal, &release_mutex, K_FOREVER);
            k_mutex_unlock(&release_mutex);
        }
        // You need to:
        // Parse color and time from the fifo data
        // Example
        //    char color = sequence[0];
        //    int time = atoi(sequence+2);
        //    printk("Data: %c %d\n", color, time);
        // Send the parsed color information to tasks using fifo
        // Use release signal to control sequence or k_yield
    }
}

// Task to handle red led
void red_led_task(void *p1, void *p2, void *p3)
{
    printk("Red led thread started\n");
    while (true) {
        k_mutex_lock(&red_mutex, K_FOREVER);
        k_condvar_wait(&red_signal, &red_mutex, K_FOREVER);
        k_mutex_unlock(&red_mutex);

        gpio_pin_set_dt(&red, 1);
        printk("Red on\n");
        k_sleep(K_SECONDS(1));

        gpio_pin_set_dt(&red, 0);
        printk("Red off\n");
        k_sleep(K_SECONDS(1));

        k_condvar_broadcast(&release_signal);
    }
}

// Task to handle green led
void green_led_task(void *p1, void *p2, void *p3)
{
    printk("Green led thread started\n");
    while (true) {
        k_mutex_lock(&green_mutex, K_FOREVER);
        k_condvar_wait(&green_signal, &green_mutex, K_FOREVER);
        k_mutex_unlock(&green_mutex);

        gpio_pin_set_dt(&green, 1);
        printk("Green on\n");
        k_sleep(K_SECONDS(1));

        gpio_pin_set_dt(&green, 0);
        printk("Green off\n");
        k_sleep(K_SECONDS(1));

        k_condvar_broadcast(&release_signal);
    }
}

// Task to handle yellow led
void yellow_led_task(void *p1, void *p2, void *p3)
{
    printk("Yellow led thread started\n");
    while (true) {
        k_mutex_lock(&yellow_mutex, K_FOREVER);
        k_condvar_wait(&yellow_signal, &yellow_mutex, K_FOREVER);
        k_mutex_unlock(&yellow_mutex);

        gpio_pin_set_dt(&green, 1);
        gpio_pin_set_dt(&red, 1);
        printk("Yellow on\n");
        k_sleep(K_SECONDS(1));

        gpio_pin_set_dt(&green, 0);
        gpio_pin_set_dt(&red, 0);
        printk("Yellow off\n");
        k_sleep(K_SECONDS(1));

        k_condvar_broadcast(&release_signal);
    }
}

K_THREAD_DEFINE(dis_thread,  STACKSIZE, dispatcher_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(uart_thread, STACKSIZE, uart_task,       NULL, NULL, NULL, PRIORITY, 0, 0);

// Initialize leds
int init_led(void)
{
    int ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Error: Led configure failed\n");
        return ret;
    }

    ret = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Error: Led configure failed\n");
        return ret;
    }

    ret = gpio_pin_configure_dt(&blue, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Error: Led configure failed\n");
        return ret;
    }

    gpio_pin_set_dt(&red, 0);
    gpio_pin_set_dt(&green, 0);
    gpio_pin_set_dt(&blue, 0);

    printk("Led initialized ok\n");
    return 0;
}