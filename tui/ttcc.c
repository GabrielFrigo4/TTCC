#define TB_IMPL
#include "../cross/termbox2.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "libds4.h"
#include "libesp32.h"

#define ICON_GAMEPAD    "\uf11b" // 
#define ICON_CHIP       "\uf2db" // 
#define ICON_USB        "\uf287" // 
#define ICON_KEYBOARD   "\uf11c" // 
#define ICON_SYNC       "\uf021" // 
#define ICON_UPLOAD     "\uf093" // 
#define ICON_EXIT       "\uf011" // 
#define ICON_CHECK      "\uf00c" // 
#define ICON_ERROR      "\uf071" // 
#define ICON_MOUSE      "\uf245" // 

#define COL_BG          TB_DEFAULT
#define COL_FG          TB_WHITE
#define COL_ACCENT      TB_CYAN
#define COL_BTN_IDLE    TB_BLUE
#define COL_BTN_HOVER   TB_MAGENTA
#define COL_BTN_PRESS   TB_GREEN
#define COL_TEXT_DARK   TB_BLACK
#define COL_EDIT_MODE   TB_YELLOW

typedef enum {
	BTN_SCAN_DS4,
	BTN_SCAN_ESP,
	BTN_MANUAL,
	BTN_PAIR,
	BTN_EXIT,
	BTN_COUNT
} ButtonId;

typedef struct {
	int x, y, w, h;
	const char *label;
	const char *icon;
} Button;

typedef struct {
	char ds4_mac[32];
	char esp_mac[32];
	char status[128];
	uint64_t status_color;

	bool ds4_ok;
	bool esp_ok;
	bool is_editing;

	Button buttons[BTN_COUNT];
	int selected_idx;
	int pressed_btn_idx;
	bool use_mouse;
	bool running;
} AppState;

// --- Actions ---

void action_scan_ds4(AppState *s) {
	ds4_context_t *ctx = ds4_create_context();

	if (!ctx) {
		snprintf(s->status, sizeof(s->status), "%s Erro: Controle DS4 não detectado via USB.", ICON_ERROR);
		s->status_color = TB_RED;
		s->ds4_ok = false;
		return;
	}

	uint8_t raw[6];
	if (ds4_get_mac(ctx, raw)) {
		ds4_mac_to_string(raw, s->ds4_mac);
		s->ds4_ok = true;
		snprintf(s->status, sizeof(s->status), "%s Sucesso: DS4 MAC lido.", ICON_CHECK);
		s->status_color = TB_GREEN;
	} else {
		s->ds4_ok = false;
		snprintf(s->status, sizeof(s->status), "%s Erro: Falha ao ler dados do DS4.", ICON_ERROR);
		s->status_color = TB_RED;
	}

	ds4_destroy_context(ctx);
}

void action_scan_esp(AppState *s) {
	if (!esp32_check_esptool()) {
		snprintf(s->status, sizeof(s->status), "%s Erro: esptool (Python) não encontrado.", ICON_ERROR);
		s->status_color = TB_RED;
		s->esp_ok = false;
		return;
	}

	snprintf(s->status, sizeof(s->status), "%s Escaneando portas seriais...", ICON_SYNC);
	s->status_color = TB_YELLOW;

	if (esp32_find_any_mac(s->esp_mac, sizeof(s->esp_mac))) {
		s->esp_ok = true;
		snprintf(s->status, sizeof(s->status), "%s Sucesso: ESP32 encontrado.", ICON_CHECK);
		s->status_color = TB_GREEN;
	} else {
		s->esp_ok = false;
		snprintf(s->status, sizeof(s->status), "%s Erro: Nenhum ESP32 encontrado.", ICON_ERROR);
		s->status_color = TB_RED;
	}
}

void action_manual_input(AppState *s) {
	s->is_editing = true;
	s->esp_ok = false;
	memset(s->esp_mac, 0, sizeof(s->esp_mac));
	snprintf(s->status, sizeof(s->status), "DIGITE O MAC (0-9, A-F). ENTER confirma.");
	s->status_color = TB_YELLOW;
}

