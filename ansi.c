/*******************************************************************************
 * ANSI.C - Implementasi ANSI Escape Sequence Parser
 *
 * Modul ini berisi implementasi parser untuk ANSI escape sequences
 * dan manajemen buffer terminal.
 *
 * Copyright (c) 2024 Pigura OS Project
 ******************************************************************************/

#include "pigura.h"

/*******************************************************************************
 * WARNA VGA 16 WARNA
 ******************************************************************************/

static const WarnaRGB warna_vga[16] = {
    {0, 0, 0},         /* 0: Hitam */
    {170, 0, 0},       /* 1: Merah */
    {0, 170, 0},       /* 2: Hijau */
    {170, 85, 0},      /* 3: Coklat */
    {0, 0, 170},       /* 4: Biru */
    {170, 0, 170},     /* 5: Magenta */
    {0, 170, 170},     /* 6: Cyan */
    {170, 170, 170},   /* 7: Abu-abu terang */
    {85, 85, 85},      /* 8: Abu-abu gelap */
    {255, 85, 85},     /* 9: Merah terang */
    {85, 255, 85},     /* 10: Hijau terang */
    {255, 255, 85},    /* 11: Kuning */
    {85, 85, 255},     /* 12: Biru terang */
    {255, 85, 255},    /* 13: Magenta terang */
    {85, 255, 255},    /* 14: Cyan terang */
    {255, 255, 255}    /* 15: Putih */
};

/*******************************************************************************
 * FUNGSI INTERNAL
 ******************************************************************************/

/* Inisialisasi atribut default */
void ansi_init_attr(AtributKarakter *attr) {
    if (!attr) return;
    
    attr->tebal = 0;
    attr->samar = 0;
    attr->miring = 0;
    attr->garis_bawah = 0;
    attr->berkedip = 0;
    attr->terbalik = 0;
    attr->sembunyi = 0;
    attr->coret = 0;
    attr->tipe_latar = 0;
    attr->tipe_blkg = 0;
    attr->warna_latar = 7;
    attr->warna_blkg = 0;
    attr->rgb_latar = warna_vga[7];
    attr->rgb_blkg = warna_vga[0];
}

/* Geser baris ke atas */
static void ansi_scroll_atas(BufferTerminal *term, int baris) {
    int i, j;
    SelTerminal *temp;
    
    for (i = 0; i < baris; i++) {
        /* Simpan baris pertama */
        temp = term->sel[term->scroll_atas];
        
        /* Geser semua baris ke atas */
        for (j = term->scroll_atas; j < term->scroll_bawah; j++) {
            term->sel[j] = term->sel[j + 1];
        }
        
        /* Pindahkan baris pertama ke terakhir dan bersihkan */
        term->sel[term->scroll_bawah] = temp;
        for (j = 0; j < term->kolom; j++) {
            term->sel[term->scroll_bawah][j].ch = ' ';
            ansi_init_attr(&term->sel[term->scroll_bawah][j].attr);
        }
    }
}

/* Geser baris ke bawah */
static void ansi_scroll_bawah(BufferTerminal *term, int baris) {
    int i, j;
    SelTerminal *temp;
    
    for (i = 0; i < baris; i++) {
        /* Simpan baris terakhir */
        temp = term->sel[term->scroll_bawah];
        
        /* Geser semua baris ke bawah */
        for (j = term->scroll_bawah; j > term->scroll_atas; j--) {
            term->sel[j] = term->sel[j - 1];
        }
        
        /* Pindahkan baris terakhir ke pertama dan bersihkan */
        term->sel[term->scroll_atas] = temp;
        for (j = 0; j < term->kolom; j++) {
            term->sel[term->scroll_atas][j].ch = ' ';
            ansi_init_attr(&term->sel[term->scroll_atas][j].attr);
        }
    }
}

/* Hapus area */
static void ansi_hapus_area(BufferTerminal *term, int awal_baris, int awal_kolom,
    int akhir_baris, int akhir_kolom) {
    int i, j;
    
    for (i = awal_baris; i <= akhir_baris; i++) {
        for (j = awal_kolom; j <= akhir_kolom; j++) {
            if (i >= 0 && i < term->baris && j >= 0 && j < term->kolom) {
                term->sel[i][j].ch = ' ';
                ansi_init_attr(&term->sel[i][j].attr);
            }
        }
    }
}

