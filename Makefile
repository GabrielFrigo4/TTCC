# ==========================================
# 1. CONFIGURAÇÕES GERAIS
# ==========================================

CC          := gcc
AR          := ar
RC          := windres
CFLAGS_BASE := -std=c23 -Wall -Wextra -O2

DIR_LIB   := lib
DIR_CLI   := cli
DIR_TUI   := tui
DIR_CROSS := cross
INCLUDES  := -I$(DIR_CROSS) -I$(DIR_LIB)

UNAME_S    := $(shell uname -s)
IS_WINDOWS :=
IS_MACOS   :=
IS_LINUX   :=

ifneq (,$(findstring MINGW,$(UNAME_S))$(findstring MSYS,$(UNAME_S))$(filter Windows_NT,$(OS)))
    IS_WINDOWS      := 1
    TARGET_EXT      := .exe
    CFLAGS_PLATFORM := -D_POSIX_C_SOURCE=202405L -D_DEFAULT_SOURCE
endif

ifeq ($(UNAME_S),Darwin)
    IS_MACOS        := 1
    TARGET_EXT      :=
    CFLAGS_PLATFORM := -D_POSIX_C_SOURCE=202405L -D_DEFAULT_SOURCE -D_DARWIN_C_SOURCE
    PKG_CONFIG_PATH := $(shell brew --prefix ncurses)/lib/pkgconfig:$(PKG_CONFIG_PATH)
    export PKG_CONFIG_PATH
endif

ifeq ($(UNAME_S),Linux)
    IS_LINUX        := 1
    TARGET_EXT      :=
    CFLAGS_PLATFORM := -D_POSIX_C_SOURCE=202405L -D_DEFAULT_SOURCE
endif

CFLAGS_COMMON = $(CFLAGS_BASE) $(CFLAGS_PLATFORM)

# ==========================================
# 2. DEFINIÇÃO DE BIBLIOTECAS
# ==========================================

PKG_USB             := libusb-1.0
CFLAGS_USB          := $(shell pkg-config --cflags $(PKG_USB))
LIBS_USB_STATIC_RAW := $(shell pkg-config --static --libs $(PKG_USB))
LIBS_USB_DYN_RAW    := $(shell pkg-config --libs $(PKG_USB))

PKG_SP             := libserialport
CFLAGS_SP          := $(shell pkg-config --cflags $(PKG_SP))
LIBS_SP_STATIC_RAW := $(shell pkg-config --static --libs $(PKG_SP))
LIBS_SP_DYN_RAW    := $(shell pkg-config --libs $(PKG_SP))

ifeq ($(IS_WINDOWS),1)
    PKG_TUI             :=
    CFLAGS_TUI          := -mwindows
    LIBS_TUI_DYN_RAW    := -lpdcurses -lgdi32 -ldwmapi
    LIBS_TUI_STATIC_RAW := -lpdcurses -lgdi32 -ldwmapi -lcomdlg32 -lwinmm
else
    PKG_TUI             := $(shell pkg-config --exists ncursesw && echo ncursesw || echo ncurses)
    CFLAGS_TUI          := $(shell pkg-config --cflags $(PKG_TUI))
    LIBS_TUI_DYN_RAW    := $(shell pkg-config --libs $(PKG_TUI))
    LIBS_TUI_STATIC_RAW := $(shell pkg-config --static --libs $(PKG_TUI))
endif

ifeq ($(IS_LINUX),1)
    NCURSES_A := $(shell find /usr/lib /usr/lib64 /lib /lib64 -name "libncursesw.a" 2>/dev/null | head -n 1)
    ifeq ($(NCURSES_A),)
        LIBS_TUI_STATIC_RAW := $(shell pkg-config --static --libs $(PKG_TUI))
    else
        TINFO_A := $(shell find /usr/lib /usr/lib64 /lib /lib64 -name "libtinfo.a" 2>/dev/null | head -n 1)
        LIBS_TUI_STATIC_RAW := $(NCURSES_A) $(TINFO_A)
    endif
endif

ifeq ($(IS_WINDOWS),1)
    define link_hybrid_usb
        -Wl,-Bstatic $(LIBS_USB_STATIC_RAW) -Wl,-Bdynamic
    endef
    define link_hybrid_sp
        -Wl,-Bstatic $(LIBS_SP_STATIC_RAW) -Wl,-Bdynamic
    endef
    define link_hybrid_tui
        $(LIBS_TUI_STATIC_RAW)
    endef
endif

ifeq ($(IS_MACOS),1)
    define link_hybrid_usb
        $(LIBS_USB_STATIC_RAW)
    endef
    define link_hybrid_sp
        $(LIBS_SP_STATIC_RAW)
    endef
    define link_hybrid_tui
        $(LIBS_TUI_STATIC_RAW)
    endef
endif

ifeq ($(IS_LINUX),1)
    define link_hybrid_usb
        -Wl,-Bstatic $(LIBS_USB_STATIC_RAW) -Wl,-Bdynamic
    endef
    define link_hybrid_sp
        -Wl,-Bstatic $(LIBS_SP_STATIC_RAW) -Wl,-Bdynamic
    endef
    define link_hybrid_tui
        $(LIBS_TUI_STATIC_RAW)
    endef
