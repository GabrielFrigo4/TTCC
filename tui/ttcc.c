#define _XOPEN_SOURCE_EXTENDED
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <locale.h>
#include <ncurses.h>
#include "libds4.h"
#include "libesp32.h"

#define ICON_GAMEPAD    "\uf11b"
#define ICON_CHIP       "\uf2db"
#define ICON_USB        "\uf287"
#define ICON_KEYBOARD   "\uf11c"
#define ICON_SYNC       "\uf021"
#define ICON_UPLOAD     "\uf093"
#define ICON_EXIT       "\uf011"
#define ICON_CHECK      "\uf00c"
#define ICON_ERROR      "\uf071"
#define ICON_MOUSE      "\uf245"

enum {
	CP_DEFAULT = 1,
	CP_ACCENT,
	CP_BTN_IDLE,
	CP_BTN_HOVER,
	CP_BTN_PRESS,
	CP_STATUS_RED,
	CP_STATUS_GREEN,
	CP_STATUS_YELLOW,
	CP_FIELD,
	CP_EDIT
};

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
	int status_pair;
	bool ds4_ok;
	bool esp_ok;
	bool is_editing;
	Button buttons[BTN_COUNT];
	int selected_idx;
	int pressed_btn_idx;
	bool running;
	bool dirty;
} AppState;

void set_status(AppState *s, const char *msg, int pair) {
	snprintf(s->status, sizeof(s->status), "%s", msg);
	s->status_pair = pair;
	s->dirty = true;
}

void render(AppState *s);

void force_render(AppState *s) {
	s->dirty = true;
	render(s);
}

void action_scan_ds4(AppState *s) {
	ds4_context_t *ctx = ds4_create_context();
	if (!ctx) {
		set_status(s, ICON_ERROR " Erro: DS4 desconectado.", CP_STATUS_RED);
		s->ds4_ok = false;
		return;
	}
	uint8_t raw[6];
	if (ds4_get_mac(ctx, raw)) {
		ds4_mac_to_string(raw, s->ds4_mac);
		s->ds4_ok = true;
		set_status(s, ICON_CHECK " Sucesso: DS4 Lido.", CP_STATUS_GREEN);
	} else {
		s->ds4_ok = false;
		set_status(s, ICON_ERROR " Erro: Falha na leitura.", CP_STATUS_RED);
	}
	ds4_destroy_context(ctx);
}

void action_scan_esp(AppState *s) {
	if (!esp32_check_esptool()) {
		set_status(s, ICON_ERROR " Erro: esptool ausente.", CP_STATUS_RED);
		s->esp_ok = false;
		return;
	}
	set_status(s, ICON_SYNC " Escaneando...", CP_STATUS_YELLOW);
	force_render(s);
	if (esp32_find_any_mac(s->esp_mac, sizeof(s->esp_mac))) {
		s->esp_ok = true;
		set_status(s, ICON_CHECK " Sucesso: ESP32 Encontrado.", CP_STATUS_GREEN);
	} else {
		s->esp_ok = false;
		set_status(s, ICON_ERROR " Erro: Nenhum ESP32.", CP_STATUS_RED);
	}
}

void action_manual_input(AppState *s) {
	s->is_editing = true;
	s->esp_ok = false;
	memset(s->esp_mac, 0, sizeof(s->esp_mac));
	set_status(s, "DIGITE O MAC. ENTER Confirma.", CP_STATUS_YELLOW);
}

void action_pair(AppState *s) {
	if (!s->esp_ok) {
		set_status(s, ICON_ERROR " Origem inválida.", CP_STATUS_RED);
		return;
	}
	ds4_context_t *ctx = ds4_create_context();
	if (!ctx) {
		set_status(s, ICON_USB " Conecte o DS4.", CP_STATUS_RED);
		return;
	}
	uint8_t target[6];
	if (!ds4_string_to_mac(s->esp_mac, target)) {
		set_status(s, ICON_ERROR " Formato MAC inválido.", CP_STATUS_RED);
		ds4_destroy_context(ctx);
		return;
	}
	if (ds4_set_mac(ctx, target)) {
		ds4_get_mac(ctx, target);
		ds4_mac_to_string(target, s->ds4_mac);
		s->ds4_ok = true;
		set_status(s, ICON_CHECK " SUCESSO! Pareado.", CP_STATUS_GREEN);
	} else {
		set_status(s, ICON_ERROR " Erro na gravação.", CP_STATUS_RED);
	}
	ds4_destroy_context(ctx);
}