void action_pair(AppState *s) {
	if (!s->esp_ok) {
		snprintf(s->status, sizeof(s->status), "%s Leia o ESP32 primeiro ou digite manual.", ICON_ERROR);
		s->status_color = TB_RED;
		return;
	}

	ds4_context_t *ctx = ds4_create_context();
	if (!ctx) {
		snprintf(s->status, sizeof(s->status), "%s Conecte o DS4 via USB.", ICON_USB);
		s->status_color = TB_RED;
		return;
	}

	uint8_t target[6];
	if (!ds4_string_to_mac(s->esp_mac, target)) {
		snprintf(s->status, sizeof(s->status), "%s Formato MAC inválido.", ICON_ERROR);
		s->status_color = TB_RED;
		ds4_destroy_context(ctx);
		return;
	}

	if (ds4_set_mac(ctx, target)) {
		ds4_get_mac(ctx, target);
		ds4_mac_to_string(target, s->ds4_mac);
		s->ds4_ok = true;
		snprintf(s->status, sizeof(s->status), "%s PAREADO! O DS4 agora busca o ESP32.", ICON_CHECK);
		s->status_color = TB_GREEN;
	} else {
		snprintf(s->status, sizeof(s->status), "%s Erro ao gravar no DS4.", ICON_ERROR);
		s->status_color = TB_RED;
	}

	ds4_destroy_context(ctx);
}

void trigger_button_action(AppState *s, int btn_idx) {
	switch (btn_idx) {
		case BTN_SCAN_DS4:
			action_scan_ds4(s);
			break;
		case BTN_SCAN_ESP:
			action_scan_esp(s);
			break;
		case BTN_MANUAL:
			action_manual_input(s);
			break;
		case BTN_PAIR:
			action_pair(s);
			break;
		case BTN_EXIT:
			s->running = false;
			break;
	}
}

// --- Layout & Init ---

void init_state(AppState *s) {
	memset(s, 0, sizeof(AppState));
	strcpy(s->ds4_mac, "--:--:--:--:--:--");
	strcpy(s->esp_mac, "--:--:--:--:--:--");
	strcpy(s->status, "Aguardando ação...");

	s->status_color = TB_DEFAULT;
	s->running = true;
	s->pressed_btn_idx = -1;
	s->is_editing = false;

	int start_y = 13;
	int btn_w = 26;

	// Linha 1: Scanners
	s->buttons[BTN_SCAN_DS4].label = "Ler DS4 (USB)";
	s->buttons[BTN_SCAN_DS4].icon = ICON_GAMEPAD;
	s->buttons[BTN_SCAN_DS4].w = btn_w;
	s->buttons[BTN_SCAN_DS4].h = 3;
	s->buttons[BTN_SCAN_DS4].y = start_y;

	s->buttons[BTN_SCAN_ESP].label = "Ler ESP32";
	s->buttons[BTN_SCAN_ESP].icon = ICON_CHIP;
	s->buttons[BTN_SCAN_ESP].w = btn_w;
	s->buttons[BTN_SCAN_ESP].h = 3;
	s->buttons[BTN_SCAN_ESP].y = start_y;

	// Linha 2: Ações
	s->buttons[BTN_PAIR].label = "GRAVAR / PAREAR";
	s->buttons[BTN_PAIR].icon = ICON_UPLOAD;
	s->buttons[BTN_PAIR].w = btn_w;
	s->buttons[BTN_PAIR].h = 3;
	s->buttons[BTN_PAIR].y = start_y + 4;

	s->buttons[BTN_MANUAL].label = "Manual Input";
	s->buttons[BTN_MANUAL].icon = ICON_KEYBOARD;
	s->buttons[BTN_MANUAL].w = btn_w;
	s->buttons[BTN_MANUAL].h = 3;
	s->buttons[BTN_MANUAL].y = start_y + 4;

	// Linha 3: Exit
	s->buttons[BTN_EXIT].label = "Sair";
	s->buttons[BTN_EXIT].icon = ICON_EXIT;
	s->buttons[BTN_EXIT].w = 20;
	s->buttons[BTN_EXIT].h = 3;
	s->buttons[BTN_EXIT].y = start_y + 8;
}

