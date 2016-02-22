#include "mcu_api.h"
#include "mcu_errno.h"

#define GPIO_LED		40	// D13 on DFRobot IO expansion shield for Intel Edison
#define INPUT			0
#define OUTPUT			1
#define HIGH			1
#define LOW				0

void mcu_main()
{
	int version = api_version();
	debug_print(DBG_INFO, "API version: %d.%d\n", version/100, version%100);

	gpio_setup(GPIO_LED, OUTPUT);
	while (1) {
		gpio_write(GPIO_LED, gpio_read(GPIO_LED)? LOW : HIGH);
		mcu_sleep(100);
	}
}
