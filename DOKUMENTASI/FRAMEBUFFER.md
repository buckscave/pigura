# SISTEM GRAFIS PIGURA OS

## Daftar Isi

1. [Filosofi Tanpa X11/Wayland](#filosofi-tanpa-x11-wayland)
2. [Arsitektur Video Driver](#arsitektur-video-driver)
3. [Arsitektur GPU Driver](#arsitektur-gpu-driver)
4. [Pigura All-in-One Graphics Layer](#pigura-all-in-one-graphics-layer)
5. [Sistem Font](#sistem-font)
6. [API Reference](#api-reference)
7. [Perbandingan Kompleksitas](#perbandingan-kompleksitas)

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

Windows menggunakan Graphics Device Interface (GDI) yang terintegrasi langsung tanpa arsitektur server-client. Pigura OS mengadopsi pendekatan serupa dengan inovasi tambahan: mengintegrasikan Compositor, Window Manager, Toolkit, dan Desktop Environment dalam satu modul.

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
│  Aplikasi → Pigura (All-in-One) → Video Driver → Framebuffer   │
│              ↑_______________↑                                   │
│            Direct Call                                          │
│  Total: ~200 KB kode                                            │
└─────────────────────────────────────────────────────────────────┘
```

---

## Arsitektur Video Driver

### Konsep Dasar

Video driver di Pigura OS bertanggung jawab untuk menginisialisasi display hardware dan menyediakan akses ke framebuffer. Berbeda dengan GPU driver, video driver bersifat sederhana dan hanya menangani mode setting dasar.

### Lokasi dan Struktur

```
SUMBER/perangkat/tampilan/video/
│
├── video.c              # Modul utama video
├── video_init.c         # Inisialisasi video
├── vbe.c                # VESA BIOS Extensions (BIOS boot)
└── uefi_gop.c           # UEFI Graphics Output Protocol
```

### Komponen Video Driver

#### 1. video.c - Modul Utama

```c
/*
 * Fungsi utama video driver
 */

/* Inisialisasi subsistem video */
status_t video_init(void);

/* Mendapatkan informasi display */
status_t video_get_info(video_info_t *info);

/* Mengatur mode display */
status_t video_set_mode(tak_bertanda32_t lebar,
                        tak_bertanda32_t tinggi,
                        tak_bertanda32_t bpp);
```

#### 2. vbe.c - VESA BIOS Extensions

Driver VBE digunakan untuk sistem yang boot via BIOS (legacy boot). VBE menyediakan akses ke mode grafis melalui interrupt BIOS.

```c
/*
 * VBE Functions
 */

/* Inisialisasi VBE */
status_t vbe_init(void);

/* Mendapatkan info VBE */
status_t vbe_get_info(vbe_info_t *info);

/* Mendapatkan info mode */
status_t vbe_get_mode_info(tak_bertanda16_t mode,
                           vbe_mode_info_t *info);

/* Mengatur mode VBE */
status_t vbe_set_mode(tak_bertanda16_t mode);

/* Mencari mode yang sesuai */
tak_bertanda32_t vbe_find_mode(tak_bertanda32_t lebar,
                               tak_bertanda32_t tinggi,
                               tak_bertanda32_t bpp);
```

#### 3. uefi_gop.c - UEFI Graphics Output Protocol

Driver GOP digunakan untuk sistem yang boot via UEFI. GOP menyediakan akses langsung ke framebuffer tanpa perlu mode switching.

```c
/*
 * UEFI GOP Functions
 */

/* Inisialisasi GOP */
status_t uefi_gop_init(void);

/* Mendapatkan info GOP */
status_t uefi_gop_get_info(void *gop,
                           tak_bertanda32_t *lebar,
                           tak_bertanda32_t *tinggi,
                           tak_bertanda32_t *bpp,
                           tak_bertanda64_t *fb_addr,
                           ukuran_t *fb_size);

/* Mengatur mode GOP */
status_t uefi_gop_set_mode(void *gop, tak_bertanda32_t mode);

/* Mencari mode yang sesuai */
tak_bertanda32_t uefi_gop_find_mode(void *gop,
                                    tak_bertanda32_t lebar,
                                    tak_bertanda32_t tinggi,
                                    tak_bertanda32_t bpp);
```

### Alur Inisialisasi Video

```
START
  │
  v
┌──────────────────┐
│ Boot Loader      │  BIOS/UEFI
│ (multiboot)      │
└────────┬─────────┘
         │
         v
┌──────────────────┐
│ Kernel Entry     │  kernel_main()
└────────┬─────────┘
         │
         v
┌──────────────────┐
│ video_init()     │  Inisialisasi subsistem video
└────────┬─────────┘
         │
         ├─────────────────┐
         │                 │
         v                 v
┌──────────────────┐  ┌──────────────────┐
│ BIOS Boot?       │  │ UEFI Boot?       │
│ → vbe_init()     │  │ → uefi_gop_init()│
└────────┬─────────┘  └────────┬─────────┘
         │                     │
         └──────────┬──────────┘
                    │
                    v
         ┌──────────────────┐
         │ Get Framebuffer  │
         │ Address & Info   │
         └────────┬─────────┘
                  │
                  v
         ┌──────────────────┐
         │ Map Framebuffer  │
         │ to Kernel Space  │
         └────────┬─────────┘
                  │
                  v
         ┌──────────────────┐
         │ pigura_mulai()   │  Initialize graphics layer
         └──────────────────┘
```

### Struktur Data Video

```c
/* Informasi video */
typedef struct {
    alamat_virtual_t alamat;       /* Alamat virtual framebuffer */
    alamat_fisik_t alamat_fisik;   /* Alamat fisik framebuffer */
    tak_bertanda32_t lebar;        /* Lebar dalam pixel */
    tak_bertanda32_t tinggi;       /* Tinggi dalam pixel */
    tak_bertanda32_t pitch;        /* Bytes per scanline */
    tak_bertanda32_t bpp;          /* Bits per pixel */
    tak_bertanda32_t merah_mask;   /* Mask warna merah */
    tak_bertanda32_t hijau_mask;   /* Mask warna hijau */
    tak_bertanda32_t biru_mask;    /* Mask warna biru */
    tak_bertanda32_t alpha_mask;   /* Mask alpha */
} video_info_t;
```

---

## Arsitektur GPU Driver

### Konsep Dasar

GPU driver di Pigura OS bertanggung jawab untuk akselerasi grafis hardware. Driver ini bersifat opsional - jika GPU tidak tersedia atau tidak didukung, sistem akan fallback ke software rendering.

### Lokasi dan Struktur

```
SUMBER/perangkat/tampilan/gpu/
│
├── gpu.c                # Core GPU driver
├── gpu_deteksi.c        # Deteksi via IC Detection
├── gpu_init.c           # Inisialisasi GPU
├── gpu_render.c         # Render commands
├── gpu_2d.c             # 2D acceleration
├── gpu_3d.c             # 3D acceleration
├── gpu_memori.c         # GPU memory management
├── gpu_command.c        # Command buffer
└── gpu_shader.c         # Shader handling
```

### Deteksi GPU

GPU dideteksi menggunakan IC Detection System:

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

### Operasi GPU

| Operasi | Fungsi | Deskripsi |
|---------|--------|-----------|
| Blit | `gpu_blit()` | Copy antar permukaan |
| Fill | `gpu_fill_rect()` | Isi kotak dengan warna |
| Copy | `gpu_mem_copy()` | Copy memori GPU |
| Scale | `gpu_scale()` | Skala gambar |
| Rotate | `gpu_rotate()` | Rotasi gambar |
| Blend | `gpu_blend()` | Alpha blending |
| 2D Draw | `gpu_2d_*` | Primitive 2D |
| 3D Draw | `gpu_3d_*` | Primitive 3D |

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

## Pigura All-in-One Graphics Layer

### Gambaran Umum

Pigura adalah layer grafis terintegrasi yang menggabungkan Compositor, Window Manager, Toolkit, dan Desktop Environment dalam satu modul. Layer ini berkomunikasi langsung dengan video driver tanpa perantara.

### Struktur Direktori

```
SUMBER/pigura/
│
├── pigura.c                # Entry point utama
│
├── framebuffer/            # Software Rendering & Buffer Management
│   ├── framebuffer.c       # Core framebuffer handling
│   ├── framebuffer_blit.c  # Blitting operations
│   ├── framebuffer_render.c# Render ke framebuffer
│   │
│   ├── akselerasi_gpu/     # GPU Acceleration (opsional)
│   │   ├── gpu_accel.c
│   │   ├── gpu_blit.c
│   │   ├── gpu_fill.c
│   │   ├── gpu_copy.c
│   │   ├── gpu_scale.c
│   │   ├── gpu_rotate.c
│   │   └── gpu_blend.c
│   │
│   ├── buffer/             # Buffer Management
│   │   ├── buffer.c
│   │   ├── bufferbelakang.c
│   │   ├── permukaan.c
│   │   ├── dampak.c
│   │   └── tukar.c
│   │
│   ├── kanvas/             # Canvas/Drawing Surface
│   │   ├── kanvas.c
│   │   ├── kanvas_buat.c
│   │   ├── kanvas_hapus.c
│   │   ├── kanvas_ubahsuai.c
│   │   ├── kanvas_flip.c
│   │   └── kanvas_blit.c
│   │
│   └── pengolah/           # Rendering Engine
│       ├── pengolah.c
│       ├── pengolah_cpu.c
│       ├── pengolah_gpu.c
│       ├── pengolah_hybrid.c
│       ├── primitif.c
│       ├── titik.c
│       ├── garis.c
│       ├── kotak.c
│       ├── lingkaran.c
│       ├── elip.c
│       ├── poligon.c
│       ├── busur.c
│       ├── kurva.c
│       └── isi.c
│
├── penata/                 # COMPOSITOR
│   ├── inti/
│   ├── buffer/
│   ├── klip/
│   ├── lapisan/
│   └── efek/
│
├── jendela/                # WINDOW MANAGER
│   ├── jendela.c
│   ├── wm.c
│   ├── dekorasi.c
│   ├── peristiwa.c
│   └── z_order.c
│
├── peristiwa/              # EVENT HANDLING
│   ├── peristiwa.c
│   ├── pengendali.c
│   ├── masukan.c
│   ├── penunjuk.c
│   └── fokus.c
│
├── teks/                   # TEXT & FONT RENDERING
│   ├── teks.c
│   ├── font.c
│   ├── font_bitmap.c
│   ├── font_ttf.c
│   ├── font_cache.c
│   ├── glyph.c
│   ├── ukuran.c
│   └── pengolah_teks.c
│
└── widget/                 # TOOLKIT / WIDGETS
    ├── widget.c
    ├── tombol.c
    ├── kotakteks.c
    ├── kotakcentang.c
    ├── bargulir.c
    ├── menu.c
    └── dialog.c
```

### Alur Data Grafis

```
┌─────────────────────────────────────────────────────────────────┐
│                    ALUR DATA GRAFIS                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  APLIKASI LAYER                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                      widget/                             │   │
│  │  ┌───────┐ ┌───────┐ ┌───────┐ ┌───────┐ ┌───────┐    │   │
│  │  │Button │ │TextBox│ │ Menu  │ │Dialog │ │ dll   │    │   │
│  │  └───┬───┘ └───┬───┘ └───┬───┘ └───┬───┘ └───┬───┘    │   │
│  │      └─────────┴─────────┴─────────┴─────────┘         │   │
│  │                          │                               │   │
│  └──────────────────────────┼───────────────────────────────┘   │
│                             │                                   │
│  COMPOSITOR LAYER           v                                   │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                      penata/                             │   │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐   │   │
│  │  │ Clipping │ │ Z-Order  │ │ Effects  │ │ Buffer   │   │   │
│  │  │ (klip/)  │ │(lapisan/)│ │ (efek/)  │ │(buffer/) │   │   │
│  │  └────┬─────┘ └────┬─────┘ └────┬─────┘ └────┬─────┘   │   │
│  │       └────────────┴────────────┴────────────┘          │   │
│  │                         │                                │   │
│  └─────────────────────────┼────────────────────────────────┘   │
│                            │                                    │
│  RENDERING LAYER          v                                    │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                   framebuffer/                           │   │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐   │   │
│  │  │ Canvas   │ │ Render   │ │ GPU Acc  │ │ Buffer   │   │   │
│  │  │(kanvas/) │ │(pengolah)│ │(akselerasi)│ │(buffer/) │   │   │
│  │  └────┬─────┘ └────┬─────┘ └────┬─────┘ └────┬─────┘   │   │
│  │       └────────────┴────────────┴────────────┘          │   │
│  │                         │                                │   │
│  └─────────────────────────┼────────────────────────────────┘   │
│                            │                                    │
│                            v                                    │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                   FRAMEBUFFER                            │   │
│  │              (Memory-mapped display)                     │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### Buffer Management

#### Double Buffering

Setiap aplikasi memiliki back buffer sendiri untuk menghindari flickering dan tearing:

```c
/* Struktur buffer aplikasi */
typedef struct {
    tak_bertanda32_t *buffer;     /* Pointer ke buffer */
    tak_bertanda32_t lebar;       /* Lebar buffer */
    tak_bertanda32_t tinggi;      /* Tinggi buffer */
    ukuran_t ukuran;              /* Ukuran dalam byte */
    bool_t dirty;                 /* Perlu update? */
    persegi_t damage;             /* Region yang berubah */
} buffer_aplikasi_t;
```

#### Damage Tracking

Hanya region yang berubah yang di-blit ke framebuffer:

```c
/* Damage tracking */
typedef struct {
    tanda32_t x, y;
    tak_bertanda32_t lebar, tinggi;
} persegi_t;

/* Hitung damage region */
void damage_gabung(persegi_t *dest, persegi_t *src);
bool_t damage_intersect(persegi_t *a, persegi_t *b, persegi_t *result);
```

### Rendering Primitives

#### CPU Software Rendering

```c
/* Gambar titik */
void pengolah_titik(kanvas_t *k, int x, int y, tak_bertanda32_t warna);

/* Gambar garis (Bresenham) */
void pengolah_garis(kanvas_t *k, int x1, int y1, int x2, int y2,
                    tak_bertanda32_t warna);

/* Gambar kotak */
void pengolah_kotak(kanvas_t *k, int x, int y, int w, int h,
                    tak_bertanda32_t warna, bool_t isi);

/* Gambar lingkaran */
void pengolah_lingkaran(kanvas_t *k, int cx, int cy, int radius,
                        tak_bertanda32_t warna, bool_t isi);

/* Gambar teks */
void pengolah_teks(kanvas_t *k, int x, int y, const char *teks,
                   font_t *font, tak_bertanda32_t warna);
```

#### GPU Hardware Acceleration

```c
/* GPU accelerated operations */
status_t gpu_blit(gpu_context_t *ctx, kanvas_t *src, kanvas_t *dst,
                  int sx, int sy, int dx, int dy, int w, int h);

status_t gpu_fill_rect(gpu_context_t *ctx, kanvas_t *dst,
                       int x, int y, int w, int h,
                       tak_bertanda32_t warna);

status_t_gpu_blend(gpu_context_t *ctx, kanvas_t *src, kanvas_t *dst,
                   int x, int y, tak_bertanda8_t alpha);
```

---

## Sistem Font

### Pendekatan Hybrid

Pigura OS menggunakan pendekatan hybrid untuk font rendering:

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

### Bitmap Font

Bitmap font adalah font default yang cepat dan kecil:

```c
/* Font default 8x8 */
static tak_bertanda8_t font_8x8[256][8] = {
    /* Karakter 'A' (0x41) */
    [0x41] = {
        0x00,  /* ........ */
        0x18,  /* ...##... */
        0x3C,  /* ..####.. */
        0x66,  /* .##..##. */
        0x66,  /* .##..##. */
        0x7E,  /* .######. */
        0x66,  /* .##..##. */
        0x00   /* ........ */
    },
    /* ... */
};
```

### Format Pigura Font (.pf)

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
                 font_t *f, tak_bertanda32_t warna);

/* Ukuran teks */
void teks_ukuran(const char *teks, font_t *f,
                 int *lebar, int *tinggi);
```

---

## API Reference

### Video Driver

```c
/* Inisialisasi */
status_t video_init(void);
void video_shutdown(void);

/* Mode setting */
status_t video_set_mode(tak_bertanda32_t lebar,
                        tak_bertanda32_t tinggi,
                        tak_bertanda32_t bpp);
status_t video_get_mode(tak_bertanda32_t *lebar,
                        tak_bertanda32_t *tinggi,
                        tak_bertanda32_t *bpp);

/* Info */
status_t video_get_info(video_info_t *info);
```

### GPU Driver

```c
/* Inisialisasi */
status_t gpu_init(void);
void gpu_shutdown(void);

/* Memory */
void *gpu_mem_alloc(ukuran_t size);
void gpu_mem_free(void *ptr);

/* 2D Operations */
status_t gpu_blit(gpu_context_t *ctx, ...);
status_t gpu_fill_rect(gpu_context_t *ctx, ...);
status_t gpu_blend(gpu_context_t *ctx, ...);

/* 3D Operations */
status_t gpu_3d_draw_vertices(gpu_context_t *ctx, ...);
status_t gpu_shader_load(gpu_context_t *ctx, ...);
```

### Pigura Graphics

```c
/* Inisialisasi */
status_t pigura_mulai(video_info_t *video);
void pigura_selesai(void);

/* Kanvas */
kanvas_t *kanvas_buat(int lebar, int tinggi);
void kanvas_hapus(kanvas_t *k);
void kanvas_clear(kanvas_t *k, tak_bertanda32_t warna);
void kanvas_flip(kanvas_t *k);

/* Primitif */
void pengolah_titik(kanvas_t *k, int x, int y, tak_bertanda32_t w);
void pengolah_garis(kanvas_t *k, int x1, int y1, int x2, int y2,
                    tak_bertanda32_t w);
void pengolah_kotak(kanvas_t *k, int x, int y, int w, int h,
                    tak_bertanda32_t color, bool_t isi);

/* Blitting */
void kanvas_blit(kanvas_t *dest, int dx, int dy,
                 kanvas_t *src, int sx, int sy, int w, int h);
```

### Compositor

```c
/* Inisialisasi */
status_t penata_mulai(void);
void penata_selesai(void);

/* Window management */
int penata_jendela_buat(int x, int y, int lebar, int tinggi);
void penata_jendela_hapus(int id);
void penata_jendela_tampilkan(int id);
void penata_jendela_sembunyikan(int id);

/* Buffer */
tak_bertanda32_t *penata_buffer_dapatkan(int id);
void penata_buffer_submit(int id);

/* Z-order */
void penata_jendela_raise(int id);
void penata_jendela_lower(int id);
```

### Event System

```c
/* Event types */
typedef enum {
    PERISTIWA_TIDAK_ADA = 0,
    PERISTIWA_KETIK,        /* Keyboard press */
    PERISTIWA_KLIK,         /* Mouse click */
    PERISTIWA_GESER,        /* Mouse move */
    PERISTIWA_FOKUS,        /* Window focus */
    PERISTIWA_TUTUP,        /* Window close */
} peristiwa_jenis_t;

/* Event structure */
typedef struct {
    peristiwa_jenis_t jenis;
    tak_bertanda32_t jendela_id;
    union {
        struct { tak_bertanda32_t kode; } ketik;
        struct { int x, y; tak_bertanda32_t tombol; } klik;
        struct { int x, y; } geser;
    } data;
} peristiwa_t;

/* Poll event */
int peristiwa_poll(peristiwa_t *ev);
```

---

## Perbandingan Kompleksitas

### Kode

| Komponen | X11/Wayland + GTK | Pigura OS |
|----------|-------------------|-----------|
| Display Server | ~500,000+ baris | 0 (tidak ada) |
| Compositor | ~100,000+ baris | ~5,000 baris |
| Window Manager | ~50,000+ baris | ~2,000 baris |
| Toolkit | ~500,000+ baris | ~5,000 baris |
| Font Engine | ~200,000+ baris | ~3,000 baris |
| **Total** | **~500 MB** | **~200 KB** |

### Performa

| Metrik | X11/Wayland | Pigura OS |
|--------|-------------|-----------|
| IPC Calls | Ribuan per frame | 0 |
| Memory Copy | Multiple | Minimal |
| Protocol Parsing | Ya | Tidak |
| Network Overhead | Ya | Tidak |
| Context Switch | Sering | Jarang |

### Overhead

```
X11/Wayland:
  Aplikasi → Encode → Socket → X Server → Decode →
  Driver → Render → COPY → Display
  Overhead: ~30-50% CPU time

Pigura OS:
  Aplikasi → Pigura → Video Driver → Framebuffer
  Overhead: ~5-10% CPU time
```

---

## Ringkasan

Sistem grafis Pigura OS dirancang dengan prinsip:

1. **Tanpa Server** - Tidak ada X11/Wayland server
2. **Langsung** - Akses langsung ke framebuffer via video driver
3. **Terintegrasi** - Compositor + WM + Toolkit + DE dalam satu modul
4. **Akselerasi** - GPU acceleration jika tersedia (opsional)
5. **Ringkas** - ~200 KB vs ~500 MB

### Struktur Hardware → Software

```
┌─────────────────┐
│    Hardware     │
├─────────────────┤
│ Display Monitor │
│       ↑         │
│ Video Hardware  │  (VGA, LCD, HDMI)
│       ↑         │
│ GPU (opsional)  │
└────────┬────────┘
         │
         v
┌─────────────────┐
│ Driver Layer    │
├─────────────────┤
│ video/          │  VBE, UEFI GOP
│ gpu/            │  2D, 3D, Shader
└────────┬────────┘
         │
         v
┌─────────────────┐
│ Pigura Layer    │
├─────────────────┤
│ framebuffer/    │  Software rendering
│ penata/         │  Compositor
│ jendela/        │  Window Manager
│ widget/         │  Toolkit
│ teks/           │  Font rendering
└────────┬────────┘
         │
         v
┌─────────────────┐
│  Aplikasi       │
└─────────────────┘
```

Pendekatan ini menghasilkan sistem grafis yang:
- **Cepat** (minimal overhead)
- **Kecil** (99% lebih kecil dari X11)
- **Sederhana** (mudah dipahami dan di-maintain)
- **Aman** (isolasi aplikasi via compositor)
- **Portabel** (berjalan di berbagai arsitektur)
