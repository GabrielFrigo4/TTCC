#include <stdio.h>
#include <string.h>
#include "libds4.h"

static void print_help(const char *prog_name) {
	fprintf(stderr, "[HELP]: %s [-i] [-r | -w <mac>]\n", prog_name);
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		print_help(argv[0]);
		return 1;
	}

	bool verbose = false;
	bool mode_read = false;
	bool mode_write = false;
	char *mac_arg = NULL;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-i") == 0) {
			verbose = true;
		} else if (strcmp(argv[i], "-r") == 0) {
			mode_read = true;
		} else if (strcmp(argv[i], "-w") == 0) {
			mode_write = true;
		} else if (strcmp(argv[i], "-h") == 0) {
			print_help(argv[0]);
			return 0;
		} else {
			mac_arg = argv[i];
		}
	}

	if (mode_read == mode_write) {
		fprintf(stderr, "[ERRO]: Escolha -r OU -w\n");
		return 1;
	}

	uint8_t mac_bytes[DS4_MAC_ADDR_LEN];

	if (mode_write) {
		bool ok = (mac_arg ? ds4_string_to_mac(mac_arg, mac_bytes) : ds4_scan_mac(mac_bytes));

		if (!ok) {
			fprintf(stderr, "[ERRO]: MAC inválido.\n");
			return 1;
		}
	}

	ds4_context_t *ctx = ds4_create_context();
	if (!ctx) {
		fprintf(stderr, "[ERRO]: Falha ao conectar ao controle.\n");
		return 1;
	}

	int exit_code = 0;

	if (mode_read) {
		if (ds4_get_mac(ctx, mac_bytes)) {
			if (verbose) {
				printf("[INFO]: MAC Lido: ");
			}
			ds4_print_mac(mac_bytes);
		} else {
			fprintf(stderr, "[ERRO]: Falha ao ler MAC.\n");
			exit_code = 1;
		}
	} else {
		if (ds4_set_mac(ctx, mac_bytes)) {
			if (verbose) {
				printf("[INFO]: MAC Gravado: ");
				ds4_print_mac(mac_bytes);
			}
		} else {
			fprintf(stderr, "[ERRO]: Falha ao gravar MAC.\n");
			exit_code = 1;
		}
	}

	ds4_destroy_context(ctx);
	return exit_code;
}