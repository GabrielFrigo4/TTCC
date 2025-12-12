#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <libserialport.h>

#include "libesp32.h"
#include "platform.h"

#define SLIP_BYTE_END           0xC0
#define SLIP_BYTE_ESC           0xDB
#define SLIP_BYTE_ESC_END       0xDC
#define SLIP_BYTE_ESC_ESC       0xDD

#define CMD_SYNC                0x08
#define CMD_READ_REG            0x0A

#define REG_EFUSE_BASE          0x3FF5A000
#define REG_MAC_ADDR_LOW        (REG_EFUSE_BASE + 0x04)
#define REG_MAC_ADDR_HIGH       (REG_EFUSE_BASE + 0x08)

#define SERIAL_BAUDRATE         115200
#define TIMEOUT_READ_MS         10
#define TIMEOUT_WRITE_MS        100

#define PACKET_SYNC_SIZE        36
#define ATTEMPTS_SYNC_FAST      5
#define ATTEMPTS_SYNC_FULL      20
#define ATTEMPTS_READ_REG       3

#define DELAY_SIGNAL_MS         5
#define DELAY_POST_RESET_MS     50

static void platform_sleep_ms(int ms) {
#ifdef PLATFORM_WINDOWS
	Sleep(ms);
#else
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000000;
	nanosleep(&ts, NULL);
#endif
}

static void reset_strategy_usb_native(struct sp_port *port) {
	sp_set_dtr(port, SP_DTR_OFF);
	sp_set_rts(port, SP_RTS_ON);
	platform_sleep_ms(DELAY_SIGNAL_MS);

	sp_set_dtr(port, SP_DTR_ON);
	platform_sleep_ms(DELAY_SIGNAL_MS);
	sp_set_dtr(port, SP_DTR_OFF);

	platform_sleep_ms(DELAY_POST_RESET_MS);
	sp_set_rts(port, SP_RTS_OFF);
}

static void reset_strategy_classic(struct sp_port *port) {
	sp_set_dtr(port, SP_DTR_OFF);
	sp_set_rts(port, SP_RTS_OFF);
	platform_sleep_ms(DELAY_SIGNAL_MS);

	sp_set_dtr(port, SP_DTR_OFF);
	sp_set_rts(port, SP_RTS_ON);
	platform_sleep_ms(DELAY_SIGNAL_MS);

	sp_set_dtr(port, SP_DTR_ON);
	sp_set_rts(port, SP_RTS_OFF);
	platform_sleep_ms(DELAY_SIGNAL_MS);

	sp_set_dtr(port, SP_DTR_OFF);
	sp_set_rts(port, SP_RTS_ON);
	platform_sleep_ms(DELAY_POST_RESET_MS);

	sp_set_dtr(port, SP_DTR_OFF);
	sp_set_rts(port, SP_RTS_OFF);
}

static bool slip_write_frame(struct sp_port *port, uint8_t op, const uint8_t *data, uint16_t len, uint32_t checksum) {
	uint8_t buffer[1024];
	int index = 0;

	uint8_t header[8] = {
		0x00, op, (uint8_t)(len & 0xFF), (uint8_t)(len >> 8),
		(uint8_t)(checksum & 0xFF), (uint8_t)(checksum >> 8),
		(uint8_t)(checksum >> 16), (uint8_t)(checksum >> 24)
	};

	buffer[index++] = SLIP_BYTE_END;

	#define SLIP_ENCODE(byte) \
	if (byte == SLIP_BYTE_END) { \
		buffer[index++] = SLIP_BYTE_ESC; buffer[index++] = SLIP_BYTE_ESC_END; \
	} else if (byte == SLIP_BYTE_ESC) { \
		buffer[index++] = SLIP_BYTE_ESC; buffer[index++] = SLIP_BYTE_ESC_ESC; \
	} else { \
		buffer[index++] = byte; \
	}

	for (int i = 0; i < 8; i++) { SLIP_ENCODE(header[i]); }
	for (int i = 0; i < len; i++) { SLIP_ENCODE(data[i]); }

	buffer[index++] = SLIP_BYTE_END;

	return sp_blocking_write(port, buffer, index, TIMEOUT_WRITE_MS) == index;
}

static int slip_read_frame(struct sp_port *port, uint8_t *out_buf, int max_len) {
	uint8_t byte;
	int count = 0;
	bool in_frame = false;
	bool is_escaped = false;

	for (int i = 0; i < 200; i++) {
		if (sp_blocking_read(port, &byte, 1, TIMEOUT_READ_MS) <= 0) continue;

		if (byte == SLIP_BYTE_END) {
			if (in_frame) return count;
			in_frame = true; count = 0; continue;
		}
		if (!in_frame) continue;

		if (byte == SLIP_BYTE_ESC) { is_escaped = true; continue; }
		if (is_escaped) {
			if (byte == SLIP_BYTE_ESC_END) byte = SLIP_BYTE_END;
			else if (byte == SLIP_BYTE_ESC_ESC) byte = SLIP_BYTE_ESC;
			is_escaped = false;
		}
		if (count < max_len) out_buf[count++] = byte;
	}
	return -1;
}

