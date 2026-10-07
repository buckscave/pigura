/* ----------------------------------------------------------------------------------------------
 * pigura/pigura.h - Master include
 * ----------------------------------------------------------------------------------------------
 * Cukup sertakan satu header ini untuk menggunakan seluruh toolkit.
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_H
#define PIGURA_H

#include "pigura/tipe.h"
#include "pigura/versi.h"
#include "pigura/galat.h"
#include "pigura/catat.h"
#include "pigura/untaian.h"
#include "pigura/aksi.h"
#include "pigura/permukaan.h"
#include "pigura/gambar.h"
#include "pigura/codec.h"
#include "pigura/font.h"
#include "pigura/utf8.h"
#include "pigura/timer.h"
#include "pigura/dialog_file.h"
#include "pigura/masukan.h"
#include "pigura/layar.h"
#include "pigura/papan_klip.h"
#include "pigura/perulangan.h"
#include "pigura/widget.h"
#include "pigura/kotak.h"
#include "pigura/splitter.h"
#include "pigura/tab.h"
#include "pigura/panel.h"
#include "pigura/kolaps.h"
#include "pigura/toolbar.h"
#include "pigura/status_bar.h"
#include "pigura/tooltip.h"
#include "pigura/dialog_modal.h"
#include "pigura/dock.h"
#include "pigura/snap.h"
#include "pigura/wm.h"
#include "pigura/passwordinput.h"
#include "pigura/contextmenu.h"
#include "pigura/spinbutton.h"
#include "pigura/treeview.h"
#include "pigura/tableview.h"
#include "pigura/colorpicker.h"
#include "pigura/datepicker.h"
#include "pigura/label.h"
#include "pigura/tombol.h"
#include "pigura/isian_teks.h"
#include "pigura/multi_teks.h"
#include "pigura/gambar_widget.h"
#include "pigura/bilah_geser.h"
#include "pigura/angka_putar.h"
#include "pigura/kotak_pencarian.h"
#include "pigura/kotak_gabungan.h"
#include "pigura/kotak_penanda.h"
#include "pigura/tombol_radio.h"
#include "pigura/kemajuan.h"
#include "pigura/gulir.h"
#include "pigura/daftar.h"
#include "pigura/menubar.h"
#include "pigura/dropdown.h"
#include "pigura/jendela_widget.h"
#include "pigura/transform.h"
#include "pigura/tema.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Inisialisasi pigura global state (logging, error TLS, dll). Aman
 * dipanggil lebih dari sekali; pemanggilan berikutnya no-op.
 */
pg_galat pigura_init(void);

/* Bersihkan global state. Aman dipanggil lebih dari sekali. */
pg_galat pigura_selesai(void);

/* Mengembalikan nonzero jika pigura sudah di-init. */
pg_bool pigura_sudah_init(void);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_H */
