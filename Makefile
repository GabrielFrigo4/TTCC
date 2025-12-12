# ==========================================
# CONFIGURAÇÕES GERAIS
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

# 1. WINDOWS (MSYS2)
ifneq (,$(findstring MINGW,$(UNAME_S))$(findstring MSYS,$(UNAME_S))$(filter Windows_NT,$(OS)))
    EXE_EXT = .exe
    CFLAGS_PLATFORM = -D_POSIX_C_SOURCE=202405L -D_DEFAULT_SOURCE -DNCURSES_STATIC
endif

# 2. MACOS (XNU)
ifeq ($(UNAME_S),Darwin)
    CFLAGS_PLATFORM = -D_POSIX_C_SOURCE=202405L -D_DARWIN_C_SOURCE
    PKG_CONFIG_PATH := $(shell brew --prefix ncurses)/lib/pkgconfig:$(PKG_CONFIG_PATH)
    export PKG_CONFIG_PATH
endif

# 3. LINUX (GNU)
ifeq ($(UNAME_S),Linux)
    CFLAGS_PLATFORM = -D_POSIX_C_SOURCE=202405L -D_DEFAULT_SOURCE
endif

CFLAGS_COMMON = $(CFLAGS_BASE) $(CFLAGS_PLATFORM)

# ==========================================
# DEPENDÊNCIAS
# ==========================================

PKG_USB = libusb-1.0
CFLAGS_USB := $(shell pkg-config --cflags $(PKG_USB))
LIBS_USB_STATIC := $(shell pkg-config --static --libs $(PKG_USB))
LIBS_USB_DYN := $(shell pkg-config --libs $(PKG_USB))

PKG_TUI := $(shell pkg-config --exists ncursesw && echo ncursesw || echo ncurses)
CFLAGS_TUI := $(shell pkg-config --cflags $(PKG_TUI))
LIBS_TUI_DYN := $(shell pkg-config --libs $(PKG_TUI))

ifeq ($(UNAME_S),Linux)
    NCURSES_A := $(shell find /usr/lib /usr/lib64 /lib /lib64 -name "libncursesw.a" 2>/dev/null | head -n 1)
    ifeq ($(NCURSES_A),)
        NCURSES_A := $(shell find /usr/lib /usr/lib64 /lib /lib64 -name "libncurses.a" 2>/dev/null | head -n 1)
    endif
    TINFO_A := $(shell find /usr/lib /usr/lib64 /lib /lib64 -name "libtinfo.a" 2>/dev/null | head -n 1)

    ifneq ($(NCURSES_A),)
        LIBS_TUI_STATIC := $(NCURSES_A) $(TINFO_A)
    else
        LIBS_TUI_STATIC := $(shell pkg-config --static --libs $(PKG_TUI))
    endif
else
    LIBS_TUI_STATIC := $(shell pkg-config --static --libs $(PKG_TUI))
endif

# ==========================================
# MACROS DE LINKAGEM HÍBRIDAS
# ==========================================

ifneq ($(UNAME_S),Darwin)
    define link_static_usb
        -Wl,-Bstatic $(LIBS_USB_STATIC) -Wl,-Bdynamic
    endef

    # Linux usa o arquivo .a direto (sem flag -l), Windows usa flags
    ifeq ($(UNAME_S),Linux)
        define link_static_tui
            $(LIBS_TUI_STATIC)
        endef
    else
        define link_static_tui
            -Wl,-Bstatic $(LIBS_TUI_STATIC) -Wl,-Bdynamic
        endef
    endif
else
    define link_static_usb
        $(LIBS_USB_STATIC)
    endef
    define link_static_tui
        $(LIBS_TUI_STATIC)
    endef
endif

# ==========================================
# TARGETS
# ==========================================

TARGET_DS4 = ttds4$(EXE_EXT)
TARGET_ESP = ttesp32$(EXE_EXT)
TARGET_TUI = ttcc$(EXE_EXT)

LIB_DS4_A = $(DIR_LIB)/libds4.a
LIB_ESP_A = $(DIR_LIB)/libesp32.a

ALL_TARGETS = $(TARGET_ESP) $(TARGET_DS4) $(TARGET_TUI)

.PHONY: all dynamic static clean install uninstall install_deps

all: dynamic

dynamic: CURRENT_USB = $(LIBS_USB_DYN)
dynamic: CURRENT_TUI = $(LIBS_TUI_DYN)
dynamic: clean $(ALL_TARGETS)
	@echo "[INFO]: Build DINÂMICO concluído."

static: CURRENT_USB = $(link_static_usb)
static: CURRENT_TUI = $(link_static_tui)
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
	$(CC) $(CFLAGS_COMMON) $(CFLAGS_USB) $(INCLUDES) -o $@ $< $(LIB_DS4_A) $(CURRENT_USB)

$(TARGET_ESP): $(DIR_CLI)/ttesp32.c $(LIB_ESP_A)
	@echo "[LD]  $@"
	$(CC) $(CFLAGS_COMMON) $(INCLUDES) -o $@ $< $(LIB_ESP_A)

$(TARGET_TUI): $(DIR_TUI)/ttcc.c $(LIB_DS4_A) $(LIB_ESP_A)
	@echo "[LD]  $@"
	$(CC) $(CFLAGS_COMMON) $(CFLAGS_USB) $(CFLAGS_TUI) $(INCLUDES) -o $@ $< $(LIB_DS4_A) $(LIB_ESP_A) $(CURRENT_USB) $(CURRENT_TUI)

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
