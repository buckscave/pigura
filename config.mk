# pigura/config.mk - deteksi platform & konfigurasi toolchain
#
# Di-include oleh Makefile top-level. Auto-detect host platform
# via pkg-config; override dari command line untuk cross-build.
#
# Contoh:
#   make                                # native build (auto-detect)
#   make BUILD=debug                    # debug build
#   make CROSS_COMPILE=arm-linux-gnueabihf-   # cross-compile
#   make PIGURA_CATAT_MATI=1            # matikan logging
#   make PIGURA_BACKEND=linuxfb         # paksa framebuffer Linux
#

# ---- Toolchain ----
CROSS_COMPILE ?=
CC            ?= $(CROSS_COMPILE)cc
AR            ?= $(CROSS_COMPILE)ar
RANLIB        ?= $(CROSS_COMPILE)ranlib
INSTALL       ?= install
PKG_CONFIG    ?= $(CROSS_COMPILE)pkg-config

# ---- Auto-detect platform ----
# Urutan prioritas:
#   1. PIGURA_BACKEND eksplisit dari command line
#      (x11 | linuxfb | wayland | windows | macos)
#   2. X11 tersedia (pkg-config x11) → pakai x11
#   3. Linux tanpa X11 → pakai linuxfb (/dev/fb0)
#   4. _WIN32 → windows
#   5. __APPLE__ → macos (CoreGraphics + AppKit via objc runtime)
#
# Catatan: wayland tidak auto-detect karena backend masih stub yang
# mengembalikan PG_GALAT_TANPA. Aktifkan eksplisit via:
#   make PIGURA_BACKEND=wayland

# PIGURA_BACKEND adalah nama publik (lihat `make help`); PG_BACKEND
# adalah alias legacy untuk kompatibilitas mundur. Keduanya dihormati,
# PIGURA_BACKEND menang bila keduanya di-set.
PIGURA_BACKEND ?=
PG_BACKEND ?= $(PIGURA_BACKEND)

ifeq ($(PG_BACKEND),)
  ifeq ($(shell $(PKG_CONFIG) --exists x11 && echo yes),yes)
    PG_BACKEND := x11
  else ifeq ($(shell uname -s 2>/dev/null),Linux)
    PG_BACKEND := linuxfb
  else
    PG_BACKEND := unknown
  endif
endif

# ---- Build flavor ----
BUILD ?= release
ifeq ($(BUILD),debug)
  PG_CFLAGS_OPT := -O0 -g3 -DDEBUG
else ifeq ($(BUILD),release)
  PG_CFLAGS_OPT := -O2 -DNDEBUG
else ifeq ($(BUILD),size)
  PG_CFLAGS_OPT := -Os -DNDEBUG
else
  PG_CFLAGS_OPT := -O2
endif

# ---- Strict C89 + POSIX ----
# _DEFAULT_SOURCE ekspos clock_gettime, nanosleep, localtime_r, vsnprintf
# (yang sebetulnya C99/POSIX, tapi kita perlukan untuk fungsi dasar).
# -pedantic tetap menyala; extension diizinkan via -Wno-*.
PG_CFLAGS_STD := -std=c89 -pedantic -Wall -Wextra -Wno-unused-parameter -Wno-declaration-after-statement \
                 -Wno-unused-function \
                 -Wno-long-long -Wno-variadic-macros \
                 -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L \
                 -D_BSD_SOURCE -D_SVID_SOURCE -D_XOPEN_SOURCE=700

# ---- Warning ketat ----
PG_CFLAGS_WARN := -Wshadow -Wpointer-arith -Wcast-align \
                  -Wwrite-strings -Wmissing-prototypes \
                  -Wstrict-prototypes 

# ---- Path include publik ----
PG_CFLAGS_INC := -Iinclude -Isrc/gambar

