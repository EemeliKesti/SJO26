#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>

#define STACKSIZE 500
#define PRIORITY 5
#define BUTTON_0 DT_ALIAS(sw0)
int tila = 0;
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec blue = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

void red_led_task(void *, void *, void*);
K_THREAD_DEFINE(red_thread,STACKSIZE,red_led_task,NULL,NULL,NULL,PRIORITY,0,0);
void green_led_task(void *, void *, void*);
K_THREAD_DEFINE(green_thread,STACKSIZE,green_led_task,NULL,NULL,NULL,PRIORITY,0,0);
//void blue_led_task(void *, void *, void*);
//K_THREAD_DEFINE(blue_thread,STACKSIZE,blue_led_task,NULL,NULL,NULL,PRIORITY,0,0);
void yellow_led_task(void *, void *, void*);
K_THREAD_DEFINE(yellow_thread,STACKSIZE,yellow_led_task,NULL,NULL,NULL,PRIORITY,0,0);

// Main program
int main(void)
{
	init_led();

	return 0;
}

// Initialize leds
int  init_led() {

	// Led pin initialization
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
	// set led off
	gpio_pin_set_dt(&red,0);
        gpio_pin_set_dt(&green,0);
        gpio_pin_set_dt(&blue,0);

	printk("Led initialized ok\n");
	
	return 0;
}

// Task to handle red led
void red_led_task(void *, void *, void*) {
	
	printk("Red led thread started\n");
	while (true) {
                if (tila == 0) {
		// 1. set led on 
		gpio_pin_set_dt(&red,1);
		printk("Red on\n");
		// 2. sleep for 2 seconds
		k_sleep(K_SECONDS(1));
		// 3. set led off
		gpio_pin_set_dt(&red,0);
		printk("Red off\n");
                k_sleep(K_SECONDS(1));
                tila = 1;
		// 4. sleep for 2 seconds
                } else{
                        k_msleep(100);
                }
	}
}
void green_led_task(void *, void *, void*) {
	
	printk("Green led thread started\n");
	while (true) {
                if (tila == 2) {
		// 1. set led on 
		gpio_pin_set_dt(&green,1);
		printk("Green on\n");
		// 2. sleep for 2 seconds
		k_sleep(K_SECONDS(1));
		// 3. set led off
		gpio_pin_set_dt(&green,0);
		printk("Green off\n");
                k_sleep(K_SECONDS(1));
                tila = 0;
                } else{
                        k_msleep(100);
                }
		// 4. sleep for 2 seconds
	}
}
void blue_led_task(void *, void *, void*) {
	
	printk("Blue led thread started\n");
	while (true) {
		// 1. set led on 
		gpio_pin_set_dt(&blue,1);
		printk("Blue on\n");
		// 2. sleep for 2 seconds
		k_sleep(K_SECONDS(1));
		// 3. set led off
		gpio_pin_set_dt(&blue,0);
		printk("Blue off\n");
		// 4. sleep for 2 seconds
		k_sleep(K_SECONDS(1));
               
	}
}
void yellow_led_task(void *, void *, void*) {
	
	printk("yellow led thread started\n");
	while (true) {
                if (tila == 1) {
		// 1. set led on 
		gpio_pin_set_dt(&red,1);
                gpio_pin_set_dt(&green,1);
		printk("yellow on\n");
		// 2. sleep for 2 seconds
		k_sleep(K_SECONDS(1));
		// 3. set led off
		gpio_pin_set_dt(&red,0);
                gpio_pin_set_dt(&green,0);
		printk("yellow off\n");
                k_sleep(K_SECONDS(1));
                tila = 2;
                } else{
                        k_msleep(100);
                }
		// 4. sleep for 2 seconds
	}
}