/* Pindahkan kursor */
static void ansi_pindah_kursor(BufferTerminal *term, int baris, int kolom) {
    if (term->mode_origin) {
        baris += term->scroll_atas;
    }
    
    if (baris >= 0 && baris < term->baris) {
        term->kursor_baris = baris;
    }
    if (kolom >= 0 && kolom < term->kolom) {
        term->kursor_kolom = kolom;
    }
}

/* Set warna SGR */
static void ansi_set_sgr(BufferTerminal *term, ParserANSI *parser, int param) {
    int i;
    
    switch (param) {
        case 0: /* Reset semua atribut */
            ansi_init_attr(&term->attr);
            break;
        case 1: /* Tebal */
            term->attr.tebal = 1;
            term->attr.samar = 0;
            break;
        case 2: /* Samar */
            term->attr.samar = 1;
            term->attr.tebal = 0;
            break;
        case 3: /* Miring */
            term->attr.miring = 1;
            break;
        case 4: /* Garis bawah */
            term->attr.garis_bawah = 1;
            break;
        case 5: /* Berkedip lambat */
        case 6: /* Berkedip cepat */
            term->attr.berkedip = 1;
            break;
        case 7: /* Video terbalik */
            term->attr.terbalik = 1;
            break;
        case 8: /* Tersembunyi */
            term->attr.sembunyi = 1;
            break;
        case 9: /* Coret */
            term->attr.coret = 1;
            break;
        case 22: /* Intensitas normal */
            term->attr.tebal = 0;
            term->attr.samar = 0;
            break;
        case 23: /* Tidak miring */
            term->attr.miring = 0;
            break;
        case 24: /* Tidak bergaris bawah */
            term->attr.garis_bawah = 0;
            break;
        case 25: /* Tidak berkedip */
            term->attr.berkedip = 0;
            break;
        case 27: /* Tidak terbalik */
            term->attr.terbalik = 0;
            break;
        case 28: /* Tidak tersembunyi */
            term->attr.sembunyi = 0;
            break;
        case 29: /* Tidak dicoret */
            term->attr.coret = 0;
            break;
        case 30: case 31: case 32: case 33: case 34: case 35: case 36: case 37:
            /* Warna latar 16 warna */
            term->attr.tipe_latar = 1;
            term->attr.warna_latar = param - 30;
            break;
        case 38: /* Warna latar 256/RGB */
            if (parser->param_jumlah > 1) {
                if (parser->params[1] == 5 && parser->param_jumlah > 2) {
                    /* 256 warna */
                    term->attr.tipe_latar = 2;
                    term->attr.warna_latar = parser->params[2];
                } else if (parser->params[1] == 2 && parser->param_jumlah > 4) {
                    /* RGB */
                    term->attr.tipe_latar = 3;
                    term->attr.rgb_latar.r = parser->params[2];
                    term->attr.rgb_latar.g = parser->params[3];
                    term->attr.rgb_latar.b = parser->params[4];
                }
            }
            break;
        case 39: /* Latar default */
            term->attr.tipe_latar = 0;
            term->attr.warna_latar = 7;
            term->attr.rgb_latar = warna_vga[7];
            break;
        case 40: case 41: case 42: case 43: case 44: case 45: case 46: case 47:
            /* Warna background 16 warna */
            term->attr.tipe_blkg = 1;
            term->attr.warna_blkg = param - 40;
            break;
        case 48: /* Background 256/RGB */
            if (parser->param_jumlah > 1) {
                if (parser->params[1] == 5 && parser->param_jumlah > 2) {
                    term->attr.tipe_blkg = 2;
                    term->attr.warna_blkg = parser->params[2];
                } else if (parser->params[1] == 2 && parser->param_jumlah > 4) {
                    term->attr.tipe_blkg = 3;
                    term->attr.rgb_blkg.r = parser->params[2];
                    term->attr.rgb_blkg.g = parser->params[3];
                    term->attr.rgb_blkg.b = parser->params[4];
                }
            }
            break;
        case 49: /* Background default */
            term->attr.tipe_blkg = 0;
            term->attr.warna_blkg = 0;
            term->attr.rgb_blkg = warna_vga[0];
            break;
        case 90: case 91: case 92: case 93: case 94: case 95: case 96: case 97:
            /* Latar terang 16 warna */
            term->attr.tipe_latar = 1;
            term->attr.warna_latar = param - 90 + 8;
            break;
        case 100: case 101: case 102: case 103: case 104: case 105: case 106: case 107:
            /* Background terang 16 warna */
            term->attr.tipe_blkg = 1;
            term->attr.warna_blkg = param - 100 + 8;
            break;
    }
}