# ---- Platform-specific ----
ifeq ($(PG_BACKEND),x11)
  PG_CFLAGS_PLATFORM := $(shell $(PKG_CONFIG) --cflags x11 xext)
  PG_LDFLAGS_PLATFORM := $(shell $(PKG_CONFIG) --libs x11 xext) -lpthread -lrt -lm
  PG_HAVE_XSHM := $(shell $(PKG_CONFIG) --exists xext && echo yes)
  ifeq ($(PG_HAVE_XSHM),yes)
    PG_CFLAGS_PLATFORM += -DHAVE_XSHM
  endif
  PG_CFLAGS_PLATFORM += -DHAVE_X11
  PG_PLATFORM_SRCS := src/layar/x11.c src/masukan/x11.c
  PG_PLATFORM_OBJS := $(PG_OBJS_DIR)/src/layar/x11.o \
                      $(PG_OBJS_DIR)/src/masukan/x11.o
else ifeq ($(PG_BACKEND),linuxfb)
  PG_CFLAGS_PLATFORM := -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE \
                        -D_XOPEN_SOURCE=700
  PG_LDFLAGS_PLATFORM := -lpthread -lrt
  PG_PLATFORM_SRCS := src/layar/linuxfb.c src/masukan/evdev.c
  PG_PLATFORM_OBJS := $(PG_OBJS_DIR)/src/layar/linuxfb.o \
                      $(PG_OBJS_DIR)/src/masukan/evdev.o
else ifeq ($(PG_BACKEND),wayland)
  # Wayland: stub untuk v0.1. Backend lengkap (wl_compositor, xdg_shell,
  # wl_shm, wl_seat) dijadwalkan v0.3+. Stub mengembalikan
  # PG_GALAT_TANPA dengan pesan "Pakai X11".
  PG_CFLAGS_PLATFORM := -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE \
                        -D_XOPEN_SOURCE=700 -DHAVE_WAYLAND
  PG_LDFLAGS_PLATFORM := -lpthread -lrt -lm
  PG_PLATFORM_SRCS := src/layar/wayland.c src/masukan/wayland.c
  PG_PLATFORM_OBJS := $(PG_OBJS_DIR)/src/layar/wayland.o \
                      $(PG_OBJS_DIR)/src/masukan/wayland.o
else ifeq ($(PG_BACKEND),windows)
  PG_CFLAGS_PLATFORM := -D_WIN32_WINNT=0x0600 -DWIN32_LEAN_AND_MEAN
  PG_LDFLAGS_PLATFORM := -lgdi32 -luser32
  PG_PLATFORM_SRCS := src/layar/windows.c src/masukan/windows.c
  PG_PLATFORM_OBJS := $(PG_OBJS_DIR)/src/layar/windows.o \
                      $(PG_OBJS_DIR)/src/masukan/windows.o
else ifeq ($(PG_BACKEND),macos)
  PG_CFLAGS_PLATFORM := -D_DARWIN_C_SOURCE -fno-objc-arc \
                        -Wno-modernize-objc -Wno-objc-string-concatenation
  PG_LDFLAGS_PLATFORM := -framework CoreGraphics -framework AppKit \
                          -framework Foundation
  PG_PLATFORM_SRCS := src/layar/macos.c src/masukan/macos.c
  PG_PLATFORM_OBJS := $(PG_OBJS_DIR)/src/layar/macos.o \
                      $(PG_OBJS_DIR)/src/masukan/macos.o
else
  PG_CFLAGS_PLATFORM :=
  PG_LDFLAGS_PLATFORM :=
  PG_PLATFORM_SRCS :=
  PG_PLATFORM_OBJS :=
endif

# ---- Matikan logging total ----
ifeq ($(PIGURA_CATAT_MATI),1)
  PG_CFLAGS_PLATFORM += -DPIGURA_CATAT_MATI
endif

# ---- Gabungan flag ----
CFLAGS  := $(PG_CFLAGS_STD) $(PG_CFLAGS_OPT) $(PG_CFLAGS_WARN) \
           $(PG_CFLAGS_INC) $(PG_CFLAGS_PLATFORM) $(CFLAGS)
LDFLAGS := $(LDFLAGS)
LDLIBS  := $(PG_LDFLAGS_PLATFORM) $(LDLIBS)

