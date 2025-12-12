#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "libesp32.h"

static void print_help(const char *prog_name) {
	fprintf(stdout, "[HELP]: %s [-i] [-r <port>]\n", prog_name);
}

int main(int argc, char *argv[]) {
	bool verbose = false;
	bool mode_read = false;
	char *port_arg = NULL;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-i") == 0) {
			verbose = true;
		} else if (strcmp(argv[i], "-r") == 0) {
			mode_read = true;
		} else if (strcmp(argv[i], "-h") == 0) {
			print_help(argv[0]);
			return 0;
		} else {
			port_arg = argv[i];
		}
	}

	if (!mode_read) {
		print_help(argv[0]);
		return 1;
	}

	char mac_str[32];
	bool success = false;

	if (port_arg) {
		if (!esp32_check_port_format(port_arg)) {
			fprintf(stderr, "[ERRO]: Formato de porta inválido.\n");
			return 1;
		}

		if (verbose) {
			fprintf(stdout, "[INFO]: Conectando em %s...\n", port_arg);
		}
		success = esp32_get_mac_from_port(port_arg, mac_str, sizeof(mac_str));
	} else {
		if (verbose) {
			fprintf(stdout, "[INFO]: Escaneando portas disponíveis...\n");
		}
		success = esp32_find_any_mac(mac_str, sizeof(mac_str));
	}

	if (success) {
		if (verbose) {
			fprintf(stdout, "[INFO]: Dispositivo encontrado.\n");
		}
		esp32_print_mac(mac_str);
		return 0;
	}

	fprintf(stderr, "[ERRO]: Falha ao ler MAC. Verifique:\n");
	fprintf(stderr, "        1. Se o dispositivo está conectado.\n");
	fprintf(stderr, "        2. Permissões da porta (use sudo/admin).\n");
	fprintf(stderr, "        3. Segure o botão BOOT se for a primeira vez.\n");
	return 1;
}
