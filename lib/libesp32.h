#ifndef LIBESP32_H
#define LIBESP32_H

#include <stdbool.h>
#include <stddef.h>

bool esp32_check_port_format(const char *port);
bool esp32_get_mac_from_port(const char *port, char *mac_buf, size_t buf_size);
bool esp32_find_any_mac(char *mac_buf, size_t buf_size);
void esp32_print_mac(const char *mac_str);

#endif