# ---- Output ----
PG_OBJS_DIR := build/obj
PG_LIB_DIR  := build
PG_LIB      := $(PG_LIB_DIR)/libpigura.a
PG_DEMO     := $(PG_LIB_DIR)/pigura_demo

# ---- Sumber library ----
PG_AKSARA_SRCS := \
        src/aksara/ttf.c           \
        src/aksara/font_bitmap.c    \
        src/aksara/unicode.c

PG_INTI_SRCS := \
        src/inti/galat.c       \
        src/inti/catat.c       \
        src/inti/untaian.c     \
        src/inti/permukaan.c   \
        src/inti/perulangan.c  \
        src/inti/papan_klip.c  \
        src/inti/pigura.c      \
        src/inti/tema.c        \
        src/inti/utf8.c        \
        src/inti/timer.c       \
        src/inti/dialog_file.c

PG_GAMBAR_SRCS := \
        src/gambar/gambar.c       \
        src/gambar/transform.c    \
        src/gambar/codec.c        \
        src/gambar/codec_png.c    \
        src/gambar/codec_jpeg.c   \
        src/gambar/codec_bmp.c    \
        src/gambar/codec_gif.c    \
        src/gambar/codec_psd.c    \
        src/gambar/codec_tiff.c   \
        src/gambar/codec_webp.c   \
        src/gambar/codec_xpm.c    \
        src/gambar/codec_ppm.c    \
        src/gambar/codec_zlib.c   \
        src/gambar/codec_huffman.c \
        src/gambar/codec_idct.c

PG_WIDGET_SRCS := \
        src/widget/dasar/widget.c           \
        src/widget/dasar/kotak.c            \
        src/widget/dasar/label.c            \
        src/widget/dasar/tombol.c           \
        src/widget/dasar/isian_teks.c       \
        src/widget/dasar/cek.c              \
        src/widget/dasar/radio.c            \
        src/widget/dasar/daftar.c           \
        src/widget/dasar/kemajuan.c         \
        src/widget/dasar/geser.c            \
        src/widget/dasar/gambar.c           \
        src/widget/dasar/gulir.c            \
        src/widget/dasar/dropdown.c         \
        src/widget/komponen/textedit.c      \
        src/widget/komponen/combobox.c      \
        src/widget/komponen/spinbox.c       \
        src/widget/komponen/searchbox.c      \
        src/widget/komponen/passwordinput.c  \
        src/widget/komponen/tableview.c      \
        src/widget/komponen/treeview.c       \
        src/widget/komponen/colorpicker.c    \
        src/widget/komponen/datepicker.c     \
        src/widget/komponen/menubar.c        \
        src/widget/komponen/tab.c            \
        src/widget/komponen/toolbar.c        \
        src/widget/komponen/status_bar.c     \
        src/widget/komponen/splitter.c       \
        src/widget/komponen/kolaps.c         \
        src/widget/komponen/jendela_widget.c \
        src/widget/komponen/panel.c            \
        src/widget/komponen/tooltip.c        \
        src/widget/komponen/contextmenu.c   \
        src/widget/komponen/dialog_modal.c   \
        src/widget/komponen/dock.c           \
        src/widget/komponen/snap.c           \
        src/widget/komponen/spinbutton.c \

# Window manager internal untuk mode standalone (framebuffer).
# Aktif bersama backend linuxfb; aman di-include di semua build.
PG_WM_SRCS := \
        src/wm/wm.c

PG_SRCS := $(PG_INTI_SRCS) $(PG_AKSARA_SRCS) $(PG_GAMBAR_SRCS) $(PG_WIDGET_SRCS) \
           $(PG_WM_SRCS) $(PG_PLATFORM_SRCS)
PG_OBJS := $(patsubst %.c,$(PG_OBJS_DIR)/%.o,$(PG_SRCS))

# ---- Demo ----
PG_DEMO_SRC := demo/main.c
PG_APLIKASI_SRC := demo/aplikasi.c
PG_APLIKASI := build/pigura_aplikasi

# ---- Pretty printing ----
ifneq ($(V),1)
  Q := @
else
  Q :=
endif
