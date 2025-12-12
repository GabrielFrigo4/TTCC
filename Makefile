# ==========================================
# 1. CONFIGURAÇÕES GERAIS
# ==========================================

CC = gcc
AR = ar
CFLAGS_BASE = -std=c23 -Wall -Wextra -O2

DIR_LIB = lib
DIR_CLI = cli
DIR_TUI = tui
DIR_CROSS = cross

INCLUDES = -I$(DIR_CROSS) -I$(DIR_LIB)

UNAME_S := $(shell uname -s)
EXE_EXT =
IS_WINDOWS =

ifneq (,$(findstring MINGW,$(UNAME_S))$(findstring MSYS,$(UNAME_S))$(filter Windows_NT,$(OS)))
    IS_WINDOWS = 1
    EXE_EXT = .exe
    CFLAGS_PLATFORM = -D_POSIX_C_SOURCE=202405L -D_DEFAULT_SOURCE -DNCURSES_STATIC
endif

ifeq ($(UNAME_S),Darwin)
    CFLAGS_PLATFORM = -D_POSIX_C_SOURCE=202405L -D_DARWIN_C_SOURCE
    PKG_CONFIG_PATH := $(shell brew --prefix ncurses)/lib/pkgconfig:$(PKG_CONFIG_PATH)
    export PKG_CONFIG_PATH
endif

ifeq ($(UNAME_S),Linux)
    CFLAGS_PLATFORM = -D_POSIX_C_SOURCE=202405L -D_DEFAULT_SOURCE
endif

CFLAGS_COMMON = $(CFLAGS_BASE) $(CFLAGS_PLATFORM)

# ==========================================
# 2. DEFINIÇÃO DE BIBLIOTECAS (Raw Data)
# ==========================================

PKG_USB = libusb-1.0
CFLAGS_USB := $(shell pkg-config --cflags $(PKG_USB))
LIBS_USB_STATIC_RAW := $(shell pkg-config --static --libs $(PKG_USB))
LIBS_USB_DYN_RAW    := $(shell pkg-config --libs $(PKG_USB))

PKG_TUI := $(shell pkg-config --exists ncursesw && echo ncursesw || echo ncurses)
CFLAGS_TUI := $(shell pkg-config --cflags $(PKG_TUI))
LIBS_TUI_DYN_RAW   := $(shell pkg-config --libs $(PKG_TUI))

ifeq ($(UNAME_S),Linux)
    NCURSES_A := $(shell find /usr/lib /usr/lib64 /lib /lib64 -name "libncursesw.a" 2>/dev/null | head -n 1)
    ifeq ($(NCURSES_A),)
        LIBS_TUI_STATIC_RAW := $(shell pkg-config --static --libs $(PKG_TUI))
    else
        TINFO_A := $(shell find /usr/lib /usr/lib64 /lib /lib64 -name "libtinfo.a" 2>/dev/null | head -n 1)
        LIBS_TUI_STATIC_RAW := $(NCURSES_A) $(TINFO_A)
    endif
else
    LIBS_TUI_STATIC_RAW := $(shell pkg-config --static --libs $(PKG_TUI))
endif

ifneq ($(UNAME_S),Darwin)
    define link_hybrid_usb
        -Wl,-Bstatic $(LIBS_USB_STATIC_RAW) -Wl,-Bdynamic
    endef
    ifeq ($(UNAME_S),Linux)
        define link_hybrid_tui
            $(LIBS_TUI_STATIC_RAW)
        endef
    else
        define link_hybrid_tui
            -Wl,-Bstatic $(LIBS_TUI_STATIC_RAW) -Wl,-Bdynamic
        endef
    endif
else
    define link_hybrid_usb
        $(LIBS_USB_STATIC_RAW)
    endef
    define link_hybrid_tui
        $(LIBS_TUI_STATIC_RAW)
    endef
endif

# ==========================================
# 3. SELEÇÃO DE MODO DE BUILD (A Mágica)
# ==========================================

SELECTED_LDFLAGS =
SELECTED_USB_LIBS = $(LIBS_USB_DYN_RAW)
SELECTED_TUI_LIBS = $(LIBS_TUI_DYN_RAW)

ifneq (,$(filter static,$(MAKECMDGOALS)))
    SELECTED_USB_LIBS = $(link_hybrid_usb)
    SELECTED_TUI_LIBS = $(link_hybrid_tui)
    ifdef IS_WINDOWS
        SELECTED_LDFLAGS += -static
    endif
endif

# ==========================================
# 4. TARGETS
# ==========================================

TARGET_DS4 = ttds4$(EXE_EXT)
TARGET_ESP = ttesp32$(EXE_EXT)
TARGET_TUI = ttcc$(EXE_EXT)

LIB_DS4_A = $(DIR_LIB)/libds4.a
LIB_ESP_A = $(DIR_LIB)/libesp32.a

ALL_TARGETS = $(TARGET_ESP) $(TARGET_DS4) $(TARGET_TUI)

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
	$(CC) $(CFLAGS_COMMON) $(INCLUDES) -c $< -o $@

$(LIB_DS4_A): $(DIR_LIB)/libds4.o
	@echo "[AR]  $@"
	$(AR) rcs $@ $<

$(LIB_ESP_A): $(DIR_LIB)/libesp32.o
	@echo "[AR]  $@"
	$(AR) rcs $@ $<

$(TARGET_DS4): $(DIR_CLI)/ttds4.c $(LIB_DS4_A)
	@echo "[LD]  $@"
	$(CC) $(CFLAGS_COMMON) $(SELECTED_LDFLAGS) $(CFLAGS_USB) $(INCLUDES) -o $@ $< $(LIB_DS4_A) $(SELECTED_USB_LIBS)

$(TARGET_ESP): $(DIR_CLI)/ttesp32.c $(LIB_ESP_A)
	@echo "[LD]  $@"
	$(CC) $(CFLAGS_COMMON) $(SELECTED_LDFLAGS) $(INCLUDES) -o $@ $< $(LIB_ESP_A)

$(TARGET_TUI): $(DIR_TUI)/ttcc.c $(LIB_DS4_A) $(LIB_ESP_A)
	@echo "[LD]  $@"
	$(CC) $(CFLAGS_COMMON) $(SELECTED_LDFLAGS) $(CFLAGS_USB) $(CFLAGS_TUI) $(INCLUDES) -o $@ $< $(LIB_DS4_A) $(LIB_ESP_A) $(SELECTED_USB_LIBS) $(SELECTED_TUI_LIBS)

clean:
	@echo "[CLEAN] Removendo artefatos..."
	rm -f $(ALL_TARGETS)
	rm -f $(DIR_LIB)/*.a $(DIR_LIB)/*.o

install:
	@echo "[INSTALL] Instalando em $(DESTDIR)$(PREFIX)/bin..."
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET_DS4) $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET_ESP) $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET_TUI) $(DESTDIR)$(PREFIX)/bin

uninstall:
	@echo "[UNINSTALL] Removendo..."
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET_DS4)
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET_ESP)
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET_TUI)