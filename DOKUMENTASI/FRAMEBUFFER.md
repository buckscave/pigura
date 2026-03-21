# SISTEM GRAFIS DAN FRAMEBUFFER PIGURA OS

## Daftar Isi

1. [Filosofi Tanpa X11/Wayland](#filosofi-tanpa-x11-wayland)
2. [Arsitektur Framebuffer](#arsitektur-framebuffer)
3. [Arsitektur GPU](#arsitektur-gpu)
4. [LibPigura](#libpigura)
5. [Dekor Compositor](#dekor-compositor)
6. [Sistem Font](#sistem-font)
7. [API Reference](#api-reference)
8. [Perbandingan Kompleksitas](#perbandingan-kompleksitas)

---

## Filosofi Tanpa X11/Wayland

### Sejarah dan Masalah X11

X11 Window System dirancang pada era 1984 untuk komputasi terdistribusi. Sistem ini menggunakan arsitektur client-server dimana:

- **Server X** berjalan di mesin dengan display
- **Client (aplikasi)** dapat berjalan di mesin remote
- Komunikasi via network protocol

Arsitektur ini tepat untuk masalah tahun 1980-an (mainframe + dummy terminal), namun menjadi overhead yang tidak perlu untuk PC personal modern:

| Aspek | X11 | Masalah |
|-------|-----|---------|
| Network Transparency | Ya | Tidak diperlukan untuk PC personal |
| IPC Overhead | Tinggi | Socket communication antara client-server |
| Protocol Parsing | Kompleks | Encoding/decoding setiap request |
| Kode | ~500,000+ baris | Maintenance nightmare |
| Ukuran | ~500 MB | Bloat untuk standalone system |

### Pendekatan Windows dan Pigura

Windows menggunakan Graphics Device Interface (GDI) yang terintegrasi langsung tanpa arsitektur server-client. Pigura OS mengadopsi pendekatan serupa:

```
┌─────────────────────────────────────────────────────────────────┐
│                      X11/WAYLAND                                │
├─────────────────────────────────────────────────────────────────┤
│  Aplikasi → Socket → X Server → Driver → Framebuffer           │
│              ↑________________↑                                  │
│               IPC Overhead                                      │
│  Total: ~500 MB kode                                            │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                      PIGURA OS                                  │
├─────────────────────────────────────────────────────────────────┤
│  Aplikasi → LibPigura → Dekor → Driver → Framebuffer           │
│              ↑________↑                                          │
│            Direct Call                                          │
│  Total: ~100 KB kode                                            │
└─────────────────────────────────────────────────────────────────┘
```

---

## Arsitektur Framebuffer

### Konsep Dasar

Framebuffer adalah area memori yang secara langsung mewakili pixel di layar. Setiap pixel direpresentasikan oleh nilai warna (RGB atau RGBA). Dengan menulis ke alamat framebuffer, tampilan layar langsung berubah.

### Layout Memori Framebuffer

```
+------------------+  Alamat awal framebuffer
| Pixel (0,0)      |  Offset: 0
| R G B A          |  4 bytes per pixel (32-bit)
+------------------+
| Pixel (1,0)      |  Offset: 4
| R G B A          |
+------------------+
| ...              |
+------------------+
| Pixel (0,1)      |  Offset: pitch * 1
| R G B A          |  (pitch = lebar * bytes_per_pixel)
+------------------+
| ...              |
+------------------+
| Pixel (W-1,H-1)  |  Offset: pitch * (H-1) + (W-1) * 4
| R G B A          |
+------------------+
```

### Struktur Driver Framebuffer

```
SUMBER/perangkat/tampilan/framebuffer/
│
├── framebuffer.c          # Core framebuffer driver
├── fb_init.c              # Inisialisasi framebuffer
├── fb_mode.c              # Mode setting (resolusi, depth)
├── fb_render.c            # Render ke framebuffer
├── fb_blit.c              # Blitting operations
├── fb_console.c           # Text console di framebuffer
├── fb_cursor.c            # Hardware/software cursor
├── vbe.c                  # VESA BIOS Extensions (BIOS boot)
└── uefi_gop.c             # UEFI Graphics Output Protocol
```

### Cara Kerja Framebuffer

```
1. BOOT
   └── BIOS/UEFI menginisialisasi mode grafis

2. ALAMAT
   └── Kernel mendapat alamat framebuffer dari bootloader

3. MAPPING
   └── Kernel memetakan framebuffer ke ruang alamat kernel

4. WRITING
   └── LibPigura menulis pixel langsung ke alamat tersebut

5. DISPLAY
   └── Tampilan langsung terupdate tanpa copying
```

### Struktur Data Framebuffer

```c
/* Informasi framebuffer */
typedef struct {
    alamat_virtual_t alamat;      /* Alamat virtual framebuffer */
    alamat_fisik_t alamat_fisik;  /* Alamat fisik framebuffer */
    tak_bertanda32_t lebar;       /* Lebar dalam pixel */
    tak_bertanda32_t tinggi;      /* Tinggi dalam pixel */
    tak_bertanda32_t pitch;       /* Bytes per scanline */
    tak_bertanda32_t bpp;         /* Bits per pixel */
    tak_bertanda32_t merah_mask;  /* Mask warna merah */
    tak_bertanda32_t hijau_mask;  /* Mask warna hijau */
    tak_bertanda32_t biru_mask;   /* Mask warna biru */
    tak_bertanda32_t alpha_mask;  /* Mask alpha */
} framebuffer_info_t;

/* Context framebuffer */
typedef struct {
    framebuffer_info_t info;
    tak_bertanda32_t *buffer;     /* Pointer ke framebuffer */
    ukuran_t ukuran;              /* Total ukuran buffer */
    bool_t double_buffer;         /* Gunakan double buffering? */
    tak_bertanda32_t *back_buffer; /* Back buffer jika double */
} framebuffer_t;
```

### Operasi Dasar

#### Menulis Pixel

```c
void fb_tulis_pixel(framebuffer_t *fb, int x, int y, tak_bertanda32_t warna) {
    if (x < 0 || x >= fb->info.lebar || y < 0 || y >= fb->info.tinggi) {
        return;  /* Bounds check */
    }
    
    tak_bertanda32_t offset = y * (fb->info.pitch / 4) + x;
    fb->buffer[offset] = warna;
}
```

#### Menggambar Garis (Bresenham)

```c
void fb_gambar_garis(framebuffer_t *fb, int x1, int y1, int x2, int y2,
                     tak_bertanda32_t warna) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;
    
    while (1) {
        fb_tulis_pixel(fb, x1, y1, warna);
        
        if (x1 == x2 && y1 == y2) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx)  { err += dx; y1 += sy; }
    }
}
```

---

## Arsitektur GPU

### Deteksi GPU

GPU dideteksi menggunakan IC Detection System, bukan driver vendor-specific:

```c
/* Deteksi GPU via IC Detection */
ic_info_t *gpu_ic = ic_deteksi_cari_kategori(IC_KATEGORI_GPU);

if (gpu_ic != NULL) {
    log_info("GPU terdeteksi: %s", gpu_ic->nama);
    
    /* Baca parameter GPU */
    ic_param_gpu_t param;
    ic_parameter_baca(gpu_ic, &param);
    
    log_info("VRAM: %d MB", param.vram / (1024 * 1024));
    log_info("Core Clock: %d MHz", param.core_clock);
}
```

### Struktur Driver GPU

```
SUMBER/perangkat/tampilan/gpu/
│
├── gpu.c                  # Core GPU driver
├── gpu_deteksi.c          # Deteksi via IC Detection
├── gpu_init.c             # Inisialisasi GPU
├── gpu_render.c           # Render commands
├── gpu_2d.c               # 2D acceleration (blit, fill, copy)
├── gpu_3d.c               # 3D acceleration (vertex, shader)
├── gpu_memori.c           # GPU memory management
├── gpu_command.c          # Command buffer
└── gpu_shader.c           # Shader handling
```

### Operasi GPU

| Operasi | Fungsi | Deskripsi |
|---------|--------|-----------|
| Blit | `gpu_blit()` | Copy antar permukaan |
| Fill | `gpu_fill_rect()` | Isi kotak dengan warna |
| Copy | `gpu_mem_copy()` | Copy memori GPU |
| Scale | `gpu_scale()` | Skala gambar |
| Rotate | `gpu_rotate()` | Rotasi gambar |
| Blend | `gpu_blend()` | Alpha blending |

### Akselerasi vs Software Fallback

```c
typedef enum {
    RENDER_CPU,      /* Software rendering */
    RENDER_GPU,      /* Hardware acceleration */
    RENDER_HYBRID    /* CPU + GPU hybrid */
} render_mode_t;

typedef struct {
    render_mode_t mode;
    bool_t gpu_tersedia;
    bool_t gpu_2d_support;
    bool_t gpu_3d_support;
    tak_bertanda32_t gpu_vram;
} render_context_t;

/* Pilih render mode optimal */
render_mode_t pilih_render_mode(void) {
    ic_info_t *gpu = ic_deteksi_cari_kategori(IC_KATEGORI_GPU);
    
    if (gpu == NULL) {
        return RENDER_CPU;  /* Fallback ke CPU */
    }
    
    ic_param_gpu_t param;
    ic_parameter_baca(gpu, &param);
    
    if (param.vram >= 256 * 1024 * 1024) {  /* >= 256MB VRAM */
        return RENDER_GPU;
    }
    
    return RENDER_HYBRID;  /* Limited GPU acceleration */
}
```

---

## LibPigura

### Gambaran Umum

LibPigura adalah library grafis terintegrasi yang menyediakan API untuk menggambar primitif, mengelola jendela, dan merender teks. Library ini dirancang untuk dapat menggunakan CPU maupun GPU secara transparan.

### Struktur LibPigura

```
SUMBER/pigura/
│
├── kanvas/                 # Drawing surface management
│   ├── kanvas.c           # Core kanvas
│   ├── kanvas_buat.c      # Buat kanvas baru
│   ├── kanvas_hapus.c     # Hapus kanvas
│   ├── kanvas_ubahsuai.c  # Ubah ukuran
│   ├── kanvas_flip.c      # Flip buffer
│   └── kanvas_blit.c      # Blit antar kanvas
│
├── pengolah/               # Rendering engine
│   ├── pengolah.c         # Core render
│   ├── pengolah_cpu.c     # CPU software rendering
│   ├── pengolah_gpu.c     # GPU hardware rendering
│   ├── pengolah_hybrid.c  # CPU+GPU hybrid
│   ├── primitif.c         # Primitive dispatcher
│   ├── titik.c            # Draw point
│   ├── garis.c            # Draw line
│   ├── kotak.c            # Draw rectangle
│   ├── lingkaran.c        # Draw circle
│   ├── elips.c            # Draw ellipse
│   ├── poligon.c          # Draw polygon
│   ├── busur.c            # Draw arc
│   ├── kurva.c            # Draw bezier curve
│   └── isi.c              # Fill shapes
│
├── gpu_accel/              # GPU acceleration
│   ├── gpu_accel.c
│   ├── gpu_blit.c
│   ├── gpu_fill.c
│   ├── gpu_copy.c
│   ├── gpu_scale.c
│   ├── gpu_rotate.c
│   └── gpu_blend.c
│
├── teks/                   # Text and font rendering
│   ├── teks.c
│   ├── font.c
│   ├── font_bitmap.c
│   ├── font_ttf.c
│   ├── font_cache.c
│   ├── glyph.c
│   ├── ukuran.c
│   └── pengolah_teks.c
│
└── jendela/                # Window management
    ├── jendela.c
    ├── wm.c
    ├── dekorasi.c
    ├── peristiwa.c
    ├── widget.c
    ├── tombol.c
    ├── kotakteks.c
    ├── kotakcentang.c
    ├── bargulir.c
    ├── menu.c
    ├── dialog.c
    └── z_order.c
```

### Struktur Data

```c
/* Kanvas - drawing surface */
typedef struct {
    tak_bertanda32_t lebar;
    tak_bertanda32_t tinggi;
    tak_bertanda32_t *piksel;      /* Buffer pixel */
    ukuran_t ukuran;               /* Ukuran buffer */
    tak_bertanda32_t pitch;        /* Bytes per row */
    bool_t dirty;                  /* Perlu update? */
    persegi_t damage;              /* Region yang berubah */
} kanvas_t;

/* Warna RGBA */
typedef struct {
    tak_bertanda8_t merah;
    tak_bertanda8_t hijau;
    tak_bertanda8_t biru;
    tak_bertanda8_t alpha;
} warna_t;

/* Persegi untuk clipping */
typedef struct {
    tanda32_t x, y;
    tak_bertanda32_t lebar, tinggi;
} persegi_t;
```

### API Utama

```c
/* Inisialisasi */
status_t pigura_mulai(framebuffer_t *fb);
void pigura_selesai(void);

/* Kanvas */
kanvas_t *kanvas_buat(int lebar, int tinggi);
void kanvas_hapus(kanvas_t *k);
void kanvas_clear(kanvas_t *k, warna_t warna);
void kanvas_flip(kanvas_t *k);

/* Primitif */
void pengolah_titik(kanvas_t *k, int x, int y, warna_t warna);
void pengolah_garis(kanvas_t *k, int x1, int y1, int x2, int y2,
                    warna_t warna);
void pengolah_kotak(kanvas_t *k, int x, int y, int w, int h,
                    warna_t warna, bool_t isi);
void pengolah_lingkaran(kanvas_t *k, int cx, int cy, int radius,
                        warna_t warna, bool_t isi);
void pengolah_teks(kanvas_t *k, int x, int y, const char *teks,
                   font_t *font, warna_t warna);

/* Blitting */
void kanvas_blit(kanvas_t *dest, int dx, int dy,
                 kanvas_t *src, int sx, int sy, int w, int h);
void kanvas_blit_alpha(kanvas_t *dest, int dx, int dy,
                       kanvas_t *src, tak_bertanda8_t alpha);
```

---

## Dekor Compositor

### Kenapa Perlu Compositor?

Menulis langsung ke framebuffer tanpa perantara memiliki beberapa kelemahan serius untuk sistem multi-aplikasi:

| Masalah | Tanpa Compositor | Dengan Dekor |
|---------|------------------|--------------|
| Keamanan | Aplikasi bisa menimpa aplikasi lain | Isolasi penuh via backbuffer |
| Stabilitas | Crash = framebuffer rusak | Crash = 1 window affected |
| Koordinasi | Race condition, chaos | Terkoordinasi via compositor |
| Z-order | Tidak bisa | Didukung penuh |
| Efek visual | Tidak bisa | Shadow, transparency, dll |

### Arsitektur Dekor

```
  +------------------+     +------------------+
  | Aplikasi A       |     | Aplikasi B       |
  | (backbuffer)     |     | (backbuffer)     |
  +--------+---------+     +--------+---------+
           |                        |
           v                        v
  +--------------------------------------------+
  |                DEKOR COMPOSITOR            |
  |  - Clipping (isolasi)                     |
  |  - Z-order sorting                         |
  |  - Compositing (gabungkan)                |
  |  - Efek visual (opsional)                  |
  +--------------------+-----------------------+
                       |
                       v
  +--------------------------------------------+
  |              FRAMEBUFFER                    |
  |           (tampilan akhir)                  |
  +--------------------------------------------+
```

### Struktur Direktori Dekor

```
SUMBER/dekor/
│
├── inti/                   # Core compositor
│   ├── dekor.c             # Main interface
│   ├── komponis.c          # Compositing engine
│   ├── pengolah.c          # Render loop
│   ├── init.c              # Inisialisasi
│   └── hancurkan.c         # Cleanup
│
├── buffer/                  # Buffer management
│   ├── buffer.c
│   ├── bufferbelakang.c    # Per-window backbuffer
│   ├── permukaan.c
│   ├── dampak.c            # Damage tracking
│   └── tukar.c             # Buffer swap
│
├── peristiwa/               # Event routing
│   ├── peristiwa.c
│   ├── pengendali.c        # Event dispatcher
│   ├── masukan.c           # Input events
│   ├── penunjuk.c          # Mouse events
│   └── fokus.c             # Focus management
│
├── klip/                    # Clipping
│   ├── klip.c
│   ├── region.c
│   ├── kotak.c
│   ├── union.c
│   └── intersect.c
│
├── lapisan/                 # Z-order management
│   ├── lapisan.c
│   ├── z_order.c
│   ├── tumpuk.c
│   ├── naik.c
│   └── rendah.c
│
└── efek/                    # Visual effects (opsional)
    ├── efek.c
    ├── shadow.c
    ├── blur.c
    ├── transparan.c
    └── animasi.c
```

### API Dekor

```c
/* Inisialisasi */
int dekor_mulai(void);
void dekor_selesai(void);

/* Window management */
int dekor_jendela_buat(int x, int y, int lebar, int tinggi);
void dekor_jendela_hapus(int id);
void dekor_jendela_tampilkan(int id);
void dekor_jendela_sembunyikan(int id);

/* Buffer */
tak_bertanda32_t *dekor_buffer_dapatkan(int id);
void dekor_buffer_submit(int id);

/* Z-order */
void dekor_jendela_raise(int id);     /* Bawa ke depan */
void dekor_jendela_lower(int id);     /* Kirim ke belakang */

/* Event */
int dekor_event_poll(struct dekor_event *ev);
```

---

## Sistem Font

### Bitmap vs Vector Font

| Aspek | Bitmap Font | Vector Font (TTF/OTF) |
|-------|-------------|------------------------|
| Storage | Simpel array | Kurva matematika |
| Scaling | Tidak bisa | Tak terbatas |
| Rendering | Langsung | Perlu rasterisasi |
| Kualitas | Pecah jika zoom | Selalu tajam |
| Ukuran code | ~5 KB | ~200 KB (FreeType) |

### Pendekatan Hybrid Pigura OS

```
Bitmap Font (Prioritas)
├── Boot menu
├── Dialog sistem
├── Terminal
├── File manager
└── UI sistem lainnya

TTF/OTF (Opsional)
├── Editor gambar
├── PDF viewer
└── Word processor
```

### Format Pigura Font (.pf)

Header struktur:

| Offset | Ukuran | Deskripsi |
|--------|--------|-----------|
| 0 | 4 byte | Magic: "PFNT" |
| 4 | 1 byte | Versi format |
| 5 | 1 byte | Tinggi font (pixel) |
| 6 | 1 byte | Baseline offset |
| 7 | 1 byte | Reserved |
| 8 | 4 byte | Jumlah glyph |
| 12 | 4 byte | Offset ke data glyph |

### API Font

```c
/* Muat font */
font_t *font_muat(const char *path);
font_t *font_muat_builtin(void);  /* Font default 8x8 */

/* Hapus font */
void font_hapus(font_t *f);

/* Render teks */
void teks_render(kanvas_t *k, int x, int y, const char *teks,
                 font_t *f, warna_t warna);

/* Ukuran teks */
void teks_ukuran(const char *teks, font_t *f, int *lebar, int *tinggi);
```

---

## API Reference

### Framebuffer

```c
/* Inisialisasi */
status_t fb_init(framebuffer_info_t *info);
void fb_shutdown(void);

/* Mode setting */
status_t fb_set_mode(tak_bertanda32_t lebar, tak_bertanda32_t tinggi,
                     tak_bertanda32_t bpp);
status_t fb_get_mode(tak_bertanda32_t *lebar, tak_bertanda32_t *tinggi,
                     tak_bertanda32_t *bpp);

/* Render */
void fb_clear(tak_bertanda32_t warna);
void fb_flip(void);
void fb_present(void);

/* Console */
void fb_console_init(framebuffer_t *fb);
void fb_console_putchar(char c);
void fb_console_puts(const char *s);
void fb_console_clear(void);

/* Cursor */
void fb_cursor_show(void);
void fb_cursor_hide(void);
void fb_cursor_move(int x, int y);
```

### LibPigura

```c
/* Kanvas */
kanvas_t *kanvas_buat(int lebar, int tinggi);
void kanvas_hapus(kanvas_t *k);
void kanvas_resize(kanvas_t *k, int lebar_baru, int tinggi_baru);
void kanvas_clear(kanvas_t *k, warna_t warna);

/* Primitif 2D */
void pigura_titik(kanvas_t *k, int x, int y, warna_t w);
void pigura_garis(kanvas_t *k, int x1, int y1, int x2, int y2, warna_t w);
void pigura_kotak(kanvas_t *k, int x, int y, int w, int h, warna_t color);
void pigura_kotak_isi(kanvas_t *k, int x, int y, int w, int h, warna_t w);
void pigura_lingkaran(kanvas_t *k, int cx, int cy, int r, warna_t w);
void pigura_lingkaran_isi(kanvas_t *k, int cx, int cy, int r, warna_t w);
void pigura_ellipse(kanvas_t *k, int cx, int cy, int rx, int ry, warna_t w);
void pigura_poligon(kanvas_t *k, titik_t *points, int n, warna_t w);

/* Teks */
void pigura_teks(kanvas_t *k, int x, int y, const char *str,
                 font_t *f, warna_t w);
```

---

## Perbandingan Kompleksitas

### Kode

| Komponen | X11/Wayland | Pigura OS |
|----------|-------------|-----------|
| Display Server | ~500,000+ baris | 0 (tidak ada) |
| Compositor | ~100,000+ baris | ~5,000 baris |
| Client Library | ~50,000+ baris | ~5,000 baris |
| **Total** | **~500 MB** | **~100 KB** |

### Performa

| Metrik | X11/Wayland | Pigura OS |
|--------|-------------|-----------|
| IPC Calls | Ribuan per frame | 0 |
| Memory Copy | Multiple | Minimal |
| Protocol Parsing | Ya | Tidak |
| Network Overhead | Ya | Tidak |

### Overhead

```
X11/Wayland:
  Aplikasi → Encode Request → Socket → X Server → Decode →
  Driver → Render → Copy → Display
  Overhead: ~30-50% CPU time

Pigura OS:
  Aplikasi → LibPigura → Dekor → Framebuffer
  Overhead: ~5-10% CPU time
```

---

## Ringkasan

Sistem grafis Pigura OS dirancang dengan prinsip:

1. **Tanpa Server** - Tidak ada X11/Wayland server
2. **Langsung** - Akses langsung ke framebuffer
3. **Akselerasi** - GPU acceleration jika tersedia
4. **Isolasi** - Compositor untuk keamanan
5. **Ringkas** - ~100 KB vs ~500 MB

Pendekatan ini menghasilkan sistem grafis yang:
- Cepat (minimal overhead)
- Kecil (99% lebih kecil dari X11)
- Sederhana (mudah dipahami dan di-maintain)
- Aman (isolasi aplikasi via compositor)