void update_layout(AppState *s) {
	int w = tb_width();
	int cx = w / 2;

	s->buttons[BTN_SCAN_DS4].x = cx - s->buttons[BTN_SCAN_DS4].w - 1;
	s->buttons[BTN_PAIR].x     = cx - s->buttons[BTN_PAIR].w - 1;

	s->buttons[BTN_SCAN_ESP].x = cx + 1;
	s->buttons[BTN_MANUAL].x   = cx + 1;

	s->buttons[BTN_EXIT].x     = cx - (s->buttons[BTN_EXIT].w / 2);
}

// --- Drawing ---

void draw_rect_filled(int x, int y, int w, int h, uint64_t bg) {
	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			tb_set_cell(x + j, y + i, ' ', TB_DEFAULT, bg);
		}
	}
}

void draw_button(Button *b, bool is_selected, bool is_pressed) {
	uint64_t bg = COL_BTN_IDLE;
	if (is_pressed) {
		bg = COL_BTN_PRESS;
	} else if (is_selected) {
		bg = COL_BTN_HOVER;
	}

	uint64_t fg = (is_selected || is_pressed) ? COL_TEXT_DARK : COL_FG;

	if (!is_pressed) {
		draw_rect_filled(b->x + 1, b->y + 1, b->w, b->h, TB_BLACK);
	}

	draw_rect_filled(b->x, b->y, b->w, b->h, bg);

	if (is_selected || is_pressed) {
		uint64_t border_fg = TB_WHITE | TB_BOLD;
		tb_set_cell(b->x, b->y, 0x250C, border_fg, bg);
		tb_set_cell(b->x + b->w - 1, b->y, 0x2510, border_fg, bg);
		tb_set_cell(b->x, b->y + b->h - 1, 0x2514, border_fg, bg);
		tb_set_cell(b->x + b->w - 1, b->y + b->h - 1, 0x2518, border_fg, bg);
	}

	int text_len = strlen(b->label) + 3;
	int tx = b->x + (b->w - text_len) / 2;
	int ty = b->y + (b->h / 2);

	tb_printf(tx, ty, fg | TB_BOLD, bg, "%s %s", b->icon, b->label);
}

void draw_panel(int x, int y, int w, int h, const char *title, const char *mac, bool active, bool editing) {
	uint64_t color = editing ? COL_EDIT_MODE : (active ? COL_ACCENT : TB_DEFAULT);

	tb_set_cell(x, y, 0x256D, color, TB_DEFAULT); // ╭
	tb_set_cell(x + w - 1, y, 0x256E, color, TB_DEFAULT); // ╮
	tb_set_cell(x, y + h - 1, 0x2570, color, TB_DEFAULT); // ╰
	tb_set_cell(x + w - 1, y + h - 1, 0x256F, color, TB_DEFAULT); // ╯

	for (int i = 1; i < w - 1; i++) {
		tb_set_cell(x + i, y, 0x2500, color, TB_DEFAULT);
		tb_set_cell(x + i, y + h - 1, 0x2500, color, TB_DEFAULT);
	}

	for (int i = 1; i < h - 1; i++) {
		tb_set_cell(x, y + i, 0x2502, color, TB_DEFAULT);
		tb_set_cell(x + w - 1, y + i, 0x2502, color, TB_DEFAULT);
	}

	tb_printf(x + 2, y, color | TB_BOLD, TB_DEFAULT, " %s ", title);

	int mac_x = x + (w - 17) / 2;
	uint64_t mac_fg = editing ? (TB_YELLOW | TB_BOLD) : (active ? (TB_GREEN | TB_BOLD) : TB_DEFAULT);

	tb_printf(mac_x, y + 2, mac_fg, TB_DEFAULT, "%s%s", mac, editing ? "_" : "");
}

