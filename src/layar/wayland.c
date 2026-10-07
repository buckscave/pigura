/* ----------------------------------------------------------------------------------------------
 * pigura layar: wayland.c - backend Wayland native
 * ---------------------------------------------------------------------------------------------- */
#include "pigura/layar.h"
#include "pigura/galat.h"
#include "pigura/catat.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>

#include <wayland-client.h>
#include <wayland-client-protocol.h>

#define PG_WL_ANTRIAN_MAKS 64

struct pg_layar {
        struct wl_display *disp;
        struct wl_registry *reg;
        struct wl_compositor *compositor;
        struct wl_shm *shm;
        struct wl_shell *shell;
        struct wl_seat *seat;
        struct wl_surface *surface;
        struct wl_shell_surface *shell_surface;
        struct wl_pointer *pointer;
        struct wl_keyboard *keyboard;

        int lebar, tinggi, langkah;
        pg_warna_t *piksel;
        struct wl_shm_pool *pool;
        struct wl_buffer *buffer;
        int shm_fd;
        size_t shm_size;

        pg_bool terkunci;
        pg_bool tutup_diminta;

        pg_aksi_t antrian[PG_WL_ANTRIAN_MAKS];
        int kepala, ekor, jumlah;

        int px, py;
        int modifier;
};

static pg_u32 pg_wl_sekarang_ms(void)
{
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (pg_u32)((pg_u64)ts.tv_sec * 1000ULL +
                         (pg_u64)ts.tv_nsec / 1000000ULL);
}

static int pg_wl_buat_shm_fd(size_t size)
{
        int fd;
        char nama[64];
        pg_u32 pid = (pg_u32)getpid();
        pg_u32 t = (pg_u32)time(NULL);
        snprintf(nama, sizeof(nama), "/pigura-%u-%u", pid, t);
        fd = shm_open(nama, O_RDWR | O_CREAT | O_EXCL, 0600);
        if (fd < 0) return -1;
        shm_unlink(nama);
        if (ftruncate(fd, (off_t)size) < 0) {
                close(fd);
                return -1;
        }
        return fd;
}

static void pg_wl_dorong(pg_layar_t *l, const pg_aksi_t *e)
{
        if (l->jumlah < PG_WL_ANTRIAN_MAKS) {
                l->antrian[l->ekor] = *e;
                l->ekor = (l->ekor + 1) % PG_WL_ANTRIAN_MAKS;
                l->jumlah++;
        }
}

static int pg_wl_key_ke_tombol(uint32_t key)
{
        switch (key) {
        case 1:  return PG_TOMBOL_ESCAPE;
        case 14: return PG_TOMBOL_BACKSPACE;
        case 15: return PG_TOMBOL_TAB;
        case 28: return PG_TOMBOL_ENTER;
        case 102: return PG_TOMBOL_HOME;
        case 103: return PG_TOMBOL_ATAS;
        case 104: return PG_TOMBOL_PAGEUP;
        case 105: return PG_TOMBOL_KIRI;
        case 106: return PG_TOMBOL_KANAN;
        case 107: return PG_TOMBOL_END;
        case 108: return PG_TOMBOL_BAWAH;
        case 109: return PG_TOMBOL_PAGEDOWN;
        case 119: return PG_TOMBOL_DELETE;
        case 42:  return PG_TOMBOL_LSHIFT;
        case 54:  return PG_TOMBOL_RSHIFT;
        case 29:  return PG_TOMBOL_LCTRL;
        case 97:  return PG_TOMBOL_RCTRL;
        case 56:  return PG_TOMBOL_LALT;
        case 100: return PG_TOMBOL_RALT;
        default:
                if (key >= 30 && key <= 48) return 'a' + (key - 30);
                if (key >= 2 && key <= 10) return '1' + (key - 2);
                if (key == 11) return '0';
                if (key == 57) return ' ';
                return 0;
        }
}

