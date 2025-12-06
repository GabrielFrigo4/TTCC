#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <libusb.h>
#include "platform.h"

#define DS4_VENDOR_ID		0x054C
#define DS4_PRODUCT_ID_GEN1	0x05C4
#define DS4_PRODUCT_ID_GEN2	0x09CC

#define USB_TIMEOUT_MS		5000
#define MAC_ADDR_LEN		6

#define REPORT_TYPE_FEATURE	0x03
#define REPORT_ID_PAIRING	0x12
#define REPORT_ID_STANDARD	0x05
#define REPORT_ID_WRITE		0x13

#define USB_REQ_GET_REPORT	0x01
#define USB_REQ_SET_REPORT	0x09

#define USB_DIR_IN			0x80
#define USB_DIR_OUT			0x00
#define USB_TYPE_CLASS		0x20
#define USB_RECIP_INTERFACE	0x01

#define HID_GET_REPORT		(USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE)
#define HID_SET_REPORT		(USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE)

void print_usage(const char *prog_name) {
	fprintf(stderr, "[HELP]: %s [-i] [-r | -w <mac>]\n", prog_name);
}

bool parse_mac_string(const char *mac_str, unsigned char *mac_out) {
	int parsed = sscanf(mac_str, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
					&mac_out[0], &mac_out[1], &mac_out[2],
					&mac_out[3], &mac_out[4], &mac_out[5]);
	return (parsed == MAC_ADDR_LEN);
}

bool read_mac_from_stdin(unsigned char *mac_out) {
	int parsed = scanf("%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
					&mac_out[0], &mac_out[1], &mac_out[2],
					&mac_out[3], &mac_out[4], &mac_out[5]);
	return (parsed == MAC_ADDR_LEN);
}

libusb_device_handle* get_device_handle(libusb_context *ctx) {
	libusb_device_handle *handle = libusb_open_device_with_vid_pid(ctx, DS4_VENDOR_ID, DS4_PRODUCT_ID_GEN1);
	if (!handle) {
		handle = libusb_open_device_with_vid_pid(ctx, DS4_VENDOR_ID, DS4_PRODUCT_ID_GEN2);
	}
	return handle;
}

void prepare_device(libusb_device_handle *handle) {
	#ifdef PLATFORM_LINUX
		if (libusb_kernel_driver_active(handle, 0) == 1) {
			libusb_detach_kernel_driver(handle, 0);
		}
	#endif
	libusb_claim_interface(handle, 0);
}

void print_mac_address(const unsigned char *mac, const char *label, bool verbose) {
	if (verbose) {
		printf("%s %02X:%02X:%02X:%02X:%02X:%02X\n", label,
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	} else {
		printf("%02X:%02X:%02X:%02X:%02X:%02X\n",
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	}
}

void reverse_array(const unsigned char *src, unsigned char *dest, int len) {
	for (int i = 0; i < len; i++) {
		dest[i] = src[len - 1 - i];
	}
}

void execute_read_mac(libusb_device_handle *handle, bool verbose) {
	unsigned char buf[65];
	int transferred;
	uint16_t wValue;

	memset(buf, 0, sizeof(buf));
	wValue = (REPORT_TYPE_FEATURE << 8) | REPORT_ID_PAIRING;
	transferred = libusb_control_transfer(handle, HID_GET_REPORT, USB_REQ_GET_REPORT,
					wValue, 0, buf, sizeof(buf), USB_TIMEOUT_MS);

	if (transferred > 15) {
		unsigned char mac[MAC_ADDR_LEN];
		reverse_array(&buf[10], mac, MAC_ADDR_LEN);
		print_mac_address(mac, "[INFO]: Atual Master MAC:", verbose);
		return;
	}

	memset(buf, 0, sizeof(buf));
	wValue = (REPORT_TYPE_FEATURE << 8) | REPORT_ID_STANDARD;
	transferred = libusb_control_transfer(handle, HID_GET_REPORT, USB_REQ_GET_REPORT,
					wValue, 0, buf, sizeof(buf), USB_TIMEOUT_MS);

	if (transferred > 6) {
		print_mac_address(&buf[1], "[INFO]: Atual Master MAC (STD):", verbose);
		return;
	}

	fprintf(stderr, "[ERRO]: Não foi possível ler o MAC.\n");
}

void execute_write_mac(libusb_device_handle *handle, const unsigned char *mac, bool verbose) {
	unsigned char buf[32];
	memset(buf, 0, sizeof(buf));

	buf[0] = REPORT_ID_WRITE;
	reverse_array(mac, &buf[1], MAC_ADDR_LEN);

	uint16_t wValue = (REPORT_TYPE_FEATURE << 8) | REPORT_ID_WRITE;
	int res = libusb_control_transfer(handle, HID_SET_REPORT, USB_REQ_SET_REPORT,
				wValue, 0, buf, sizeof(buf), USB_TIMEOUT_MS);

	if (res < 0) {
		fprintf(stderr, "[ERRO]: Falha na escrita: %s\n", libusb_error_name(res));
	} else if (verbose) {
		print_mac_address(mac, "[INFO]: Novo Master MAC:", verbose);
	}
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		print_usage(argv[0]);
		return 1;
	}

	bool is_info = false;
	bool is_read = false;
	bool is_write = false;
	char *mac_str_arg = NULL;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-i") == 0) {
			is_info = true;
		} else if (strcmp(argv[i], "-r") == 0) {
			is_read = true;
		} else if (strcmp(argv[i], "-w") == 0) {
			is_write = true;
		} else if (strcmp(argv[i], "-h") == 0) {
			print_usage(argv[0]);
			return 0;
		} else {
			mac_str_arg = argv[i];
		}
	}

	if (!is_read && !is_write) {
		print_usage(argv[0]);
		return 1;
	}

	if (is_read && is_write) {
		fprintf(stderr, "[ERRO]: Selecione apenas -r ou -w\n");
		return 1;
	}

	unsigned char mac_buffer[MAC_ADDR_LEN];
	if (is_write) {
		bool valid_mac = false;
		if (mac_str_arg) {
			valid_mac = parse_mac_string(mac_str_arg, mac_buffer);
		} else {
			valid_mac = read_mac_from_stdin(mac_buffer);
		}

		if (!valid_mac) {
			fprintf(stderr, "[ERRO]: Formato inválido. Use AA:BB:CC:DD:EE:FF\n");
			return 1;
		}
	}

	libusb_context *ctx = NULL;
	if (libusb_init(&ctx) < 0) {
		fprintf(stderr, "[ERRO]: Falha ao inicializar 'libusb'\n");
		return 1;
	}

	libusb_device_handle *handle = get_device_handle(ctx);
	if (!handle) {
		fprintf(stderr, "[ERRO]: Nenhum controle encontrado. (Windows: Verifique driver WinUSB via Zadig. Linux: use sudo)\n");
		libusb_exit(ctx);
		return 1;
	}

	prepare_device(handle);

	if (is_read) {
		execute_read_mac(handle, is_info);
	} else {
		execute_write_mac(handle, mac_buffer, is_info);
	}

	libusb_release_interface(handle, 0);
	libusb_close(handle);
	libusb_exit(ctx);
	return 0;
}
