# ==========================================
# 1. CONFIGURAÇÕES GERAIS (VARIÁVEIS)
# ==========================================

CC = gcc
CFLAGS_COMMON = -std=c23 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=202405L

UNAME_S := $(shell uname -s)
EXE_EXT =

# ==========================================
# 2. LÓGICA DE LINKAGEM (MACROS)
# ==========================================
#
# A macro pkg_static_link recebe dois argumentos:
#   $(1): Nome no pkg-config (ex: libusb-1.0, sdl3)
#   $(2): Nome do arquivo da lib estática (apenas p/ MacOS) (ex: libusb-1.0.a, libSDL3.a)

# --- WINDOWS (MSYS2) ---
ifneq (,$(findstring MINGW,$(UNAME_S))$(findstring MSYS,$(UNAME_S))$(filter Windows_NT,$(OS)))
    EXE_EXT = .exe
    define pkg_static_link
        -Wl,-Bstatic $(shell pkg-config --static --libs-only-l $(1)) \
        -Wl,-Bdynamic $(shell pkg-config --static --libs-only-other $(1))
    endef

# --- MACOS (XNU) ---
else ifeq ($(UNAME_S),Darwin)
    EXE_EXT =
    define pkg_static_link
        $(shell pkg-config --variable=libdir $(1))/$(2) \
        $(shell pkg-config --static --libs-only-other $(1))
    endef

# --- LINUX (GNU) ---
else
    EXE_EXT =
    define pkg_static_link
        -Wl,-Bstatic $(shell pkg-config --static --libs-only-l $(1)) \
        -Wl,-Bdynamic $(shell pkg-config --static --libs-only-other $(1))
    endef
endif

# ==========================================
# 3. DEPENDÊNCIAS (HÍBRIDO)
# ==========================================

PKG_USB = libusb-1.0
CFLAGS_USB := $(shell pkg-config --cflags $(PKG_USB))
LIBS_USB := $(call pkg_static_link,$(PKG_USB),libusb-1.0.a)

# ==========================================
# 4. TARGETS (FUNÇÕES)
# ==========================================

TARGETS = ttesp32$(EXE_EXT) ttds4$(EXE_EXT)
DEPS = platform.h

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

.PHONY: all clean install uninstall

all: $(TARGETS)

ttesp32$(EXE_EXT): ttesp32.c $(DEPS)
	@echo "[INFO]: Compilando $@..."
	$(CC) $(CFLAGS_COMMON) -o $@ $<

ttds4$(EXE_EXT): ttds4.c $(DEPS)
	@echo "[INFO]: Compilando $@ com LibUSB Estático..."
	$(CC) $(CFLAGS_COMMON) $(CFLAGS_USB) -o $@ $< $(LIBS_USB)

clean:
	@echo "[INFO]: Limpando os Binários $(TARGETS)..."
	rm -f $(TARGETS)

install: all
	@echo "[INFO]: Instalando Binários em $(DESTDIR)$(BINDIR)..."
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGETS) $(DESTDIR)$(BINDIR)
	@echo "[INFO]: Instalação Concluída."

uninstall:
	@echo "[INFO]: Removendo Binários de $(DESTDIR)$(BINDIR)..."
	rm -f $(addprefix $(DESTDIR)$(BINDIR)/, $(TARGETS))
	@echo "[INFO]: Desinstalação Concluída."