static void pg_wl_registry_global(void *data, struct wl_registry *reg,
                                    uint32_t nama, const char *iface,
                                    uint32_t versi)
{
        pg_layar_t *l = (pg_layar_t *)data;
        (void)versi;
        if (strcmp(iface, "wl_compositor") == 0) {
                l->compositor = (struct wl_compositor *)
                        wl_registry_bind(reg, nama, &wl_compositor_interface, 3);
        } else if (strcmp(iface, "wl_shm") == 0) {
                l->shm = (struct wl_shm *)
                        wl_registry_bind(reg, nama, &wl_shm_interface, 1);
        } else if (strcmp(iface, "wl_shell") == 0) {
                l->shell = (struct wl_shell *)
                        wl_registry_bind(reg, nama, &wl_shell_interface, 1);
        } else if (strcmp(iface, "wl_seat") == 0) {
                l->seat = (struct wl_seat *)
                        wl_registry_bind(reg, nama, &wl_seat_interface, 4);
        }
}

static void pg_wl_registry_global_hapus(void *data, struct wl_registry *reg,
                                          uint32_t nama)
{
        (void)data; (void)reg; (void)nama;
}

static const struct wl_registry_listener pg_wl_registry_listener = {
        pg_wl_registry_global, pg_wl_registry_global_hapus
};

static void pg_wl_shell_ping(void *data, struct wl_shell_surface *ss, uint32_t serial)
{
        (void)data;
        wl_shell_surface_pong(ss, serial);
}

static void pg_wl_shell_konfigurasi(void *data, struct wl_shell_surface *ss,
                                      uint32_t tepi, int32_t lebar, int32_t tinggi)
{
        (void)data; (void)ss; (void)tepi; (void)lebar; (void)tinggi;
}

static const struct wl_shell_surface_listener pg_wl_shell_listener = {
        pg_wl_shell_ping, pg_wl_shell_konfigurasi, NULL
};

static void pg_wl_pointer_masuk(void *data, struct wl_pointer *p, uint32_t serial,
                                  struct wl_surface *surf, wl_fixed_t x, wl_fixed_t y)
{
        (void)p; (void)serial; (void)surf;
        pg_layar_t *l = (pg_layar_t *)data;
        l->px = wl_fixed_to_int(x);
        l->py = wl_fixed_to_int(y);
}

static void pg_wl_pointer_keluar(void *data, struct wl_pointer *p, uint32_t serial,
                                   struct wl_surface *surf)
{
        (void)data; (void)p; (void)serial; (void)surf;
}

static void pg_wl_pointer_gerak(void *data, struct wl_pointer *p, uint32_t waktu,
                                  wl_fixed_t x, wl_fixed_t y)
{
        (void)p; (void)waktu;
        pg_layar_t *l = (pg_layar_t *)data;
        pg_aksi_t e;
        memset(&e, 0, sizeof(e));
        e.tipe = PG_AKSI_TETIKUS_GERAK;
        e.waktu_ms = pg_wl_sekarang_ms();
        e.tetik_pos.x = wl_fixed_to_int(x);
        e.tetik_pos.y = wl_fixed_to_int(y);
        e.modifier = l->modifier;
        l->px = e.tetik_pos.x;
        l->py = e.tetik_pos.y;
        pg_wl_dorong(l, &e);
}

static void pg_wl_pointer_tombol(void *data, struct wl_pointer *p, uint32_t serial,
                                   uint32_t waktu, uint32_t tombol, uint32_t state)
{
        (void)p; (void)serial; (void)waktu;
        pg_layar_t *l = (pg_layar_t *)data;
        pg_aksi_t e;
        memset(&e, 0, sizeof(e));
        if (tombol == 0x110) e.tetik_tombol = PG_TETIKUS_KIRI;
        else if (tombol == 0x111) e.tetik_tombol = PG_TETIKUS_KANAN;
        else if (tombol == 0x112) e.tetik_tombol = PG_TETIKUS_TENGAH;
        else return;
        e.waktu_ms = pg_wl_sekarang_ms();
        e.tetik_pos.x = l->px;
        e.tetik_pos.y = l->py;
        e.modifier = l->modifier;
        e.tipe = (state == 1) ? PG_AKSI_TETIKUS_TEKAN : PG_AKSI_TETIKUS_LEPAS;
        pg_wl_dorong(l, &e);
}