/* Eksekusi sequence ANSI */
static void ansi_eksekusi(BufferTerminal *term, ParserANSI *parser, char final) {
    int param1, param2;
    int i;
    
    /* Default parameter */
    param1 = (parser->param_jumlah > 0) ? parser->params[0] : 0;
    param2 = (parser->param_jumlah > 1) ? parser->params[1] : 0;
    
    /* Jika param1 = 0, gunakan 1 untuk beberapa perintah */
    
    switch (final) {
        case 'A': /* Kursor naik */
            ansi_pindah_kursor(term, term->kursor_baris - (param1 ? param1 : 1),
                term->kursor_kolom);
            break;
        case 'B': /* Kursor turun */
            ansi_pindah_kursor(term, term->kursor_baris + (param1 ? param1 : 1),
                term->kursor_kolom);
            break;
        case 'C': /* Kursor kanan */
            ansi_pindah_kursor(term, term->kursor_baris,
                term->kursor_kolom + (param1 ? param1 : 1));
            break;
        case 'D': /* Kursor kiri */
            ansi_pindah_kursor(term, term->kursor_baris,
                term->kursor_kolom - (param1 ? param1 : 1));
            break;
        case 'E': /* Kursor ke baris berikutnya */
            ansi_pindah_kursor(term, term->kursor_baris + (param1 ? param1 : 1), 0);
            break;
        case 'F': /* Kursor ke baris sebelumnya */
            ansi_pindah_kursor(term, term->kursor_baris - (param1 ? param1 : 1), 0);
            break;
        case 'G': /* Kursor ke kolom absolut */
            ansi_pindah_kursor(term, term->kursor_baris, param1 ? param1 - 1 : 0);
            break;
        case 'H': /* Kursor ke posisi */
        case 'f':
            ansi_pindah_kursor(term, param1 ? param1 - 1 : 0, param2 ? param2 - 1 : 0);
            break;
        case 'J': /* Hapus layar */
            switch (param1) {
                case 0: /* Dari kursor ke akhir */
                    ansi_hapus_area(term, term->kursor_baris, term->kursor_kolom,
                        term->baris - 1, term->kolom - 1);
                    break;
                case 1: /* Dari awal ke kursor */
                    ansi_hapus_area(term, 0, 0, term->kursor_baris, term->kursor_kolom);
                    break;
                case 2: /* Seluruh layar */
                    ansi_hapus_area(term, 0, 0, term->baris - 1, term->kolom - 1);
                    break;
            }
            break;
        case 'K': /* Hapus baris */
            switch (param1) {
                case 0: /* Dari kursor ke akhir baris */
                    ansi_hapus_area(term, term->kursor_baris, term->kursor_kolom,
                        term->kursor_baris, term->kolom - 1);
                    break;
                case 1: /* Dari awal baris ke kursor */
                    ansi_hapus_area(term, term->kursor_baris, 0,
                        term->kursor_baris, term->kursor_kolom);
                    break;
                case 2: /* Seluruh baris */
                    ansi_hapus_area(term, term->kursor_baris, 0,
                        term->kursor_baris, term->kolom - 1);
                    break;
            }
            break;
        case 'L': /* Sisipkan baris */
            if (param1 == 0) param1 = 1;
            for (i = 0; i < param1; i++) {
                ansi_scroll_bawah(term, 1);
            }
            break;
        case 'M': /* Hapus baris */
            if (param1 == 0) param1 = 1;
            for (i = 0; i < param1; i++) {
                ansi_scroll_atas(term, 1);
            }
            break;
        case 'P': /* Hapus karakter */
            /* Implementasi sederhana */
            break;
        case 'S': /* Scroll atas */
            ansi_scroll_atas(term, param1 ? param1 : 1);
            break;
        case 'T': /* Scroll bawah */
            ansi_scroll_bawah(term, param1 ? param1 : 1);
            break;
        case 'd': /* Kursor ke baris absolut */
            ansi_pindah_kursor(term, param1 ? param1 - 1 : 0, term->kursor_kolom);
            break;
        case 'h': /* Set mode */
            if (parser->privasi) {
                switch (param1) {
                    case 25: /* Tampilkan kursor */
                        term->kursor_terlihat = 1;
                        break;
                    case 1000: /* Mode tetikus normal */
                        term->mode_tetikus = 2;
                        break;
                    case 1002: /* Mode tetikus button-event */
                        term->mode_tetikus = 3;
                        break;
                    case 1003: /* Mode tetikus any-event */
                        term->mode_tetikus = 4;
                        break;
                    case 1006: /* SGR encoding */
                        term->encoding_tetikus = 2;
                        break;
                }
            }
            break;
        case 'l': /* Reset mode */
            if (parser->privasi) {
                switch (param1) {
                    case 25: /* Sembunyikan kursor */
                        term->kursor_terlihat = 0;
                        break;
                    case 1000: case 1002: case 1003:
                        term->mode_tetikus = 0;
                        break;
                    case 1006:
                        term->encoding_tetikus = 0;
                        break;
                }
            }
            break;
        case 'm': /* Set Graphics Rendition */
            if (parser->param_jumlah == 0) {
                ansi_set_sgr(term, parser, 0);
            } else {
                for (i = 0; i < parser->param_jumlah; i++) {
                    ansi_set_sgr(term, parser, parser->params[i]);
                }
            }
            break;
        case 'r': /* Set scrolling region */
            term->scroll_atas = param1 ? param1 - 1 : 0;
            term->scroll_bawah = param2 ? param2 - 1 : term->baris - 1;
            if (term->scroll_atas < 0) term->scroll_atas = 0;
            if (term->scroll_bawah >= term->baris) {
                term->scroll_bawah = term->baris - 1;
            }
            break;
        case 's': /* Simpan posisi kursor */
            term->kursor_simpan_baris = term->kursor_baris;
            term->kursor_simpan_kolom = term->kursor_kolom;
            break;
        case 'u': /* Pulihkan posisi kursor */
            term->kursor_baris = term->kursor_simpan_baris;
            term->kursor_kolom = term->kursor_simpan_kolom;
            break;
    }
}

