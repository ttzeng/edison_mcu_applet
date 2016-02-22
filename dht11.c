#include "mcu_api.h"
#include "mcu_errno.h"
#include <string.h>

typedef unsigned char	uint8_t;
typedef unsigned short	uint16_t;
typedef unsigned int	uint32_t;

#define GPIO_DHT		48	// D7 on DFRobot IO expansion shield for Intel Edison
#define INPUT			0
#define OUTPUT			1
#define HIGH			1
#define LOW				0

#define DHT_DATA_LEN	5
#define DHT_SUCCESS		0
#define DHT_ENOSPACE	-1
#define DHT_ECHECKSUM	-2

#define LEN_IPCBUF		64

static int dht11_read(uint8_t pin, uint8_t *buf, uint8_t buflen)
{
	if (buflen < DHT_DATA_LEN)
		return DHT_ENOSPACE;

	/* Send START signal by pulling low the Data Signal bus at least 18ms */
	gpio_setup(pin, OUTPUT);
	gpio_write(pin, LOW);
	mcu_delay(20000);

	/* Release the Data Signal bus and wait 20~40us for DHT's response */
	gpio_write(pin, HIGH);
	gpio_setup(pin, INPUT);
	mcu_delay(30);

	/* DHT responds by pulling the Data Signal bus for 80us */
	while (gpio_read(pin) != LOW);
	while (gpio_read(pin) == LOW);

	/* DHT pulls high the Data Signal bus for 80us and prepares the sending data */
	while (gpio_read(pin) != LOW);

	uint8_t index, checksum = 0;
	for (index = 0; index < DHT_DATA_LEN; index++) {
		uint8_t bit, value = 0;
		for (bit = 0x80; bit; bit >>= 1) {
			/* DHT pulls low the Data Signal bus for 50us to start sending each bit */
			while (gpio_read(pin) == LOW);

			/* DHT pulls high the Data Signal bus, the length of duration is
			   determined by value of the sending bit. 0: 26~28us, 1: 70us */
			mcu_delay(40);
			if (gpio_read(pin) != LOW) {
				while (gpio_read(pin) != LOW);
				value |= bit;
			}
		}
		buf[index] = value;
	}

	/* Acquire the Data Signal bus and keep it high */
	gpio_setup(pin, OUTPUT);
	gpio_write(pin, HIGH);

	for (index = 0; index < DHT_DATA_LEN-1; index++)
		checksum += buf[index];
	return (buf[4] == checksum)? DHT_SUCCESS : DHT_ECHECKSUM;
}

void mcu_main()
{
	int version = api_version();
	debug_print(DBG_INFO, "API version: %d.%d\n", version/100, version%100);

	gpio_setup(GPIO_DHT, OUTPUT);
	gpio_write(GPIO_DHT, HIGH);

	/* do not send instructions within 1S after power on to pass the unstable state */
	mcu_sleep(100);

	uint8_t data[DHT_DATA_LEN];
	while (1) {
		int rc;
		uint8_t humidity, temperature;
		if ((rc = dht11_read(GPIO_DHT, data, sizeof(data))) == DHT_SUCCESS) {
			humidity = data[0], temperature = data[2];
			debug_print(DBG_INFO, "Humidity: %d%, Temperature: %d C\n", humidity, temperature);
//			debug_print(DBG_INFO, "%d %d %d %d %d\n",
//						data[0], data[1], data[2], data[3], data[4]);
		} else {
			debug_print(DBG_ERROR, "dht11_read() error [%d]\n", rc);
		}

		unsigned char buf[LEN_IPCBUF];
		if ((rc = host_receive(buf, sizeof(buf))) > 0 && buf[0] == '?') {
			rc = mcu_snprintf((char*)buf, LEN_IPCBUF, "RH=%d,T=%d\n", humidity, temperature);
			host_send(buf, rc);
		}

		mcu_sleep(300);
	}
}