static void pg_wl_pointer_gulir(void *data, struct wl_pointer *p, uint32_t waktu,
                                  wl_fixed_t dx, wl_fixed_t dy)
{
        (void)p; (void)waktu;
        pg_layar_t *l = (pg_layar_t *)data;
        pg_aksi_t e;
        int dyi = wl_fixed_to_int(dy);
        if (dyi == 0) return;
        memset(&e, 0, sizeof(e));
        e.tipe = PG_AKSI_TETIKUS_GULIR;
        e.waktu_ms = pg_wl_sekarang_ms();
        e.tetik_pos.x = l->px;
        e.tetik_pos.y = l->py;
        e.roda_dx = wl_fixed_to_int(dx);
        e.roda_dy = dyi;
        e.modifier = l->modifier;
        pg_wl_dorong(l, &e);
}

static const struct wl_pointer_listener pg_wl_pointer_listener = {
        pg_wl_pointer_masuk, pg_wl_pointer_keluar, pg_wl_pointer_gerak,
        pg_wl_pointer_tombol, NULL, NULL, NULL, NULL
};

static void pg_wl_keyboard_peta(void *data, struct wl_keyboard *kb, uint32_t format,
                                  int32_t fd, uint32_t size)
{
        (void)data; (void)kb; (void)format; (void)fd; (void)size;
}

static void pg_wl_keyboard_masuk(void *data, struct wl_keyboard *kb, uint32_t serial,
                                   struct wl_surface *surf, struct wl_array *keys)
{
        (void)data; (void)kb; (void)serial; (void)surf; (void)keys;
}

static void pg_wl_keyboard_keluar(void *data, struct wl_keyboard *kb, uint32_t serial,
                                    struct wl_surface *surf)
{
        (void)data; (void)kb; (void)serial; (void)surf;
}

static void pg_wl_keyboard_tombol(void *data, struct wl_keyboard *kb, uint32_t serial,
                                    uint32_t waktu, uint32_t key, uint32_t state)
{
        (void)kb; (void)serial; (void)waktu;
        pg_layar_t *l = (pg_layar_t *)data;
        pg_aksi_t e;
        int tombol = pg_wl_key_ke_tombol(key);
        if (tombol == 0) return;
        memset(&e, 0, sizeof(e));
        if (tombol == PG_TOMBOL_LSHIFT || tombol == PG_TOMBOL_RSHIFT) {
                if (state == 1) l->modifier |= PG_MOD_SHIFT;
                else            l->modifier &= ~PG_MOD_SHIFT;
        }
        if (tombol == PG_TOMBOL_LCTRL || tombol == PG_TOMBOL_RCTRL) {
                if (state == 1) l->modifier |= PG_MOD_CTRL;
                else            l->modifier &= ~PG_MOD_CTRL;
        }
        if (tombol == PG_TOMBOL_LALT || tombol == PG_TOMBOL_RALT) {
                if (state == 1) l->modifier |= PG_MOD_ALT;
                else            l->modifier &= ~PG_MOD_ALT;
        }
        e.tipe = (state == 1) ? PG_AKSI_TOMBOL_TURUN : PG_AKSI_TOMBOL_NAIK;
        e.tombol = tombol;
        e.modifier = l->modifier;
        e.unicode = (state == 1 && tombol >= 32 && tombol < 127) ? (pg_u32)tombol : 0;
        e.waktu_ms = pg_wl_sekarang_ms();
        pg_wl_dorong(l, &e);
}

static void pg_wl_keyboard_modifikasi(void *data, struct wl_keyboard *kb,
                                         uint32_t serial, uint32_t depresi,
                                         uint32_t latch, uint32_t lock, uint32_t grup)
{
        (void)data; (void)kb; (void)serial; (void)latch; (void)lock; (void)grup;
}

static void pg_wl_keyboard_ulang(void *data, struct wl_keyboard *kb, int32_t rate,
                                   int32_t delay)
{
        (void)data; (void)kb; (void)rate; (void)delay;
}

static const struct wl_keyboard_listener pg_wl_keyboard_listener = {
        pg_wl_keyboard_peta, pg_wl_keyboard_masuk, pg_wl_keyboard_keluar,
        pg_wl_keyboard_tombol, pg_wl_keyboard_modifikasi, pg_wl_keyboard_ulang
};