endif

# ==========================================
# 3. SELEÇÃO DE MODO DE BUILD
# ==========================================

SELECTED_USB_LIBS := $(LIBS_USB_DYN_RAW)
SELECTED_SP_LIBS  := $(LIBS_SP_DYN_RAW)
SELECTED_TUI_LIBS := $(LIBS_TUI_DYN_RAW)

ifneq (,$(filter static,$(MAKECMDGOALS)))
    ifeq ($(IS_WINDOWS),1)
        SELECTED_LDFLAGS  := -static
        SELECTED_USB_LIBS := $(LIBS_USB_STATIC_RAW)
        SELECTED_SP_LIBS  := $(LIBS_SP_STATIC_RAW)
        SELECTED_TUI_LIBS := $(LIBS_TUI_STATIC_RAW)
    else
        SELECTED_LDFLAGS  :=
        SELECTED_USB_LIBS := $(link_hybrid_usb)
        SELECTED_SP_LIBS  := $(link_hybrid_sp)
        SELECTED_TUI_LIBS := $(link_hybrid_tui)
    endif
endif

# ==========================================
# 4. TARGETS GERAIS
# ==========================================

TARGET_DS4  := ttds4$(TARGET_EXT)
TARGET_ESP  := ttesp32$(TARGET_EXT)
TARGET_TUI  := ttcc$(TARGET_EXT)
ALL_TARGETS := $(TARGET_ESP) $(TARGET_DS4) $(TARGET_TUI)

LIB_DS4_A := $(DIR_LIB)/libds4.a
LIB_ESP_A := $(DIR_LIB)/libesp32.a

ifeq ($(IS_WINDOWS),1)
    TUI_RES := $(DIR_TUI)/ttcc.res
endif

PREFIX ?= /usr/local

.PHONY: all dynamic static clean install uninstall

all: dynamic

dynamic: clean $(ALL_TARGETS)
	@echo "[INFO]: Build DINÂMICO concluído."

static: clean $(ALL_TARGETS)
	@echo "[INFO]: Build ESTÁTICO concluído."

$(DIR_LIB)/libds4.o: $(DIR_LIB)/libds4.c $(DIR_LIB)/libds4.h $(DIR_CROSS)/platform.h
	@echo "[CC]  $@"
	$(CC) $(CFLAGS_COMMON) $(CFLAGS_USB) $(INCLUDES) -c $< -o $@

$(DIR_LIB)/libesp32.o: $(DIR_LIB)/libesp32.c $(DIR_LIB)/libesp32.h $(DIR_CROSS)/platform.h
	@echo "[CC]  $@"
	$(CC) $(CFLAGS_COMMON) $(CFLAGS_SP) $(INCLUDES) -c $< -o $@

$(LIB_DS4_A): $(DIR_LIB)/libds4.o
	@echo "[AR]  $@"
	$(AR) rcs $@ $<

$(LIB_ESP_A): $(DIR_LIB)/libesp32.o
	@echo "[AR]  $@"
	$(AR) rcs $@ $<

$(TUI_RES): $(DIR_TUI)/ttcc.rc
	@echo "[RC]  $@"
	$(RC) $< -O coff -o $@

$(TARGET_DS4): $(DIR_CLI)/ttds4.c $(LIB_DS4_A)
	@echo "[LD]  $@"
	$(CC) $(CFLAGS_COMMON) $(SELECTED_LDFLAGS) $(CFLAGS_USB) $(INCLUDES) -o $@ $< $(LIB_DS4_A) $(SELECTED_USB_LIBS)

$(TARGET_ESP): $(DIR_CLI)/ttesp32.c $(LIB_ESP_A)
	@echo "[LD]  $@"
	$(CC) $(CFLAGS_COMMON) $(SELECTED_LDFLAGS) $(CFLAGS_SP) $(INCLUDES) -o $@ $< $(LIB_ESP_A) $(SELECTED_SP_LIBS)

$(TARGET_TUI): $(DIR_TUI)/ttcc.c $(LIB_DS4_A) $(LIB_ESP_A) $(TUI_RES)
	@echo "[LD]  $@"
	$(CC) $(CFLAGS_COMMON) $(SELECTED_LDFLAGS) $(CFLAGS_USB) $(CFLAGS_SP) $(CFLAGS_TUI) $(INCLUDES) -o $@ $< $(LIB_DS4_A) $(LIB_ESP_A) $(TUI_RES) $(SELECTED_USB_LIBS) $(SELECTED_SP_LIBS) $(SELECTED_TUI_LIBS)

clean:
	@echo "[CLEAN] Removendo artefatos..."
	rm -f $(ALL_TARGETS)
	rm -f $(DIR_LIB)/*.a $(DIR_LIB)/*.o
	rm -f $(DIR_TUI)/*.res

install:
	@echo "[INSTALL] Instalando..."
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET_DS4) $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET_ESP) $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET_TUI) $(DESTDIR)$(PREFIX)/bin

uninstall:
	@echo "[UNINSTALL] Removendo programas de $(DESTDIR)$(PREFIX)/bin..."
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET_DS4)
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET_ESP)
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET_TUI)