void trigger_action(AppState *s, int btn_idx) {
	switch (btn_idx) {
		case BTN_SCAN_DS4: action_scan_ds4(s); break;
		case BTN_SCAN_ESP: action_scan_esp(s); break;
		case BTN_MANUAL:   action_manual_input(s); break;
		case BTN_PAIR:     action_pair(s); break;
		case BTN_EXIT:     s->running = false; break;
	}
	s->dirty = true;
}

void init_state(AppState *s) {
	memset(s, 0, sizeof(AppState));
	strcpy(s->ds4_mac, "--:--:--:--:--:--");
	strcpy(s->esp_mac, "--:--:--:--:--:--");
	strcpy(s->status, "Aguardando ação...");
	s->status_pair = CP_DEFAULT;
	s->running = true;
	s->pressed_btn_idx = -1;
	s->is_editing = false;
	s->dirty = true;

	int y = 13;
	int btn_w = 26;

	s->buttons[BTN_SCAN_DS4] = (Button){0, y, btn_w, 3, "Ler DS4 (USB)", ICON_GAMEPAD};
	s->buttons[BTN_SCAN_ESP] = (Button){0, y, btn_w, 3, "Ler ESP32", ICON_CHIP};
	s->buttons[BTN_PAIR]     = (Button){0, y + 4, btn_w, 3, "GRAVAR / PAREAR", ICON_UPLOAD};
	s->buttons[BTN_MANUAL]   = (Button){0, y + 4, btn_w, 3, "Manual Input", ICON_KEYBOARD};
	s->buttons[BTN_EXIT]     = (Button){0, y + 8, 20, 3, "Sair", ICON_EXIT};
}

void update_layout(AppState *s) {
	int cx = getmaxx(stdscr) / 2;
	s->buttons[BTN_SCAN_DS4].x = cx - s->buttons[BTN_SCAN_DS4].w - 1;
	s->buttons[BTN_PAIR].x     = cx - s->buttons[BTN_PAIR].w - 1;
	s->buttons[BTN_SCAN_ESP].x = cx + 1;
	s->buttons[BTN_MANUAL].x   = cx + 1;
	s->buttons[BTN_EXIT].x     = cx - (s->buttons[BTN_EXIT].w / 2);
}

void draw_rect(int x, int y, int w, int h) {
	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			mvaddch(y + i, x + j, ' ');
		}
	}
}

void draw_outline(int x, int y, int w, int h) {
	mvaddch(y, x, ACS_ULCORNER);
	mvaddch(y, x + w - 1, ACS_URCORNER);
	mvaddch(y + h - 1, x, ACS_LLCORNER);
	mvaddch(y + h - 1, x + w - 1, ACS_LRCORNER);
	for (int i = 1; i < w - 1; i++) {
		mvaddch(y, x + i, ACS_HLINE);
		mvaddch(y + h - 1, x + i, ACS_HLINE);
	}
	for (int i = 1; i < h - 1; i++) {
		mvaddch(y + i, x, ACS_VLINE);
		mvaddch(y + i, x + w - 1, ACS_VLINE);
	}
}

void draw_button(Button *b, bool selected, bool pressed) {
	int pair = pressed ? CP_BTN_PRESS : (selected ? CP_BTN_HOVER : CP_BTN_IDLE);
	if (!pressed) {
		attron(COLOR_PAIR(CP_DEFAULT));
		draw_rect(b->x + 1, b->y + 1, b->w, b->h);
	}
	attron(COLOR_PAIR(pair));
	draw_rect(b->x, b->y, b->w, b->h);
	if (selected || pressed) {
		attron(A_BOLD);
		draw_outline(b->x, b->y, b->w, b->h);
	}
	int tx = b->x + (b->w - (strlen(b->label) + 2)) / 2;
	int ty = b->y + (b->h / 2);
	mvprintw(ty, tx, "%s %s", b->icon, b->label);
	attroff(COLOR_PAIR(pair) | A_BOLD);
}