void render(AppState *s) {
	tb_clear();
	update_layout(s);

	int w = tb_width();
	int cx = w / 2;

	const char *title = ICON_GAMEPAD "  PAIR TOOL " ICON_CHIP;
	tb_printf(cx - 8, 2, COL_ACCENT | TB_BOLD, TB_DEFAULT, "%s", title);

	draw_panel(cx - 28, 5, 26, 5, "DualShock 4", s->ds4_mac, s->ds4_ok, false);
	draw_panel(cx + 2, 5, 26, 5, "ESP32 Device", s->esp_mac, s->esp_ok, s->is_editing);

	tb_printf(cx - 1, 7, TB_WHITE, TB_DEFAULT, "\uf060");

	for (int i = 0; i < BTN_COUNT; i++) {
		draw_button(&s->buttons[i], (s->selected_idx == i), (s->pressed_btn_idx == i));
	}

	int h = tb_height();
	draw_rect_filled(0, h - 2, w, 2, TB_BLACK);
	tb_printf(2, h - 1, s->status_color | TB_BOLD, TB_BLACK, "%s", s->status);

	char help[64];
	if (s->is_editing) {
		snprintf(help, sizeof(help), "ENTER: Confirmar | ESC: Cancelar");
	} else {
		snprintf(help, sizeof(help), "%s Click or Enter to Select | %s Quit", ICON_MOUSE, ICON_EXIT);
	}
	tb_printf(w - strlen(help) + 1, h - 1, TB_WHITE, TB_BLACK, "%s", help);

	tb_present();
}

// --- Input Handling ---

void handle_text_input(AppState *s, struct tb_event *ev) {
	if (ev->key == TB_KEY_ENTER) {
		uint8_t dummy[6];
		if (ds4_string_to_mac(s->esp_mac, dummy)) {
			s->is_editing = false;
			s->esp_ok = true;
			snprintf(s->status, sizeof(s->status), "MAC Manual definido.");
			s->status_color = TB_GREEN;
		} else {
			snprintf(s->status, sizeof(s->status), "Formato Invalido (AA:BB:CC:DD:EE:FF)");
			s->status_color = TB_RED;
		}
		return;
	}

	if (ev->key == TB_KEY_ESC) {
		s->is_editing = false;
		strcpy(s->esp_mac, "--:--:--:--:--:--");
		snprintf(s->status, sizeof(s->status), "Edição cancelada.");
		s->status_color = TB_DEFAULT;
		return;
	}

	if (ev->key == TB_KEY_BACKSPACE || ev->key == TB_KEY_BACKSPACE2) {
		int len = strlen(s->esp_mac);
		if (len > 0) {
			s->esp_mac[len - 1] = '\0';
		}
		return;
	}

	if (ev->ch) {
		char c = toupper((char)ev->ch);
		int len = strlen(s->esp_mac);
		if (len < 17 && ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || c == ':')) {
			s->esp_mac[len] = c;
			s->esp_mac[len + 1] = '\0';
		}
	}
}

