# pigura/Makefile - build top-level
#
# Plain POSIX make. Auto-detect platform via pkg-config.

NAME    := pigura
VERSION := 0.1.0

include config.mk

.PHONY: all lib demo aplikasi check clean install uninstall dist help

all: lib demo aplikasi

# ---- Library ----
lib: $(PG_LIB)

$(PG_LIB): $(PG_OBJS)
	@echo "AR    $@"
	$(Q)mkdir -p $(@D)
	$(Q)$(AR) rcs $@ $^
	$(Q)$(RANLIB) $@

$(PG_OBJS_DIR)/%.o: %.c
	@echo "CC    $<"
	$(Q)mkdir -p $(@D)
	$(Q)$(CC) $(CFLAGS) -c -o $@ $<

# ---- Lenient compile (codec.c = stb_image impl) ----
# codec.c meng-include stb_image.h dengan STB_IMAGE_IMPLEMENTATION
# aktif. stb_image memakai idiom C99 (komentar //, deklarasi setelah
# statement, dsb). Kompilasi terpisah dengan -Wno-* supaya tidak
# mengganggu baseline strict C89 pigura.
$(PG_OBJS_DIR)/src/gambar/codec.o: src/gambar/codec.c src/gambar/codec_internal_decl.h src/gambar/codec_internal_impl.h include/pigura/codec.h
	@echo "CC    $< (lenient)"
	$(Q)mkdir -p $(@D)
	$(Q)$(CC) -std=gnu89 -Isrc/gambar -Iinclude \
	    -Wno-comment -Wno-declaration-after-statement \
	    -Wno-unused-but-set-variable -Wno-unused-parameter \
	    -Wno-long-long -Wno-variadic-macros -Wno-endif-labels \
	    -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L \
	    -D_BSD_SOURCE -D_SVID_SOURCE -D_XOPEN_SOURCE=700 \
	    -O2 -c -o $@ $<

# ---- Demo ----
demo: $(PG_DEMO)

$(PG_DEMO): $(PG_LIB) $(PG_DEMO_SRC)
	@echo "LD    $@"
	$(Q)mkdir -p $(@D)
	$(Q)$(CC) $(CFLAGS) -o $@ $(PG_DEMO_SRC) -Iinclude \
	    $(PG_LIB) $(LDFLAGS) $(LDLIBS)

aplikasi: $(PG_APLIKASI)

$(PG_APLIKASI): $(PG_LIB) $(PG_APLIKASI_SRC)
	@echo "LD    $@"
	$(Q)mkdir -p $(@D)
	$(Q)$(CC) $(CFLAGS) -o $@ $(PG_APLIKASI_SRC) -Iinclude \
	    $(PG_LIB) $(LDFLAGS) $(LDLIBS)

# ---- Self-test ----
check: $(PG_DEMO)
	@echo "RUN   $< --uji-sendiri"
	$(Q)$(PG_DEMO) --uji-sendiri && echo "OK"

# ---- Install ----
DESTDIR  ?=
PREFIX   ?= /usr/local
INCDIR   := $(DESTDIR)$(PREFIX)/include
LIBDIR   := $(DESTDIR)$(PREFIX)/lib

install: lib
	@echo "INSTALL header -> $(INCDIR)/pigura"
	$(Q)mkdir -p $(INCDIR)/pigura
	$(Q)$(INSTALL) -m 644 include/pigura/*.h $(INCDIR)/pigura/
	@echo "INSTALL lib    -> $(LIBDIR)"
	$(Q)mkdir -p $(LIBDIR)
	$(Q)$(INSTALL) -m 644 $(PG_LIB) $(LIBDIR)/

uninstall:
	$(Q)rm -rf $(INCDIR)/pigura
	$(Q)rm -f $(LIBDIR)/libpigura.a

# ---- Dist ----
DIST := $(NAME)-$(VERSION)
dist:
	@echo "DIST  $(DIST).tar.gz"
	$(Q)tar --transform 's,^\.,$(DIST),' -czf $(DIST).tar.gz \
	    --exclude=build --exclude=.git .

# ---- Clean ----
clean:
	@echo "CLEAN build/"
	$(Q)rm -rf build $(NAME)-*.tar.gz

# ---- Help ----
help:
	@echo "pigura $(VERSION) build targets:"
	@echo "  all      - build libpigura.a + demo (default)"
	@echo "  lib      - build static library only"
	@echo "  demo     - build demo binary"
	@echo "  check    - run demo in --uji-sendiri mode"
	@echo "  install  - install headers + lib to PREFIX=$(PREFIX)"
	@echo "  uninstall- remove installed files"
	@echo "  dist     - build $(DIST).tar.gz"
	@echo "  clean    - remove build artifacts"
	@echo ""
	@echo "Useful variables:"
	@echo "  BUILD=debug|release|size     (default: release)"
	@echo "  PIGURA_BACKEND=x11|linuxfb|wayland|windows  (default: auto-detect)"
	@echo "  CROSS_COMPILE=arm-linux-gnueabihf-"
	@echo "  V=1                          (verbose)"
	@echo "  PIGURA_CATAT_MATI=1          (matikan logging)"
	@echo ""
	@echo "Backend terdeteksi: $(PG_BACKEND)"
