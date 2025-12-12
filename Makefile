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

ifneq (,$(findstring MINGW,$(UNAME_S))$(findstring MSYS,$(UNAME_S))$(filter Windows_NT,$(OS)))
    EXE_EXT = .exe
    CFLAGS_PLATFORM = -D_POSIX_C_SOURCE=202405L -D_DEFAULT_SOURCE
endif

ifeq ($(UNAME_S),Darwin)
    CFLAGS_PLATFORM = -D_POSIX_C_SOURCE=200809L -D_DARWIN_C_SOURCE
endif

ifeq ($(UNAME_S),Linux)
    CFLAGS_PLATFORM = -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE
endif

CFLAGS_COMMON = $(CFLAGS_BASE) $(CFLAGS_PLATFORM)

# ==========================================
# 2. DEFINIÇÃO DE BIBLIOTECAS HÍBRIDAS
# ==========================================

PKG_USB = libusb-1.0
CFLAGS_USB := $(shell pkg-config --cflags $(PKG_USB))

PKG_TUI = ncurses
CFLAGS_TUI := $(shell pkg-config --cflags $(PKG_TUI))

ifneq (,$(findstring MINGW,$(UNAME_S))$(findstring MSYS,$(UNAME_S))$(filter Windows_NT,$(OS)))
    define libs_usb_static_macro
        -Wl,-Bstatic $(shell pkg-config --static --libs-only-l $(PKG_USB)) \
        -Wl,-Bdynamic $(shell pkg-config --static --libs-only-other $(PKG_USB))
    endef
    define libs_tui_static_macro
        -Wl,-Bstatic $(shell pkg-config --static --libs-only-l $(PKG_TUI)) \
        -Wl,-Bdynamic $(shell pkg-config --static --libs-only-other $(PKG_TUI))
    endef
endif

ifeq ($(UNAME_S),Darwin)
    define libs_usb_static_macro
        $(shell pkg-config --variable=libdir $(PKG_USB))/libusb-1.0.a \
        $(shell pkg-config --static --libs-only-other $(PKG_USB)) \
        -lobjc -Wl,-framework,IOKit -Wl,-framework,CoreFoundation
    endef
    define libs_tui_static_macro
        $(shell pkg-config --variable=libdir $(PKG_TUI))/libncurses.a \
        $(shell pkg-config --static --libs-only-other $(PKG_TUI))
    endef
endif

ifeq ($(UNAME_S),Linux)
    define libs_usb_static_macro
        $(shell pkg-config --variable=libdir $(PKG_USB))/libusb-1.0.a \
        $(filter-out -l$(PKG_USB), $(shell pkg-config --static --libs $(PKG_USB)))
    endef
    define libs_tui_static_macro
        $(shell pkg-config --variable=libdir $(PKG_TUI))/libncurses.a \
        $(shell pkg-config --variable=libdir $(PKG_TUI))/libtinfo.a \
        $(shell pkg-config --static --libs-only-other $(PKG_TUI))
    endef
endif

LIBS_USB_DYN := $(shell pkg-config --libs $(PKG_USB))
LIBS_USB_STAT := $(libs_usb_static_macro)

LIBS_TUI_DYN := $(shell pkg-config --libs $(PKG_TUI))
LIBS_TUI_STAT := $(libs_tui_static_macro)

# ==========================================
# 3. TARGETS
# ========================================

TARGET_DS4 = ttds4$(EXE_EXT)
TARGET_ESP = ttesp32$(EXE_EXT)
TARGET_TUI = ttcc$(EXE_EXT)

LIB_DS4_A = $(DIR_LIB)/libds4.a
LIB_ESP_A = $(DIR_LIB)/libesp32.a

ALL_TARGETS = $(TARGET_ESP) $(TARGET_DS4) $(TARGET_TUI)

.PHONY: all dynamic static clean install uninstall install_deps

all: dynamic

dynamic: LIBS_CURRENT_USB = $(LIBS_USB_DYN)
dynamic: LIBS_CURRENT_TUI = $(LIBS_TUI_DYN)
dynamic: clean $(ALL_TARGETS)
	@echo "[INFO]: Build DINÂMICO concluído."

static: LIBS_CURRENT_USB = $(LIBS_USB_STAT)
static: LIBS_CURRENT_TUI = $(LIBS_TUI_STAT)
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
	$(CC) $(CFLAGS_COMMON) $(CFLAGS_USB) $(INCLUDES) -o $@ $< $(LIB_DS4_A) $(LIBS_CURRENT_USB)

$(TARGET_ESP): $(DIR_CLI)/ttesp32.c $(LIB_ESP_A)
	@echo "[LD]  $@"
	$(CC) $(CFLAGS_COMMON) $(INCLUDES) -o $@ $< $(LIB_ESP_A)

$(TARGET_TUI): $(DIR_TUI)/ttcc.c $(LIB_DS4_A) $(LIB_ESP_A)
	@echo "[LD]  $@"
	$(CC) $(CFLAGS_COMMON) $(CFLAGS_USB) $(CFLAGS_TUI) $(INCLUDES) -o $@ $< $(LIB_DS4_A) $(LIB_ESP_A) $(LIBS_CURRENT_USB) $(LIBS_CURRENT_TUI)

clean:
	@echo "[CLEAN] Removendo binários e objetos..."
	rm -f $(ALL_TARGETS)
	rm -f $(DIR_LIB)/*.a $(DIR_LIB)/*.o

install:
	@echo "[INSTALL] Copiando para $(DESTDIR)$(PREFIX)/bin..."
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET_DS4) $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET_ESP) $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(TARGET_TUI) $(DESTDIR)$(PREFIX)/bin

uninstall:
	@echo "[UNINSTALL] Removendo..."
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET_DS4)
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET_ESP)
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET_TUI)
