#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libusb.h>
#include "libds4.h"
#include "platform.h"

#define DS4_VENDOR_ID       0x054C
#define DS4_PRODUCT_ID_GEN1 0x05C4
#define DS4_PRODUCT_ID_GEN2 0x09CC
#define DS4_USB_TIMEOUT_MS  5000

#define DS4_REP_TYPE_FEAT   0x03
#define DS4_REP_ID_PAIRING  0x12
#define DS4_REP_ID_STD      0x05
#define DS4_REP_ID_WRITE    0x13

#define DS4_REQ_GET_REP     0x01
#define DS4_REQ_SET_REP     0x09
#define DS4_DIR_IN          0x80
#define DS4_DIR_OUT         0x00
#define DS4_TYPE_CLASS      0x20
#define DS4_RECIP_IFACE     0x01

#define DS4_HID_GET         (DS4_DIR_IN | DS4_TYPE_CLASS | DS4_RECIP_IFACE)
#define DS4_HID_SET         (DS4_DIR_OUT | DS4_TYPE_CLASS | DS4_RECIP_IFACE)

struct ds4_context {
	libusb_context *usb_ctx;
	libusb_device_handle *handle;
};

static void internal_reverse_array(const uint8_t *src, uint8_t *dest, int len) {
	for (int i = 0; i < len; i++) {
		dest[i] = src[len - 1 - i];
	}
}

ds4_context_t* ds4_create_context(void) {
	ds4_context_t *ctx = calloc(1, sizeof(ds4_context_t));
	if (!ctx) {
		return NULL;
	}

	if (libusb_init(&ctx->usb_ctx) < 0) {
		free(ctx);
		return NULL;
	}

	ctx->handle = libusb_open_device_with_vid_pid(ctx->usb_ctx, DS4_VENDOR_ID, DS4_PRODUCT_ID_GEN1);
	if (!ctx->handle) {
		ctx->handle = libusb_open_device_with_vid_pid(ctx->usb_ctx, DS4_VENDOR_ID, DS4_PRODUCT_ID_GEN2);
	}

	if (!ctx->handle) {
		libusb_exit(ctx->usb_ctx);
		free(ctx);
		return NULL;
	}

#ifdef PLATFORM_LINUX
	if (libusb_kernel_driver_active(ctx->handle, 0) == 1) {
		libusb_detach_kernel_driver(ctx->handle, 0);
	}
#endif
	libusb_claim_interface(ctx->handle, 0);

	return ctx;
}

void ds4_destroy_context(ds4_context_t *ctx) {
	if (!ctx) {
		return;
	}
	if (ctx->handle) {
		libusb_release_interface(ctx->handle, 0);
		libusb_close(ctx->handle);
	}
	if (ctx->usb_ctx) {
		libusb_exit(ctx->usb_ctx);
	}
	free(ctx);
}

bool ds4_get_mac(ds4_context_t *ctx, uint8_t *mac_out) {
	if (!ctx || !ctx->handle || !mac_out) {
		return false;
	}

	unsigned char buf[65];
	int transferred;
	uint16_t wValue;

	memset(buf, 0, sizeof(buf));
	wValue = (DS4_REP_TYPE_FEAT << 8) | DS4_REP_ID_PAIRING;
	transferred = libusb_control_transfer(ctx->handle, DS4_HID_GET, DS4_REQ_GET_REP,
					wValue, 0, buf, sizeof(buf), DS4_USB_TIMEOUT_MS);

	if (transferred > 15) {
		internal_reverse_array(&buf[10], mac_out, DS4_MAC_ADDR_LEN);
		return true;
	}

	memset(buf, 0, sizeof(buf));
	wValue = (DS4_REP_TYPE_FEAT << 8) | DS4_REP_ID_STD;
	transferred = libusb_control_transfer(ctx->handle, DS4_HID_GET, DS4_REQ_GET_REP,
					wValue, 0, buf, sizeof(buf), DS4_USB_TIMEOUT_MS);

	if (transferred > 6) {
		memcpy(mac_out, &buf[1], DS4_MAC_ADDR_LEN);
		return true;
	}

	return false;
}

bool ds4_set_mac(ds4_context_t *ctx, const uint8_t *mac_in) {
	if (!ctx || !ctx->handle || !mac_in) {
		return false;
	}

	unsigned char buf[32];
	memset(buf, 0, sizeof(buf));

	buf[0] = DS4_REP_ID_WRITE;
	internal_reverse_array(mac_in, &buf[1], DS4_MAC_ADDR_LEN);

	uint16_t wValue = (DS4_REP_TYPE_FEAT << 8) | DS4_REP_ID_WRITE;
	int res = libusb_control_transfer(ctx->handle, DS4_HID_SET, DS4_REQ_SET_REP,
				wValue, 0, buf, sizeof(buf), DS4_USB_TIMEOUT_MS);

	return (res >= 0);
}

void ds4_mac_to_string(const uint8_t *mac_raw, char *str_out) {
	if (!mac_raw || !str_out) {
		return;
	}
	sprintf(str_out, "%02X:%02X:%02X:%02X:%02X:%02X",
		mac_raw[0], mac_raw[1], mac_raw[2],
		mac_raw[3], mac_raw[4], mac_raw[5]);
}

bool ds4_string_to_mac(const char *str_in, uint8_t *mac_out) {
	if (!str_in || !mac_out) {
		return false;
	}
	int parsed = sscanf(str_in, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
					&mac_out[0], &mac_out[1], &mac_out[2],
					&mac_out[3], &mac_out[4], &mac_out[5]);
	return (parsed == DS4_MAC_ADDR_LEN);
}

void ds4_print_mac(const uint8_t *mac_raw) {
	char temp[18];
	ds4_mac_to_string(mac_raw, temp);
	printf("%s\n", temp);
}

bool ds4_scan_mac(uint8_t *mac_out) {
	char buffer[32];
	if (scanf("%31s", buffer) != 1) {
		return false;
	}
	return ds4_string_to_mac(buffer, mac_out);
}
