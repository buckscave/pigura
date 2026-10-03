/* ----------------------------------------------------------------------------------------------
 * pigura/dialog_file.h - Dialog file native
 * ----------------------------------------------------------------------------------------------
 * Membuka dialog file open/save native platform:
 *
 *   X11/Linux: zenity atau kdialog via popen()
 *   Windows:   GetOpenFileName / GetSaveFileName (comdlg32)
 *   Fallback:  return NULL + galat PG_GALAT_TANPA
 *
 * Filter adalah string comma-separated extension tanpa dot, mis.
 * "png,jpg,bmp". NULL = semua file.
 *
 * Return: path yang dipilih user (malloc'd, caller free). NULL
 * jika dibatalkan atau gagal.
 * ---------------------------------------------------------------------------------------------- */
#ifndef PIGURA_DIALOG_FILE_H
#define PIGURA_DIALOG_FILE_H

#include "pigura/tipe.h"
#include "pigura/layar.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Tipe dialog file. */
typedef enum {
        PG_DIALOG_BUKA = 0,
        PG_DIALOG_SIMPAN = 1
} pg_dialog_tipe_t;

/* Tampilkan dialog buka/simpan file.
 * Return path yang dipilih user (malloc'd, caller free).
 * NULL jika dibatalkan atau gagal (cek pg_galat_terakhir()).
 *
 * filter: extension tanpa dot, dipisah koma. Contoh: "png,jpg,bmp".
 *         NULL untuk semua file.
 * judul: judul dialog. NULL = default platform. */
char *pg_dialog_file(pg_layar_t *layar, pg_dialog_tipe_t tipe,
                      const char *judul, const char *filter);

#ifdef __cplusplus
}
#endif

#endif /* PIGURA_DIALOG_FILE_H */