/*******************************************************************************
 * FUNGSI PUBLIK
 ******************************************************************************/

/* Inisialisasi buffer terminal */
int ansi_init_terminal(BufferTerminal *term, int baris, int kolom) {
    int i, j;
    
    if (!term || baris <= 0 || kolom <= 0) {
        return 0;
    }
    
    /* Alokasi memori untuk baris */
    term->sel = (SelTerminal **)malloc(baris * sizeof(SelTerminal *));
    if (!term->sel) {
        return 0;
    }
    
    /* Alokasi memori untuk setiap baris */
    for (i = 0; i < baris; i++) {
        term->sel[i] = (SelTerminal *)malloc(kolom * sizeof(SelTerminal));
        if (!term->sel[i]) {
            /* Cleanup jika gagal */
            for (j = 0; j < i; j++) {
                free(term->sel[j]);
            }
            free(term->sel);
            return 0;
        }
        
        /* Inisialisasi sel */
        for (j = 0; j < kolom; j++) {
            term->sel[i][j].ch = ' ';
            ansi_init_attr(&term->sel[i][j].attr);
        }
    }
    
    /* Alokasi tab stops */
    term->tab_stop = (int *)malloc(kolom * sizeof(int));
    if (!term->tab_stop) {
        for (i = 0; i < baris; i++) {
            free(term->sel[i]);
        }
        free(term->sel);
        return 0;
    }
    
    /* Inisialisasi tab stops setiap 8 kolom */
    for (i = 0; i < kolom; i++) {
        term->tab_stop[i] = (i % 8 == 0) ? 1 : 0;
    }
    
    /* Inisialisasi state */
    term->baris = baris;
    term->kolom = kolom;
    term->kursor_baris = 0;
    term->kursor_kolom = 0;
    term->kursor_terlihat = 1;
    term->kursor_simpan_baris = 0;
    term->kursor_simpan_kolom = 0;
    ansi_init_attr(&term->attr);
    ansi_init_attr(&term->attr_def);
    term->scroll_atas = 0;
    term->scroll_bawah = baris - 1;
    term->mode_origin = 0;
    term->auto_wrap = 1;
    term->mode_sisip = 0;
    term->mode_layar = 0;
    term->layar_simpan = NULL;
    term->mode_tetikus = 0;
    term->encoding_tetikus = 0;
    term->bracketed_paste = 0;
    
    return 1;
}

/* Bebaskan buffer terminal */
void ansi_bebas_terminal(BufferTerminal *term) {
    int i;
    
    if (!term) return;
    
    if (term->sel) {
        for (i = 0; i < term->baris; i++) {
            if (term->sel[i]) {
                free(term->sel[i]);
            }
        }
        free(term->sel);
    }
    
    if (term->tab_stop) {
        free(term->tab_stop);
    }
    
    if (term->layar_simpan) {
        for (i = 0; i < term->baris; i++) {
            if (term->layar_simpan[i]) {
                free(term->layar_simpan[i]);
            }
        }
        free(term->layar_simpan);
    }
}

