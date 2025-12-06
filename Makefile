CC = gcc
CFLAGS_COMMON = -std=c23 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=202405L

ifeq ($(OS),Windows_NT)
	EXE_EXT = .exe
	LDFLAGS_STATIC = -static
	PKG_CONFIG_FLAGS = --static
else
	EXE_EXT =
	LDFLAGS_STATIC =
	PKG_CONFIG_FLAGS =
endif

LIBUSB_CFLAGS := $(shell pkg-config --cflags libusb-1.0)
LIBUSB_LIBS   := $(shell pkg-config --libs $(PKG_CONFIG_FLAGS) libusb-1.0)

TARGETS = ttesp32$(EXE_EXT) ttds4$(EXE_EXT)
DEPS = platform.h

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

.PHONY: all clean install uninstall

all: $(TARGETS)

ttesp32$(EXE_EXT): ttesp32.c $(DEPS)
	@echo "[INFO]: Compilando ttesp32.c..."
	$(CC) $(CFLAGS_COMMON) $(LDFLAGS_STATIC) -o $@ $<

ttds4$(EXE_EXT): ttds4.c $(DEPS)
	@echo "[INFO]: Compilando ttds4.c..."
	$(CC) $(CFLAGS_COMMON) $(LDFLAGS_STATIC) $(LIBUSB_CFLAGS) -o $@ $< $(LIBUSB_LIBS)

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
