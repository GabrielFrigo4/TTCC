#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "libesp32.h"
#include "platform.h"

#define ESP32_CMD_BUFFER_SIZE 256
#define ESP32_MAC_BUFFER_SIZE 18
#define ESP32_MAC_PREFIX      "MAC: "
#define ESP32_MAC_PREFIX_LEN  (sizeof(ESP32_MAC_PREFIX) - 1)

#ifdef PLATFORM_WINDOWS
#define ESP32_PYTHON_BIN      "python"
#define ESP32_CMD_SCAN_PORTS  "powershell -Command \"[System.IO.Ports.SerialPort]::GetPortNames()\""
#define ESP32_CMD_CHECK_TOOL  "where esptool.exe >NUL 2>&1"
#define ESP32_NULL_REDIRECT   "NUL"
#elif defined(PLATFORM_MACOS)
#define ESP32_PYTHON_BIN      "python3"
#define ESP32_CMD_SCAN_PORTS  "ls /dev/cu.usb* /dev/cu.SLAB* /dev/cu.wch* 2>/dev/null"
#define ESP32_CMD_CHECK_TOOL  "which esptool.py >/dev/null 2>&1"
#define ESP32_NULL_REDIRECT   "/dev/null"
#elif defined(PLATFORM_LINUX)
#define ESP32_PYTHON_BIN      "python3"
#define ESP32_CMD_SCAN_PORTS  "ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null"
#define ESP32_CMD_CHECK_TOOL  "which esptool.py >/dev/null 2>&1"
#define ESP32_NULL_REDIRECT   "/dev/null"
#elif defined(PLATFORM_BSD)
#define ESP32_PYTHON_BIN      "python3"
#define ESP32_CMD_SCAN_PORTS  "ls /dev/cuaU* /dev/ttyU* 2>/dev/null"
#define ESP32_CMD_CHECK_TOOL  "which esptool.py >/dev/null 2>&1"
#define ESP32_NULL_REDIRECT   "/dev/null"
#endif

static bool internal_parse_pipe(FILE *pipe, char *out_buf, size_t size) {
	char line[ESP32_CMD_BUFFER_SIZE];
	while (fgets(line, sizeof(line), pipe)) {
		if (strncmp(line, ESP32_MAC_PREFIX, ESP32_MAC_PREFIX_LEN) == 0) {
			char *mac_start = line + ESP32_MAC_PREFIX_LEN;

			while (*mac_start && isspace((unsigned char)*mac_start)) {
				mac_start++;
			}
			mac_start[strcspn(mac_start, "\r\n")] = 0;

			if (out_buf && size > 0) {
				snprintf(out_buf, size, "%s", mac_start);
			}
			return true;
		}
	}
	return false;
}

bool esp32_check_port_format(const char *port) {
	if (!port || strlen(port) == 0 || strstr(port, "..")) {
		return false;
	}
	for (int i = 0; port[i]; i++) {
		if (!isalnum(port[i]) && strchr("/\\._-", port[i]) == NULL) {
			return false;
		}
	}
	return true;
}

bool esp32_check_esptool(void) {
	if (system(ESP32_CMD_CHECK_TOOL) == 0) {
		return true;
	}
	char cmd[ESP32_CMD_BUFFER_SIZE];
	snprintf(cmd, sizeof(cmd), "%s -m esptool version >%s 2>&1", ESP32_PYTHON_BIN, ESP32_NULL_REDIRECT);
	return system(cmd) == 0;
}

bool esp32_get_mac_from_port(const char *port, char *mac_buf, size_t buf_size) {
	char cmd[ESP32_CMD_BUFFER_SIZE];
	snprintf(cmd, sizeof(cmd), "%s -m esptool --port %s read-mac 2>%s",
		ESP32_PYTHON_BIN, port, ESP32_NULL_REDIRECT);

	FILE *pipe = popen(cmd, "r");
	if (!pipe) {
		return false;
	}

	bool found = internal_parse_pipe(pipe, mac_buf, buf_size);
	pclose(pipe);
	return found;
}

bool esp32_find_any_mac(char *mac_buf, size_t buf_size) {
	FILE *pipe = popen(ESP32_CMD_SCAN_PORTS, "r");
	if (!pipe) {
		return false;
	}

	char port[ESP32_CMD_BUFFER_SIZE];
	bool found = false;

	while (fgets(port, sizeof(port), pipe)) {
		port[strcspn(port, "\r\n")] = 0;
		if (!esp32_check_port_format(port)) {
			continue;
		}

		if (esp32_get_mac_from_port(port, mac_buf, buf_size)) {
			found = true;
			break;
		}
	}
	pclose(pipe);
	return found;
}

void esp32_print_mac(const char *mac_str) {
	if (mac_str) {
		printf("%s\n", mac_str);
	}
}