static void pg_wl_seat_capability(void *data, struct wl_seat *seat, uint32_t caps)
{
        pg_layar_t *l = (pg_layar_t *)data;
        (void)seat;
        if ((caps & WL_SEAT_CAPABILITY_POINTER) && !l->pointer) {
                l->pointer = wl_seat_get_pointer(l->seat);
                wl_pointer_add_listener(l->pointer, &pg_wl_pointer_listener, l);
        } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && l->pointer) {
                wl_pointer_release(l->pointer);
                l->pointer = NULL;
        }
        if ((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !l->keyboard) {
                l->keyboard = wl_seat_get_keyboard(l->seat);
                wl_keyboard_add_listener(l->keyboard, &pg_wl_keyboard_listener, l);
        } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && l->keyboard) {
                wl_keyboard_release(l->keyboard);
                l->keyboard = NULL;
        }
}

static const struct wl_seat_listener pg_wl_seat_listener = {
        pg_wl_seat_capability, NULL
};

pg_galat pg_buka_layar(pg_layar_t **out, const pg_layar_config_t *cfg)
{
        pg_layar_t *l;
        int lebar, tinggi;

        if (!out) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
        *out = NULL;

        l = (pg_layar_t *)calloc(1, sizeof(*l));
        if (!l) PG_KEMBALI_GALAT(PG_GALAT_MEMORI);

        lebar  = (cfg && cfg->lebar  > 0) ? cfg->lebar  : 640;
        tinggi = (cfg && cfg->tinggi > 0) ? cfg->tinggi : 480;
        l->lebar = lebar;
        l->tinggi = tinggi;
        l->langkah = lebar * (int)sizeof(pg_warna_t);
        l->shm_fd = -1;

        l->disp = wl_display_connect(NULL);
        if (!l->disp) {
                pg_set_galat(PG_GALAT_IO, "wayland: tidak bisa connect");
                free(l);
                return PG_GALAT_IO;
        }

        l->reg = wl_display_get_registry(l->disp);
        if (!l->reg) {
                pg_set_galat(PG_GALAT_UMUM, "wayland: registry gagal");
                wl_display_disconnect(l->disp);
                free(l);
                return PG_GALAT_UMUM;
        }
        wl_registry_add_listener(l->reg, &pg_wl_registry_listener, l);
        wl_display_roundtrip(l->disp);

        if (!l->compositor || !l->shm || !l->shell) {
                pg_set_galat(PG_GALAT_UMUM,
                             "wayland: compositor/shm/shell tidak tersedia");
                pg_tutup_layar(l);
                return PG_GALAT_UMUM;
        }

        if (l->seat) {
                wl_seat_add_listener(l->seat, &pg_wl_seat_listener, l);
                wl_display_roundtrip(l->disp);
        }

        l->surface = wl_compositor_create_surface(l->compositor);
        if (!l->surface) {
                pg_set_galat(PG_GALAT_UMUM, "wayland: surface gagal");
                pg_tutup_layar(l);
                return PG_GALAT_UMUM;
        }
        l->shell_surface = wl_shell_get_shell_surface(l->shell, l->surface);
        if (!l->shell_surface) {
                pg_set_galat(PG_GALAT_UMUM, "wayland: shell_surface gagal");
                pg_tutup_layar(l);
                return PG_GALAT_UMUM;
        }
        wl_shell_surface_add_listener(l->shell_surface, &pg_wl_shell_listener, l);
        wl_shell_surface_set_toplevel(l->shell_surface);
        if (cfg && cfg->judul) {
                wl_shell_surface_set_title(l->shell_surface, cfg->judul);
        }

        l->shm_size = (size_t)lebar * tinggi * sizeof(pg_warna_t);
        l->shm_fd = pg_wl_buat_shm_fd(l->shm_size);
        if (l->shm_fd < 0) {
                pg_set_galat(PG_GALAT_IO, "wayland: shm_open gagal");
                pg_tutup_layar(l);
                return PG_GALAT_IO;
        }
        l->piksel = (pg_warna_t *)mmap(NULL, l->shm_size, PROT_READ | PROT_WRITE,
                                        MAP_SHARED, l->shm_fd, 0);
        if (l->piksel == MAP_FAILED) {
                l->piksel = NULL;
                pg_set_galat(PG_GALAT_IO, "wayland: mmap gagal");
                pg_tutup_layar(l);
                return PG_GALAT_IO;
        }
        memset(l->piksel, 0, l->shm_size);

        l->pool = wl_shm_create_pool(l->shm, l->shm_fd, (int32_t)l->shm_size);
        l->buffer = wl_shm_pool_create_buffer(l->pool, 0, lebar, tinggi,
                                                l->langkah, WL_SHM_FORMAT_ARGB8888);

        pg_info("wayland: %dx%d ARGB8888, shm=%zu byte", lebar, tinggi, l->shm_size);

        *out = l;
        return PG_OK;
}

