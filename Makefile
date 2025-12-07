# ==========================================
# 1. CONFIGURAÇÕES GERAIS
# ==========================================

CC = gcc
CFLAGS_COMMON = -std=c23 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=202405L

UNAME_S := $(shell uname -s)
EXE_EXT =

ifneq (,$(findstring MINGW,$(UNAME_S))$(findstring MSYS,$(UNAME_S))$(filter Windows_NT,$(OS)))
    EXE_EXT = .exe
endif

# ==========================================
# 2. DEFINIÇÃO DE LIBRARIAS (STATIC VS DYNAMIC)
# ==========================================

PKG_USB = libusb-1.0
CFLAGS_USB := $(shell pkg-config --cflags $(PKG_USB))

# Windows (MSYS2) - Static
ifneq (,$(findstring MINGW,$(UNAME_S))$(findstring MSYS,$(UNAME_S))$(filter Windows_NT,$(OS)))
    define libs_usb_static_macro
        -Wl,-Bstatic $(shell pkg-config --static --libs-only-l $(PKG_USB)) \
        -Wl,-Bdynamic $(shell pkg-config --static --libs-only-other $(PKG_USB))
    endef
endif

# MacOS (XNU) - Static
ifeq ($(UNAME_S),Darwin)
    define libs_usb_static_macro
        $(shell pkg-config --variable=libdir $(PKG_USB))/libusb-1.0.a \
        $(shell pkg-config --static --libs-only-other $(PKG_USB))
    endef
endif

# Linux (GNU) - Static
ifeq ($(UNAME_S),Linux)
    define libs_usb_static_macro
        -Wl,-Bstatic $(shell pkg-config --static --libs-only-l $(PKG_USB)) \
        -Wl,-Bdynamic $(shell pkg-config --static --libs-only-other $(PKG_USB))
    endef
endif

LIBS_USB_DYN := $(shell pkg-config --libs $(PKG_USB))
LIBS_USB_STAT := $(libs_usb_static_macro)

# ==========================================
# 3. TARGETS
# ==========================================

TARGETS = ttesp32$(EXE_EXT) ttds4$(EXE_EXT)
DEPS = platform.h
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

.PHONY: all dynamic static clean install uninstall

all: dynamic

dynamic: LIBS_CURRENT = $(LIBS_USB_DYN)
dynamic: clean ttesp32$(EXE_EXT) ttds4$(EXE_EXT)
	@echo "[INFO]: Build DINÂMICO concluído."

static: LIBS_CURRENT = $(LIBS_USB_STAT)
static: clean ttesp32$(EXE_EXT) ttds4$(EXE_EXT)
	@echo "[INFO]: Build ESTÁTICO concluído."

ttesp32$(EXE_EXT): ttesp32.c $(DEPS)
	@echo "[INFO]: Compilando $@..."
	$(CC) $(CFLAGS_COMMON) -o $@ $<

ttds4$(EXE_EXT): ttds4.c $(DEPS)
	@echo "[INFO]: Compilando $@..."
	$(CC) $(CFLAGS_COMMON) $(CFLAGS_USB) -o $@ $< $(LIBS_CURRENT)

clean:
	@echo "[INFO]: Limpando os Binários $(TARGETS)..."
	rm -f $(TARGETS)

install:
	@echo "[INFO]: Instalando em $(DESTDIR)$(BINDIR)..."
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGETS) $(DESTDIR)$(BINDIR)

uninstall:
	@echo "[INFO]: Removendo de $(DESTDIR)$(BINDIR)..."
	rm -f $(addprefix $(DESTDIR)$(BINDIR)/, $(TARGETS))
