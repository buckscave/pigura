# Makefile untuk Pigura OS
# Sistem Operasi berbasis Framebuffer
# Bahasa: C89 + POSIX.1-2008

# Kompiler dan flag
CC = gcc
CFLAGS = -std=c89 -Wall -Wextra -Werror -pedantic
CFLAGS += -D_POSIX_C_SOURCE=200112L
CFLAGS += -D_XOPEN_SOURCE=600
CFLAGS += -O2 -g
CFLAGS += -fno-builtin -fno-stack-protector
CFLAGS += -I./include

# Linker flags
LDFLAGS = -static
LIBS = -lm

# Direktori
SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = bin

# File sumber
SOURCES = huruf.c unikode.c ansi.c papan_ketik.c tetikus.c \
          virtual_terminal.c cangkang.c papan_klip.c konfigurasi.c \
          terminal.c framebuffer.c pembantu.c

# File objek
OBJECTS = $(SOURCES:%.c=$(OBJDIR)/%.o)

# Target utama
TARGET = terminal

# Header dependencies
HEADERS = pigura.h

# Aturan default
all: direktori $(TARGET)

# Buat direktori
direktori:
	@mkdir -p $(OBJDIR) $(BINDIR)

# Link target
$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	@echo "Build selesai: $@"

# Kompilasi file sumber
$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

# Aturan untuk membersihkan
clean:
	rm -rf $(OBJDIR) $(BINDIR)
	@echo "Bersih selesai"

# Aturan untuk install
install: $(TARGET)
	@echo "Install ke /usr/local/bin..."
	cp $(TARGET) /usr/local/bin/
	@echo "Install selesai"

# Aturan untuk uninstall
uninstall:
	rm -f /usr/local/bin/pigura-terminal
	@echo "Uninstall selesai"

# Aturan untuk debug
debug: CFLAGS += -DDEBUG -O0
debug: clean all

# Aturan untuk release
release: CFLAGS += -DNDEBUG -O3
release: clean all

# Dependensi file
huruf.o: huruf.c pigura.h
unikode.o: unikode.c pigura.h
ansi.o: ansi.c pigura.h
papan_ketik.o: papan_ketik.c pigura.h
tetikus.o: tetikus.c pigura.h
virtual_terminal.o: virtual_terminal.c pigura.h
cangkang.o: cangkang.c pigura.h
klip.o: klip.c pigura.h
konfigurasi.o: konfigurasi.c pigura.h
terminal.o: terminal.c pigura.h
framebuffer.o: framebuffer.c pigura.h
utilitas.o: utilitas.c pigura.h

.PHONY: all clean install uninstall debug release direktori