void handle_navigation(AppState *s, struct tb_event *ev) {
	s->use_mouse = false;
	s->pressed_btn_idx = -1;

	if (ev->key == TB_KEY_ESC || ev->ch == 'q') {
		s->running = false;
		return;
	}

	if (ev->key == TB_KEY_ARROW_RIGHT) {
		if (s->selected_idx == BTN_SCAN_DS4) s->selected_idx = BTN_SCAN_ESP;
		else if (s->selected_idx == BTN_PAIR) s->selected_idx = BTN_MANUAL;
		else if (s->selected_idx == BTN_SCAN_ESP) s->selected_idx = BTN_SCAN_DS4;
		else if (s->selected_idx == BTN_MANUAL) s->selected_idx = BTN_PAIR;
	}
	else if (ev->key == TB_KEY_ARROW_LEFT) {
		if (s->selected_idx == BTN_SCAN_ESP) s->selected_idx = BTN_SCAN_DS4;
		else if (s->selected_idx == BTN_MANUAL) s->selected_idx = BTN_PAIR;
		else if (s->selected_idx == BTN_SCAN_DS4) s->selected_idx = BTN_SCAN_ESP;
		else if (s->selected_idx == BTN_PAIR) s->selected_idx = BTN_MANUAL;
	}
	else if (ev->key == TB_KEY_ARROW_DOWN || ev->key == TB_KEY_TAB) {
		if (s->selected_idx == BTN_SCAN_DS4) s->selected_idx = BTN_PAIR;
		else if (s->selected_idx == BTN_SCAN_ESP) s->selected_idx = BTN_MANUAL;
		else if (s->selected_idx == BTN_PAIR) s->selected_idx = BTN_EXIT;
		else if (s->selected_idx == BTN_MANUAL) s->selected_idx = BTN_EXIT;
		else if (s->selected_idx == BTN_EXIT) s->selected_idx = BTN_SCAN_DS4;
	}
	else if (ev->key == TB_KEY_ARROW_UP) {
		if (s->selected_idx == BTN_EXIT) s->selected_idx = BTN_PAIR;
		else if (s->selected_idx == BTN_PAIR) s->selected_idx = BTN_SCAN_DS4;
		else if (s->selected_idx == BTN_MANUAL) s->selected_idx = BTN_SCAN_ESP;
		else if (s->selected_idx == BTN_SCAN_DS4) s->selected_idx = BTN_EXIT;
		else if (s->selected_idx == BTN_SCAN_ESP) s->selected_idx = BTN_EXIT;
	}
	else if (ev->key == TB_KEY_ENTER || ev->key == TB_KEY_SPACE) {
		s->pressed_btn_idx = s->selected_idx;
		render(s);
		usleep(100000);
		s->pressed_btn_idx = -1;
		trigger_button_action(s, s->selected_idx);
	}
}

void handle_mouse(AppState *s, struct tb_event *ev) {
	if (s->is_editing) {
		return;
	}

	s->use_mouse = true;
	int mx = ev->x;
	int my = ev->y;
	bool mouse_over_any = false;

	for (int i = 0; i < BTN_COUNT; i++) {
		Button *b = &s->buttons[i];

		if (mx >= b->x && mx < b->x + b->w && my >= b->y && my < b->y + b->h) {
			s->selected_idx = i;
			mouse_over_any = true;

			if (ev->key == TB_KEY_MOUSE_LEFT) {
				s->pressed_btn_idx = i;
			}
			else if (ev->key == TB_KEY_MOUSE_RELEASE) {
				if (s->pressed_btn_idx == i) {
					s->pressed_btn_idx = -1;
					render(s);
					trigger_button_action(s, i);
				}
				s->pressed_btn_idx = -1;
			}
			return;
		}
	}

	if (!mouse_over_any) {
		if (ev->key == TB_KEY_MOUSE_RELEASE) {
			s->pressed_btn_idx = -1;
		}
	}
}

int main(void) {
	tb_init();
	tb_set_input_mode(TB_INPUT_ESC | TB_INPUT_MOUSE);

	printf("\033[?1003h");
	fflush(stdout);

	AppState state;
	init_state(&state);

	struct tb_event ev;

	while (state.running) {
		render(&state);
		tb_poll_event(&ev);

		if (ev.type == TB_EVENT_KEY) {
			if (state.is_editing) {
				handle_text_input(&state, &ev);
			} else {
				handle_navigation(&state, &ev);
			}
		}
		else if (ev.type == TB_EVENT_MOUSE) {
			handle_mouse(&state, &ev);
		}
	}

	printf("\033[?1003l");
	fflush(stdout);

	tb_shutdown();
	return 0;
}