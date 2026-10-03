/* ----------------------------------------------------------------------------------------------
 * pigura/timer.h - Timer satu-kali dan berulang
 * ----------------------------------------------------------------------------------------------
 * Timer minimal berbasis linked list global, di-pump dari event
 * loop. Timer berjalan di thread yang sama dengan event loop
 * (single-threaded); aman dipanggil dari callback.
 *
 * Penggunaan:
 *   pg_timer_init();           // di pigura_init()
 *   pg_timer_t *t = pg_timer_berulang(500, blink_cb, ctx);
 *   ...
 *   pg_timer_hancur(t);         // batalkan
 *   pg_timer_selesai();        // di pigura_selesai()
 *
 * Event loop memanggil pg_timer_pompa() tiap iterasi; return value
 * = ms sampai timer berikutnya expire, dipakai sebagai sleep
 * timeout (mengganti pg_tidur_ms(1) hard-coded).
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_TIMER_H
#define PIGURA_TIMER_H

#include "pigura/tipe.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pg_timer pg_timer_t;
typedef void (*pg_timer_cb)(void *ctx);

/* Buat timer satu kali: cb dipanggil sekali setelah ms milidetik.
 * Mengembalikan NULL bila gagal alokasi atau sistem belum init.
 * ctx boleh NULL. */
pg_timer_t *pg_timer_buat(unsigned ms, pg_timer_cb cb, void *ctx);

/* Buat timer berulang: cb dipanggil tiap ms milidetik sampai
 * pg_timer_hancur() dipanggil. Mengembalikan NULL bila gagal.
 * ctx boleh NULL. */
pg_timer_t *pg_timer_berulang(unsigned ms, pg_timer_cb cb, void *ctx);

/* Batalkan timer. Aman bila NULL. Setelah panggilan ini, ctx
 * tidak lagi diakses oleh timer system. */
void pg_timer_hancur(pg_timer_t *t);

/* Pompa semua timer yang sudah expired. Dipanggil oleh event loop
 * tiap iterasi. Mengembalikan jumlah milidetik sampai timer
 * berikutnya expire (untuk dipakai sebagai select/poll timeout),
 * atau 0xFFFFFFFF bila tidak ada timer aktif (boleh tidur lama). */
unsigned pg_timer_pompa(void);

/* Init global timer system. Aman dipanggil lebih dari sekali. */
pg_galat pg_timer_init(void);

/* Shutdown global timer system: hancurkan SEMUA timer aktif.
 * Aman dipanggil lebih dari sekali. */
void pg_timer_selesai(void);

/* Hitung jumlah timer aktif (debug + test). */
int pg_timer_aktif(void);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_TIMER_H */
