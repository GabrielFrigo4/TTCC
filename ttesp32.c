#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#define CMD_BUFFER_SIZE 256
#define MAC_BUFFER_SIZE 18
#define MAC_PREFIX "MAC: "
#define MAC_PREFIX_LEN (sizeof(MAC_PREFIX) - 1)

bool is_esptool_installed() {
	return system("which esptool.py >/dev/null 2>&1") == 0;
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
	snprintf(command, sizeof(command), "python -m esptool --port %s read-mac 2>/dev/null", port);

	FILE *pipe = popen(command, "r");
	if (!pipe) {
		perror("[ERRO]: 'popen()' falhou");
		return false;
	}

	bool success = extract_mac_from_output(pipe, mac_out);
	pclose(pipe);
	return success;
}

void scan_ports_and_find_mac() {
	FILE *pipe = popen("ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null", "r");
	if (!pipe) {
		perror("[ERRO]: Falha ao listar portas");
		exit(1);
	}

	char port_path[CMD_BUFFER_SIZE];
	char mac_address[MAC_BUFFER_SIZE];
	bool found = false;

	while (fgets(port_path, sizeof(port_path), pipe) != NULL) {
		port_path[strcspn(port_path, "\n")] = 0;

		if (try_read_mac_from_port(port_path, mac_address)) {
			printf("%s\n", mac_address);
			found = true;
			break;
		}
	}

	pclose(pipe);

	if (!found) {
		fprintf(stderr, "[ERRO]: Nenhum dispositivo ESP32 foi encontrado ou respondeu\n");
		exit(1);
	}
}

int main(int argc, char *argv[]) {
	if (!is_esptool_installed()) {
		fprintf(stderr, "[ERRO]: 'esptool.py' não está instalado ou não se encontra no PATH\n");
		return 1;
	}

	if (argc > 1) {
		char mac_address[MAC_BUFFER_SIZE];
		if (try_read_mac_from_port(argv[1], mac_address)) {
			printf("%s\n", mac_address);
		} else {
			fprintf(stderr, "[ERRO]: Falha ao obter o MAC de %s\n", argv[1]);
			return 1;
		}
	} else {
		scan_ports_and_find_mac();
	}

	return 0;
}