/* Inisialisasi parser ANSI */
void ansi_init_parser(ParserANSI *parser) {
    if (!parser) return;
    
    parser->state = ANSI_STATE_NORMAL;
    parser->seq_pos = 0;
    parser->param_jumlah = 0;
    parser->privasi = 0;
    parser->intermediate = 0;
    memset(parser->params, 0, sizeof(parser->params));
    memset(parser->seq_buf, 0, sizeof(parser->seq_buf));
}

/* Proses karakter untuk parser */
void ansi_proses_karakter(BufferTerminal *term, ParserANSI *parser, char ch) {
    int i, next_tab;
    
    switch (parser->state) {
        case ANSI_STATE_NORMAL:
            if (ch == '\033') {
                parser->state = ANSI_STATE_ESCAPE;
                parser->seq_pos = 0;
                parser->param_jumlah = 0;
                parser->privasi = 0;
                parser->intermediate = 0;
                memset(parser->params, 0, sizeof(parser->params));
            } else {
                /* Handle karakter normal */
                if (ch == '\n') {
                    term->kursor_baris++;
                    term->kursor_kolom = 0;
                    if (term->kursor_baris > term->scroll_bawah) {
                        ansi_scroll_atas(term, 1);
                        term->kursor_baris = term->scroll_bawah;
                    }
                } else if (ch == '\r') {
                    term->kursor_kolom = 0;
                } else if (ch == '\t') {
                    next_tab = term->kursor_kolom + 1;
                    while (next_tab < term->kolom && !term->tab_stop[next_tab]) {
                        next_tab++;
                    }
                    term->kursor_kolom = (next_tab >= term->kolom) ? 
                        term->kolom - 1 : next_tab;
                } else if (ch == '\b') {
                    if (term->kursor_kolom > 0) {
                        term->kursor_kolom--;
                    }
                } else if (ch == '\a') {
                    /* Bell - abaikan untuk sekarang */
                } else if (ch >= 32) {
                    /* Karakter yang dapat dicetak */
                    if (term->kursor_baris >= 0 && term->kursor_baris < term->baris &&
                        term->kursor_kolom >= 0 && term->kursor_kolom < term->kolom) {
                        
                        term->sel[term->kursor_baris][term->kursor_kolom].ch = ch;
                        term->sel[term->kursor_baris][term->kursor_kolom].attr = term->attr;
                        term->kursor_kolom++;
                        
                        if (term->auto_wrap && term->kursor_kolom >= term->kolom) {
                            term->kursor_kolom = 0;
                            term->kursor_baris++;
                            if (term->kursor_baris > term->scroll_bawah) {
                                ansi_scroll_atas(term, 1);
                                term->kursor_baris = term->scroll_bawah;
                            }
                        }
                    }
                }
            }
            break;
            
        case ANSI_STATE_ESCAPE:
            if (ch == '[') {
                parser->state = ANSI_STATE_CSI;
                parser->seq_pos = 0;
            } else if (ch == ']') {
                parser->state = ANSI_STATE_OSC;
                parser->seq_pos = 0;
            } else if (ch == '7') {
                /* Simpan kursor */
                term->kursor_simpan_baris = term->kursor_baris;
                term->kursor_simpan_kolom = term->kursor_kolom;
                parser->state = ANSI_STATE_NORMAL;
            } else if (ch == '8') {
                /* Pulihkan kursor */
                term->kursor_baris = term->kursor_simpan_baris;
                term->kursor_kolom = term->kursor_simpan_kolom;
                parser->state = ANSI_STATE_NORMAL;
            } else if (ch == 'D') {
                /* Index */
                ansi_scroll_atas(term, 1);
                parser->state = ANSI_STATE_NORMAL;
            } else if (ch == 'M') {
                /* Reverse index */
                ansi_scroll_bawah(term, 1);
                parser->state = ANSI_STATE_NORMAL;
            } else if (ch == 'E') {
                /* Next line */
                term->kursor_baris++;
                term->kursor_kolom = 0;
                if (term->kursor_baris > term->scroll_bawah) {
                    ansi_scroll_atas(term, 1);
                    term->kursor_baris = term->scroll_bawah;
                }
                parser->state = ANSI_STATE_NORMAL;
            } else if (ch == 'H') {
                /* Tab set */
                if (term->kursor_kolom < term->kolom) {
                    term->tab_stop[term->kursor_kolom] = 1;
                }
                parser->state = ANSI_STATE_NORMAL;
            } else {
                parser->state = ANSI_STATE_NORMAL;
            }
            break;
            
        case ANSI_STATE_CSI:
            if (ch >= '0' && ch <= '9') {
                /* Parameter numerik */
                parser->seq_buf[parser->seq_pos++] = ch;
                if (parser->seq_pos >= ANSI_SEQ_MAKS - 1) {
                    parser->state = ANSI_STATE_NORMAL;
                }
            } else if (ch == ';') {
                /* Separator parameter */
                if (parser->seq_pos > 0) {
                    parser->seq_buf[parser->seq_pos] = '\0';
                    if (parser->param_jumlah < ANSI_PARAM_MAKS) {
                        parser->params[parser->param_jumlah++] = 
                            atoi(parser->seq_buf);
                    }
                    parser->seq_pos = 0;
                }
            } else if (ch == '?') {
                /* Private mode */
                parser->privasi = 1;
            } else if (ch >= '@' && ch <= '~') {
                /* Final character */
                if (parser->seq_pos > 0) {
                    parser->seq_buf[parser->seq_pos] = '\0';
                    if (parser->param_jumlah < ANSI_PARAM_MAKS) {
                        parser->params[parser->param_jumlah++] = 
                            atoi(parser->seq_buf);
                    }
                }
                ansi_eksekusi(term, parser, ch);
                parser->state = ANSI_STATE_NORMAL;
            } else {
                parser->state = ANSI_STATE_NORMAL;
            }
            break;
            
        case ANSI_STATE_OSC:
            if (ch == '\007' || ch == '\033') {
                parser->state = ANSI_STATE_NORMAL;
            }
            break;
            
        default:
            parser->state = ANSI_STATE_NORMAL;
            break;
    }
}