static bool perform_chip_sync(struct sp_port *port, int attempts) {
	uint8_t sync_pattern[PACKET_SYNC_SIZE] = {
		0x07, 0x07, 0x12, 0x20, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
		0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
		0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55
	};
	uint8_t response[128];

	for (int i = 0; i < attempts; i++) {
		slip_write_frame(port, CMD_SYNC, sync_pattern, PACKET_SYNC_SIZE, 0);
		if (slip_read_frame(port, response, sizeof(response)) > 1) {
			return true;
		}
	}
	return false;
}

static uint32_t read_efuse_register(struct sp_port *port, uint32_t address) {
	uint8_t payload[4] = {
		(uint8_t)(address & 0xFF), (uint8_t)((address >> 8) & 0xFF),
		(uint8_t)((address >> 16) & 0xFF), (uint8_t)((address >> 24) & 0xFF)
	};
	uint8_t response[128];

	for (int i = 0; i < ATTEMPTS_READ_REG; i++) {
		slip_write_frame(port, CMD_READ_REG, payload, 4, 0);
		int len = slip_read_frame(port, response, sizeof(response));

		if (len >= 8 && response[1] == CMD_READ_REG) {
			return (uint32_t)response[4] | ((uint32_t)response[5] << 8) |
			((uint32_t)response[6] << 16) | ((uint32_t)response[7] << 24);
		}
		sp_flush(port, SP_BUF_INPUT);
	}
	return 0;
}

static void format_mac_address(uint32_t low, uint32_t high, char *buffer, size_t size) {
	uint8_t mac[6];
	mac[0] = (uint8_t)((high >> 8) & 0xFF);
	mac[1] = (uint8_t)(high & 0xFF);
	mac[2] = (uint8_t)((low >> 24) & 0xFF);
	mac[3] = (uint8_t)((low >> 16) & 0xFF);
	mac[4] = (uint8_t)((low >> 8) & 0xFF);
	mac[5] = (uint8_t)(low & 0xFF);

	snprintf(buffer, size, "%02X:%02X:%02X:%02X:%02X:%02X",
			 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

bool esp32_check_port_format(const char *port) {
	return (port && strlen(port) > 0);
}

void esp32_print_mac(const char *mac_str) {
	if (mac_str) printf("%s\n", mac_str);
}

bool esp32_get_mac_from_port(const char *port_name, char *mac_buf, size_t buf_size) {
	struct sp_port *port;
	if (sp_get_port_by_name(port_name, &port) != SP_OK) return false;
	if (sp_open(port, SP_MODE_READ_WRITE) != SP_OK) {
		sp_free_port(port);
		return false;
	}

	sp_set_baudrate(port, SERIAL_BAUDRATE);
	sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE);
	sp_set_bits(port, 8);
	sp_set_parity(port, SP_PARITY_NONE);
	sp_set_stopbits(port, 1);

	sp_flush(port, SP_BUF_BOTH);

	bool sync_success = perform_chip_sync(port, ATTEMPTS_SYNC_FAST);

	if (!sync_success) {
		if (strstr(port_name, "ACM")) {
			reset_strategy_usb_native(port);
		} else {
			reset_strategy_classic(port);
		}
		sp_flush(port, SP_BUF_BOTH);
		sync_success = perform_chip_sync(port, ATTEMPTS_SYNC_FULL);
	}

	if (sync_success) {
		sp_flush(port, SP_BUF_INPUT);
		uint32_t mac_low = read_efuse_register(port, REG_MAC_ADDR_LOW);
		uint32_t mac_high = read_efuse_register(port, REG_MAC_ADDR_HIGH);
		format_mac_address(mac_low, mac_high, mac_buf, buf_size);
	}

	sp_close(port);
	sp_free_port(port);
	return sync_success;
}

bool esp32_find_any_mac(char *mac_buf, size_t buf_size) {
	struct sp_port **ports;
	if (sp_list_ports(&ports) != SP_OK) return false;

	bool found = false;
	for (int i = 0; ports[i]; i++) {
		char *name = sp_get_port_name(ports[i]);
		bool candidate = (strstr(name, "USB") || strstr(name, "usb") ||
		strstr(name, "ACM") || strstr(name, "acm") ||
		strstr(name, "COM") || strstr(name, "slab") ||
		strstr(name, "wch"));

		if (candidate) {
			if (esp32_get_mac_from_port(name, mac_buf, buf_size)) {
				found = true;
				break;
			}
		}
	}
	sp_free_port_list(ports);
	return found;
}