void draw_panel(int x, int y, int w, int h, const char *title, const char *mac, bool active, bool editing) {
	int pair = editing ? CP_EDIT : (active ? CP_STATUS_GREEN : CP_DEFAULT);
	attron(COLOR_PAIR(pair));
	draw_outline(x, y, w, h);
	mvprintw(y, x + 2, " %s ", title);
	attroff(COLOR_PAIR(pair));
	int mac_pair = editing ? CP_EDIT : CP_FIELD;
	attron(COLOR_PAIR(mac_pair) | A_BOLD);
	mvprintw(y + 2, x + (w - 17) / 2, "%s%s", mac, editing ? "_" : "");
	attroff(COLOR_PAIR(mac_pair) | A_BOLD);
}

void render(AppState *s) {
	if (!s->dirty) return;
	erase();
	update_layout(s);
	int w = getmaxx(stdscr);
	int h = getmaxy(stdscr);
	int cx = w / 2;

	attron(COLOR_PAIR(CP_ACCENT) | A_BOLD);
	mvprintw(2, cx - 8, "%s  PAIR TOOL %s", ICON_GAMEPAD, ICON_CHIP);
	attroff(COLOR_PAIR(CP_ACCENT) | A_BOLD);

	draw_panel(cx - 28, 5, 26, 5, "DualShock 4", s->ds4_mac, s->ds4_ok, false);
	draw_panel(cx + 2, 5, 26, 5, "ESP32 Device", s->esp_mac, s->esp_ok, s->is_editing);
	mvprintw(7, cx - 1, "\uf060");

	for (int i = 0; i < BTN_COUNT; i++) {
		draw_button(&s->buttons[i], i == s->selected_idx, i == s->pressed_btn_idx);
	}

	attron(COLOR_PAIR(s->status_pair) | A_BOLD);
	mvhline(h - 2, 0, ' ', w);
	mvprintw(h - 2, 2, "%s", s->status);
	attroff(COLOR_PAIR(s->status_pair) | A_BOLD);

	char help[64];
	int x_pos;
	if (s->is_editing) {
		snprintf(help, sizeof(help), "ENTER: Confirmar | ESC: Cancelar");
		x_pos = w - strlen(help) - 2;
	} else {
		snprintf(help, sizeof(help), "%s Click/Enter: Select | %s Quit", ICON_MOUSE, ICON_EXIT);
		x_pos = w - strlen(help) + 2;
	}
	attron(COLOR_PAIR(CP_DEFAULT));
	mvprintw(h - 2, x_pos, "%s", help);
	attroff(COLOR_PAIR(CP_DEFAULT));
	refresh();
	s->dirty = false;
}

void handle_text(AppState *s, int ch) {
	if (ch == '\n' || ch == KEY_ENTER) {
		uint8_t dummy[6];
		if (ds4_string_to_mac(s->esp_mac, dummy)) {
			s->is_editing = false;
			s->esp_ok = true;
			set_status(s, "MAC Manual definido.", CP_STATUS_GREEN);
		} else {
			set_status(s, "Formato Invalido (AA:BB:...)", CP_STATUS_RED);
		}
	} else if (ch == 27) {
		s->is_editing = false;
		strcpy(s->esp_mac, "--:--:--:--:--:--");
		set_status(s, "Cancelado.", CP_DEFAULT);
	} else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
		int len = strlen(s->esp_mac);
		if (len > 0) {
			s->esp_mac[len - 1] = '\0';
			s->dirty = true;
		}
	} else if (isprint(ch)) {
		char c = toupper((char)ch);
		int len = strlen(s->esp_mac);
		if (len < 17 && ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || c == ':')) {
			s->esp_mac[len] = c;
			s->esp_mac[len + 1] = '\0';
			s->dirty = true;
		}
	}
}