pg_galat pg_tutup_layar(pg_layar_t *l)
{
        if (!l) return PG_OK;
        if (l->buffer) wl_buffer_destroy(l->buffer);
        if (l->pool) wl_shm_pool_destroy(l->pool);
        if (l->piksel && l->piksel != MAP_FAILED)
                munmap(l->piksel, l->shm_size);
        if (l->shm_fd >= 0) close(l->shm_fd);
        if (l->keyboard) wl_keyboard_release(l->keyboard);
        if (l->pointer) wl_pointer_release(l->pointer);
        if (l->shell_surface) wl_shell_surface_destroy(l->shell_surface);
        if (l->surface) wl_surface_destroy(l->surface);
        if (l->seat) wl_seat_destroy(l->seat);
        if (l->shell) wl_shell_destroy(l->shell);
        if (l->shm) wl_shm_destroy(l->shm);
        if (l->compositor) wl_compositor_destroy(l->compositor);
        if (l->reg) wl_registry_destroy(l->reg);
        if (l->disp) wl_display_disconnect(l->disp);
        free(l);
        return PG_OK;
}

pg_galat pg_layar_kueri(pg_layar_t *l, pg_layar_info_t *info)
{
        if (!l || !info) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
        info->lebar = l->lebar;
        info->tinggi = l->tinggi;
        info->bpp = 32;
        info->langkah = l->langkah;
        info->format = PG_LAYAR_FORMAT_A8R8G8B8;
        info->double_buffer = PG_SALAH;
        return PG_OK;
}

pg_galat pg_layar_kunci(pg_layar_t *l, void **piksel, int *langkah)
{
        if (!l || !l->piksel) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
        l->terkunci = PG_BENAR;
        *piksel = l->piksel;
        if (langkah) *langkah = l->langkah;
        return PG_OK;
}

pg_galat pg_layar_buka_kunci(pg_layar_t *l)
{
        if (!l) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
        l->terkunci = PG_SALAH;
        return PG_OK;
}

pg_galat pg_layar_presentasi(pg_layar_t *l)
{
        if (!l) PG_KEMBALI_GALAT(PG_GALAT_ARGUMEN);
        wl_surface_attach(l->surface, l->buffer, 0, 0);
        wl_surface_damage(l->surface, 0, 0, l->lebar, l->tinggi);
        wl_surface_commit(l->surface);
        wl_display_dispatch(l->disp);
        return PG_OK;
}

pg_galat pg_layar_tunggu_vsync(pg_layar_t *l)
{
        (void)l;
        return PG_OK;
}

pg_galat pg_layar_pompa_aksi(pg_layar_t *l)
{
        if (!l || !l->disp) return PG_OK;
        while (wl_display_prepare_read(l->disp) != 0) {
                if (wl_display_dispatch_pending(l->disp) == -1) break;
        }
        wl_display_flush(l->disp);
        if (wl_display_read_events(l->disp) == -1) return PG_GALAT_IO;
        wl_display_dispatch_pending(l->disp);
        return PG_OK;
}

void *pg_layar_handle_native(pg_layar_t *l)
{
        return l ? (void *)l->disp : NULL;
}

unsigned long pg_layar_jendela_id(pg_layar_t *l)
{
        return l ? (unsigned long)(uintptr_t)l->surface : 0;
}

pg_bool pg_layar_punya_aksi(pg_layar_t *l)
{
        if (!l) return PG_SALAH;
        return l->jumlah > 0 ? PG_BENAR : PG_SALAH;
}

pg_bool pg_layar_aksi_berikutnya(pg_layar_t *l, pg_aksi_t *out)
{
        if (!l || !out || l->jumlah == 0) return PG_SALAH;
        pg_layar_pompa_aksi(l);
        if (l->jumlah == 0) return PG_SALAH;
        *out = l->antrian[l->kepala];
        l->kepala = (l->kepala + 1) % PG_WL_ANTRIAN_MAKS;
        l->jumlah--;
        if (out->tipe == PG_AKSI_KELUAR) l->tutup_diminta = PG_BENAR;
        return PG_BENAR;
}