/* Proses string untuk parser */
void ansi_proses_string(BufferTerminal *term, ParserANSI *parser, const char *str) {
    if (!str) return;
    
    while (*str) {
        ansi_proses_karakter(term, parser, *str);
        str++;
    }
}

/* Dapatkan warna RGB dari atribut */
WarnaRGB ansi_warna(AtributKarakter *attr, int latar) {
    WarnaRGB warna;
    
    if (!attr) {
        warna.r = 255; warna.g = 255; warna.b = 255;
        return warna;
    }
    
    if (latar) {
        switch (attr->tipe_latar) {
            case 0: /* Default */
                return attr->rgb_latar;
            case 1: /* 16 warna */
                if (attr->warna_latar < 16) {
                    return warna_vga[attr->warna_latar];
                }
                break;
            case 2: /* 256 warna - sederhana */
            case 3: /* RGB */
                return attr->rgb_latar;
        }
    } else {
        switch (attr->tipe_blkg) {
            case 0: /* Default */
                return attr->rgb_blkg;
            case 1: /* 16 warna */
                if (attr->warna_blkg < 16) {
                    return warna_vga[attr->warna_blkg];
                }
                break;
            case 2: /* 256 warna */
            case 3: /* RGB */
                return attr->rgb_blkg;
        }
    }
    
    warna.r = 0; warna.g = 0; warna.b = 0;
    return warna;
}

/* Render terminal ke framebuffer */
void ansi_render(BufferTerminal *term, InfoFramebuffer *fb,
    void (*gambar_karakter)(InfoFramebuffer*, int, int, unsigned int, WarnaRGB, WarnaRGB)) {
    
    int baris, kolom;
    WarnaRGB latar, blkg;
    char ch;
    
    if (!term || !fb || !gambar_karakter) return;
    
    for (baris = 0; baris < term->baris; baris++) {
        for (kolom = 0; kolom < term->kolom; kolom++) {
            ch = term->sel[baris][kolom].ch;
            
            if (term->sel[baris][kolom].attr.terbalik) {
                latar = ansi_warna(&term->sel[baris][kolom].attr, 0);
                blkg = ansi_warna(&term->sel[baris][kolom].attr, 1);
            } else {
                latar = ansi_warna(&term->sel[baris][kolom].attr, 1);
                blkg = ansi_warna(&term->sel[baris][kolom].attr, 0);
            }
            
            gambar_karakter(fb, kolom * HURUF_LEBAR, baris * HURUF_TINGGI,
                (unsigned char)ch, latar, blkg);
        }
    }
}
