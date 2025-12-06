#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "platform.h"

#define CMD_BUFFER_SIZE	256
#define MAC_BUFFER_SIZE	18
#define MAC_PREFIX		"MAC: "
#define MAC_PREFIX_LEN	(sizeof(MAC_PREFIX) - 1)

#ifdef PLATFORM_WINDOWS
	#define PYTHON_BIN		"python"
	#define CMD_SCAN_PORTS	"powershell -Command \"[System.IO.Ports.SerialPort]::GetPortNames()\""
	#define CMD_CHECK_TOOL	"where esptool.exe >NUL 2>&1"
	#define NULL_REDIRECT	"NUL"
#elif defined(PLATFORM_LINUX)
	#define PYTHON_BIN		"python3"
	#define CMD_SCAN_PORTS	"ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null"
	#define CMD_CHECK_TOOL	"which esptool.py >/dev/null 2>&1"
	#define NULL_REDIRECT	"/dev/null"
#elif defined(PLATFORM_BSD)
	#define PYTHON_BIN		"python3"
	#define CMD_SCAN_PORTS	"ls /dev/cuaU* /dev/ttyU* 2>/dev/null"
	#define CMD_CHECK_TOOL	"which esptool.py >/dev/null 2>&1"
	#define NULL_REDIRECT	"/dev/null"
#endif

void print_usage(const char *prog_name) {
	fprintf(stderr, "[HELP]: %s [-i] -mac [PORT]\n", prog_name);
}

bool is_valid_port_format(const char *port) {
	if (strlen(port) == 0) return false;
	if (strstr(port, "..")) return false;

	for (int i = 0; port[i] != '\0'; i++) {
		if (!isalnum(port[i]) &&
			port[i] != '/' && port[i] != '\\' &&
			port[i] != '.' && port[i] != '_' && port[i] != '-') {
			return false;
		}
	}
	return true;
}

bool is_esptool_installed() {
	if (system(CMD_CHECK_TOOL) == 0) return true;

	char cmd[CMD_BUFFER_SIZE];
	snprintf(cmd, sizeof(cmd), "%s -m esptool version >%s 2>&1", PYTHON_BIN, NULL_REDIRECT);
	return system(cmd) == 0;
}

bool extract_mac_from_output(FILE *pipe, char *mac_out) {
	char buffer[CMD_BUFFER_SIZE];
	while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
		if (strncmp(buffer, MAC_PREFIX, MAC_PREFIX_LEN) == 0) {
			sscanf(buffer + MAC_PREFIX_LEN, "%17s", mac_out);
			return true;
		}
	}
	return false;
}

bool try_read_mac_from_port(const char *port, char *mac_out) {
	char command[CMD_BUFFER_SIZE];
	snprintf(command, sizeof(command),
			 "%s -m esptool --port %s read-mac 2>%s",
			 PYTHON_BIN, port, NULL_REDIRECT);

	FILE *pipe = popen(command, "r");
	if (!pipe) {
		perror("[ERRO]: 'popen()' falhou");
		return false;
	}

	bool success = extract_mac_from_output(pipe, mac_out);
	pclose(pipe);
	return success;
}

void print_result(const char *mac, bool verbose) {
	if (verbose) {
		printf("[INFO]: Mac Address Read: %s\n", mac);
	} else {
		printf("%s\n", mac);
	}
}

void scan_ports_and_find_mac(bool verbose) {
	FILE *pipe = popen(CMD_SCAN_PORTS, "r");

	if (!pipe) {
		perror("[ERRO]: Falha ao iniciar scan de portas");
		exit(1);
	}

	char port_path[CMD_BUFFER_SIZE];
	char mac_address[MAC_BUFFER_SIZE];
	bool found = false;

	while (fgets(port_path, sizeof(port_path), pipe) != NULL) {
		port_path[strcspn(port_path, "\r\n")] = 0;
		if (!is_valid_port_format(port_path)) continue;

		if (try_read_mac_from_port(port_path, mac_address)) {
			print_result(mac_address, verbose);
			found = true;
			break;
		}
	}
	pclose(pipe);

	if (!found) {
		fprintf(stderr, "[ERRO]: Nenhum dispositivo ESP32 foi encontrado.\n");
		exit(1);
	}
}

int main(int argc, char *argv[]) {
	if (!is_esptool_installed()) {
		fprintf(stderr, "[ERRO]: 'esptool' não encontrado.\n");
		return 1;
	}

	bool is_info = false;
	bool is_read = false;
	char *specified_port = NULL;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-i") == 0) {
			is_info = true;
		} else if (strcmp(argv[i], "-r") == 0) {
			is_read = true;
		} else if (strcmp(argv[i], "-h") == 0) {
			print_usage(argv[0]);
			return 0;
		} else {
			specified_port = argv[i];
		}
	}

	if (!is_read) {
		print_usage(argv[0]);
		return 1;
	}

	if (specified_port) {
		if (!is_valid_port_format(specified_port)) {
			fprintf(stderr, "[ERRO]: Formato de porta inválido ou inseguro: %s\n", specified_port);
			return 1;
		}

		char mac_address[MAC_BUFFER_SIZE];
		if (try_read_mac_from_port(specified_port, mac_address)) {
			print_result(mac_address, is_info);
		} else {
			fprintf(stderr, "[ERRO]: Falha ao obter o MAC de %s\n", specified_port);
			return 1;
		}
	} else {
		scan_ports_and_find_mac(is_info);
	}

	return 0;
}
