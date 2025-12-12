#ifndef LIBDS4_H
#define LIBDS4_H

#include <stdbool.h>
#include <stdint.h>

#define DS4_MAC_ADDR_LEN 6

typedef struct ds4_context ds4_context_t;

ds4_context_t* ds4_create_context(void);
void ds4_destroy_context(ds4_context_t *ctx);

bool ds4_get_mac(ds4_context_t *ctx, uint8_t *mac_out);
bool ds4_set_mac(ds4_context_t *ctx, const uint8_t *mac_in);

void ds4_mac_to_string(const uint8_t *mac_raw, char *str_out);
bool ds4_string_to_mac(const char *str_in, uint8_t *mac_out);

void ds4_print_mac(const uint8_t *mac_raw);
bool ds4_scan_mac(uint8_t *mac_out);

#endif