void handle_nav(AppState *s, int ch) {
	s->pressed_btn_idx = -1;
	int old_idx = s->selected_idx;
	switch (ch) {
		case 27: case 'q': s->running = false; break;
		case KEY_DOWN: case '\t':
			if (s->selected_idx == BTN_SCAN_DS4) s->selected_idx = BTN_PAIR;
			else if (s->selected_idx == BTN_SCAN_ESP) s->selected_idx = BTN_MANUAL;
			else if (s->selected_idx == BTN_PAIR || s->selected_idx == BTN_MANUAL) s->selected_idx = BTN_EXIT;
			else s->selected_idx = BTN_SCAN_DS4;
			break;
		case KEY_UP:
			if (s->selected_idx == BTN_EXIT) s->selected_idx = BTN_PAIR;
			else if (s->selected_idx == BTN_PAIR) s->selected_idx = BTN_SCAN_DS4;
			else if (s->selected_idx == BTN_MANUAL) s->selected_idx = BTN_SCAN_ESP;
			else s->selected_idx = BTN_EXIT;
			break;
		case KEY_RIGHT:
			if (s->selected_idx == BTN_SCAN_DS4) s->selected_idx = BTN_SCAN_ESP;
			else if (s->selected_idx == BTN_PAIR) s->selected_idx = BTN_MANUAL;
			break;
		case KEY_LEFT:
			if (s->selected_idx == BTN_SCAN_ESP) s->selected_idx = BTN_SCAN_DS4;
			else if (s->selected_idx == BTN_MANUAL) s->selected_idx = BTN_PAIR;
			break;
		case '\n': case KEY_ENTER: case ' ':
			s->pressed_btn_idx = s->selected_idx;
			force_render(s);
			usleep(100000);
			s->pressed_btn_idx = -1;
			trigger_action(s, s->selected_idx);
			break;
	}
	if (old_idx != s->selected_idx) s->dirty = true;
}

int get_hovered_button(AppState *s, int x, int y) {
	for (int i = 0; i < BTN_COUNT; i++) {
		Button *b = &s->buttons[i];
		if (x >= b->x && x < b->x + b->w && y >= b->y && y < b->y + b->h) return i;
	}
	return -1;
}

void handle_mouse(AppState *s) {
	MEVENT event;
	if (getmouse(&event) != OK) return;

	int hovered = get_hovered_button(s, event.x, event.y);
	if (hovered != -1 && s->selected_idx != hovered) {
		s->selected_idx = hovered;
		s->dirty = true;
	}

	if (event.bstate & BUTTON1_PRESSED) {
		if (hovered != -1) {
			s->pressed_btn_idx = hovered;
			s->dirty = true;
		}
	} else if (event.bstate & (BUTTON1_RELEASED | BUTTON1_CLICKED)) {
		if (s->pressed_btn_idx != -1) {
			if (s->pressed_btn_idx == hovered) {
				s->pressed_btn_idx = -1;
				force_render(s);
				trigger_action(s, hovered);
			} else {
				s->pressed_btn_idx = -1;
				s->dirty = true;
			}
		}
	}
}

void setup_ncurses() {
	setlocale(LC_ALL, "");
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	curs_set(0);
	set_escdelay(25);
	mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
	mouseinterval(0);
	printf("\033[?1003h\n");
	start_color();
	use_default_colors();
	init_pair(CP_DEFAULT, COLOR_WHITE, -1);
	init_pair(CP_ACCENT, COLOR_CYAN, -1);
	init_pair(CP_BTN_IDLE, COLOR_WHITE, COLOR_BLUE);
	init_pair(CP_BTN_HOVER, COLOR_WHITE, COLOR_MAGENTA);
	init_pair(CP_BTN_PRESS, COLOR_WHITE, COLOR_GREEN);
	init_pair(CP_STATUS_RED, COLOR_RED, -1);
	init_pair(CP_STATUS_GREEN, COLOR_GREEN, -1);
	init_pair(CP_STATUS_YELLOW, COLOR_YELLOW, -1);
	init_pair(CP_FIELD, COLOR_CYAN, -1);
	init_pair(CP_EDIT, COLOR_YELLOW, -1);
}

int main(void) {
	setup_ncurses();
	AppState state;
	init_state(&state);
	while (state.running) {
		render(&state);
		int ch = getch();
		if (ch == KEY_RESIZE) {
			erase();
			state.dirty = true;
		} else if (ch == KEY_MOUSE) {
			handle_mouse(&state);
		} else if (state.is_editing) {
			handle_text(&state, ch);
		} else if (ch != ERR) {
			handle_nav(&state, ch);
		}
	}
	printf("\033[?1003l\n");
	endwin();
	return 0;
}