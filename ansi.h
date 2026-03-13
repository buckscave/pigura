#ifndef ANSI_ESCAPE_H
#define ANSI_ESCAPE_H

#include <stddef.h> /* untuk size_t */

/* Konstanta untuk ANSI escape sequences */
#define ANSI_MAX_PARAMS 32
#define ANSI_MAX_SEQ_LEN 128
#define ANSI_MAX_SAVED_POSITIONS 10
#define ANSI_MAX_TAB_STOPS 80

/* Struktur warna RGB */
typedef struct {
    unsigned char r, g, b;
} RGBColor;

/* Struktur informasi framebuffer */
typedef struct {
    void *ptr;           /* Pointer ke memori framebuffer */
    int width;           /* Lebar layar dalam piksel */
    int height;          /* Tinggi layar dalam piksel */
    int bpp;             /* Bit per piksel (16, 24, atau 32) */
    int line_length;     /* Panjang satu baris dalam byte */
} FBInfo;

/* Struktur atribut karakter */
typedef struct {
    unsigned char bold:1;        /* Bold/bright */
    unsigned char faint:1;       /* Faint/dim */
    unsigned char italic:1;      /* Italic */
    unsigned char underline:1;   /* Underline */
    unsigned char blink:1;       /* Blink */
    unsigned char reverse:1;     /* Reverse video */
    unsigned char conceal:1;     /* Concealed/hidden */
    unsigned char crossed_out:1;  /* Crossed out */
    unsigned char fg_type:2;     /* 0=default, 1=16-color, 2=256-color, 3=RGB */
    unsigned char bg_type:2;     /* 0=default, 1=16-color, 2=256-color, 3=RGB */
    unsigned char fg_color;       /* Warna foreground (tergantung tipe) */
    unsigned char bg_color;       /* Warna background (tergantung tipe) */
    RGBColor fg_rgb;             /* RGB foreground */
    RGBColor bg_rgb;             /* RGB background */
} CharAttr;

/* Struktur sel terminal */
typedef struct {
    char ch;          /* Karakter */
    CharAttr attr;    /* Atribut karakter */
} TerminalCell;

/* Struktur buffer terminal */
typedef struct {
    TerminalCell **cells;    /* Array 2D dari sel terminal */
    int rows;               /* Jumlah baris */
    int cols;               /* Jumlah kolom */
    int cursor_row;         /* Posisi kursor (baris) */
    int cursor_col;         /* Posisi kursor (kolom) */
    int cursor_visible;     /* Visibilitas kursor */
    int saved_cursor_row;   /* Posisi kursor tersimpan */
    int saved_cursor_col;   /* Posisi kursor tersimpan */
    CharAttr current_attr;  /* Atribut saat ini */
    CharAttr default_attr;  /* Atribut default */
    int scroll_top;         /* Area scroll atas */
    int scroll_bottom;      /* Area scroll bawah */
    int *tab_stops;        /* Posisi tab stops */
    int origin_mode;        /* Origin mode (DECOM) */
    int auto_wrap;          /* Auto wrap mode (DECAWM) */
    int insert_mode;        /* Insert mode (IRM) */
    int screen_mode;        /* 0=normal, 1=alternate */
    TerminalCell **saved_screen; /* Layar tersimpan */
    int mouse_mode;         /* 0=off, 1=x10, 2=normal, 3=button-event, 4=any-event */
    int mouse_encoding;     /* 0=default, 1=UTF-8, 2=SGR */
    int bracketed_paste;    /* Bracketed paste mode */
} TerminalBuffer;

/* Struktur parser ANSI */
typedef struct {
    int state;              /* State parser */
    char seq_buf[ANSI_MAX_SEQ_LEN]; /* Buffer sequence */
    int seq_pos;            /* Posisi dalam buffer */
    int params[ANSI_MAX_PARAMS];    /* Parameter */
    int param_count;        /* Jumlah parameter */
    int private;            /* Private sequence flag */
    int intermediate;       /* Intermediate character */
} ANSIParser;

/* Warna default VGA 16 warna */
static RGBColor vga_colors[16] = {
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

/* Warna 256 warna (6x6x6 cube + 24 grays) */
static RGBColor color256[256] = {
    /* 0-15: Standard colors */
    {0, 0, 0}, {170, 0, 0}, {0, 170, 0}, {170, 85, 0},
    {0, 0, 170}, {170, 0, 170}, {0, 170, 170}, {170, 170, 170},
    {85, 85, 85}, {255, 85, 85}, {85, 255, 85}, {255, 255, 85},
    {85, 85, 255}, {255, 85, 255}, {85, 255, 255}, {255, 255, 255},
    
    /* 16-231: 6x6x6 color cube */
    /* Red component */
    {0, 0, 0}, {0, 0, 95}, {0, 0, 135}, {0, 0, 175}, {0, 0, 215}, {0, 0, 255},
    {0, 95, 0}, {0, 95, 95}, {0, 95, 135}, {0, 95, 175}, {0, 95, 215}, {0, 95, 255},
    {0, 135, 0}, {0, 135, 95}, {0, 135, 135}, {0, 135, 175}, {0, 135, 215}, {0, 135, 255},
    {0, 175, 0}, {0, 175, 95}, {0, 175, 135}, {0, 175, 175}, {0, 175, 215}, {0, 175, 255},
    {0, 215, 0}, {0, 215, 95}, {0, 215, 135}, {0, 215, 175}, {0, 215, 215}, {0, 215, 255},
    {0, 255, 0}, {0, 255, 95}, {0, 255, 135}, {0, 255, 175}, {0, 255, 215}, {0, 255, 255},
    {95, 0, 0}, {95, 0, 95}, {95, 0, 135}, {95, 0, 175}, {95, 0, 215}, {95, 0, 255},
    {95, 95, 0}, {95, 95, 95}, {95, 95, 135}, {95, 95, 175}, {95, 95, 215}, {95, 95, 255},
    {95, 135, 0}, {95, 135, 95}, {95, 135, 135}, {95, 135, 175}, {95, 135, 215}, {95, 135, 255},
    {95, 175, 0}, {95, 175, 95}, {95, 175, 135}, {95, 175, 175}, {95, 175, 215}, {95, 175, 255},
    {95, 215, 0}, {95, 215, 95}, {95, 215, 135}, {95, 215, 175}, {95, 215, 215}, {95, 215, 255},
    {95, 255, 0}, {95, 255, 95}, {95, 255, 135}, {95, 255, 175}, {95, 255, 215}, {95, 255, 255},
    {135, 0, 0}, {135, 0, 95}, {135, 0, 135}, {135, 0, 175}, {135, 0, 215}, {135, 0, 255},
    {135, 95, 0}, {135, 95, 95}, {135, 95, 135}, {135, 95, 175}, {135, 95, 215}, {135, 95, 255},
    {135, 135, 0}, {135, 135, 95}, {135, 135, 135}, {135, 135, 175}, {135, 135, 215}, {135, 135, 255},
    {135, 175, 0}, {135, 175, 95}, {135, 175, 135}, {135, 175, 175}, {135, 175, 215}, {135, 175, 255},
    {135, 215, 0}, {135, 215, 95}, {135, 215, 135}, {135, 215, 175}, {135, 215, 215}, {135, 215, 255},
    {135, 255, 0}, {135, 255, 95}, {135, 255, 135}, {135, 255, 175}, {135, 255, 215}, {135, 255, 255},
    {175, 0, 0}, {175, 0, 95}, {175, 0, 135}, {175, 0, 175}, {175, 0, 215}, {175, 0, 255},
    {175, 95, 0}, {175, 95, 95}, {175, 95, 135}, {175, 95, 175}, {175, 95, 215}, {175, 95, 255},
    {175, 135, 0}, {175, 135, 95}, {175, 135, 135}, {175, 135, 175}, {175, 135, 215}, {175, 135, 255},
    {175, 175, 0}, {175, 175, 95}, {175, 175, 135}, {175, 175, 175}, {175, 175, 215}, {175, 175, 255},
    {175, 215, 0}, {175, 215, 95}, {175, 215, 135}, {175, 215, 175}, {175, 215, 215}, {175, 215, 255},
    {175, 255, 0}, {175, 255, 95}, {175, 255, 135}, {175, 255, 175}, {175, 255, 215}, {175, 255, 255},
    {215, 0, 0}, {215, 0, 95}, {215, 0, 135}, {215, 0, 175}, {215, 0, 215}, {215, 0, 255},
    {215, 95, 0}, {215, 95, 95}, {215, 95, 135}, {215, 95, 175}, {215, 95, 215}, {215, 95, 255},
    {215, 135, 0}, {215, 135, 95}, {215, 135, 135}, {215, 135, 175}, {215, 135, 215}, {215, 135, 255},
    {215, 175, 0}, {215, 175, 95}, {215, 175, 135}, {215, 175, 175}, {215, 175, 215}, {215, 175, 255},
    {215, 215, 0}, {215, 215, 95}, {215, 215, 135}, {215, 215, 175}, {215, 215, 215}, {215, 215, 255},
    {215, 255, 0}, {215, 255, 95}, {215, 255, 135}, {215, 255, 175}, {215, 255, 215}, {215, 255, 255},
    {255, 0, 0}, {255, 0, 95}, {255, 0, 135}, {255, 0, 175}, {255, 0, 215}, {255, 0, 255},
    {255, 95, 0}, {255, 95, 95}, {255, 95, 135}, {255, 95, 175}, {255, 95, 215}, {255, 95, 255},
    {255, 135, 0}, {255, 135, 95}, {255, 135, 135}, {255, 135, 175}, {255, 135, 215}, {255, 135, 255},
    {255, 175, 0}, {255, 175, 95}, {255, 175, 135}, {255, 175, 175}, {255, 175, 215}, {255, 175, 255},
    {255, 215, 0}, {255, 215, 95}, {255, 215, 135}, {255, 215, 175}, {255, 215, 215}, {255, 215, 255},
    {255, 255, 0}, {255, 255, 95}, {255, 255, 135}, {255, 255, 175}, {255, 255, 215}, {255, 255, 255},
    
    /* 232-255: Grayscale */
    {8, 8, 8}, {18, 18, 18}, {28, 28, 28}, {38, 38, 38},
    {48, 48, 48}, {58, 58, 58}, {68, 68, 68}, {78, 78, 78},
    {88, 88, 88}, {98, 98, 98}, {108, 108, 108}, {118, 118, 118},
    {128, 128, 128}, {138, 138, 138}, {148, 148, 148}, {158, 158, 158},
    {168, 168, 168}, {178, 178, 178}, {188, 188, 188}, {198, 198, 198},
    {208, 208, 208}, {218, 218, 218}, {228, 228, 228}, {238, 238, 238}
};

/* State parser */
#define ANSI_STATE_NORMAL     0
#define ANSI_STATE_ESCAPE     1
#define ANSI_STATE_CSI        2
#define ANSI_STATE_OSC        3
#define ANSI_STATE_STRING     4

/* Inisialisasi atribut default */
static void ansi_init_attr(CharAttr *attr) {
    attr->bold = 0;
    attr->faint = 0;
    attr->italic = 0;
    attr->underline = 0;
    attr->blink = 0;
    attr->reverse = 0;
    attr->conceal = 0;
    attr->crossed_out = 0;
    attr->fg_type = 0; /* Default */
    attr->bg_type = 0; /* Default */
    attr->fg_color = 7; /* Putih */
    attr->bg_color = 0; /* Hitam */
    attr->fg_rgb = vga_colors[7];
    attr->bg_rgb = vga_colors[0];
}

/* Inisialisasi terminal buffer */
static int ansi_init_terminal(TerminalBuffer *term, int rows, int cols) {
    int i, j;
    
    /* Alokasi memori untuk baris */
    term->cells = (TerminalCell **)malloc(rows * sizeof(TerminalCell *));
    if (!term->cells) return 0;
    
    /* Alokasi memori untuk setiap baris */
    for (i = 0; i < rows; i++) {
        term->cells[i] = (TerminalCell *)malloc(cols * sizeof(TerminalCell));
        if (!term->cells[i]) {
            /* Cleanup jika gagal */
            for (int k = 0; k < i; k++) free(term->cells[k]);
            free(term->cells);
            return 0;
        }
        
        /* Inisialisasi sel */
        for (j = 0; j < cols; j++) {
            term->cells[i][j].ch = ' ';
            ansi_init_attr(&term->cells[i][j].attr);
        }
    }
    
    /* Alokasi tab stops */
    term->tab_stops = (int *)malloc(cols * sizeof(int));
    if (!term->tab_stops) {
        for (i = 0; i < rows; i++) free(term->cells[i]);
        free(term->cells);
        return 0;
    }
    
    /* Inisialisasi tab stops setiap 8 kolom */
    for (i = 0; i < cols; i++) {
        term->tab_stops[i] = (i % 8 == 0) ? 1 : 0;
    }
    
    /* Inisialisasi state */
    term->rows = rows;
    term->cols = cols;
    term->cursor_row = 0;
    term->cursor_col = 0;
    term->cursor_visible = 1;
    term->saved_cursor_row = 0;
    term->saved_cursor_col = 0;
    ansi_init_attr(&term->current_attr);
    ansi_init_attr(&term->default_attr);
    term->scroll_top = 0;
    term->scroll_bottom = rows - 1;
    term->origin_mode = 0;
    term->auto_wrap = 1;
    term->insert_mode = 0;
    term->screen_mode = 0;
    term->saved_screen = NULL;
    term->mouse_mode = 0;
    term->mouse_encoding = 0;
    term->bracketed_paste = 0;
    
    return 1;
}

/* Bebas memori terminal buffer */
static void ansi_free_terminal(TerminalBuffer *term) {
    if (term->cells) {
        for (int i = 0; i < term->rows; i++) {
            if (term->cells[i]) free(term->cells[i]);
        }
        free(term->cells);
    }
    
    if (term->tab_stops) {
        free(term->tab_stops);
    }
    
    if (term->saved_screen) {
        for (int i = 0; i < term->rows; i++) {
            if (term->saved_screen[i]) free(term->saved_screen[i]);
        }
        free(term->saved_screen);
    }
}

/* Inisialisasi parser ANSI */
static void ansi_init_parser(ANSIParser *parser) {
    parser->state = ANSI_STATE_NORMAL;
    parser->seq_pos = 0;
    parser->param_count = 0;
    parser->private = 0;
    parser->intermediate = 0;
    memset(parser->params, 0, sizeof(parser->params));
}

/* Reset parameter parser */
static void ansi_reset_params(ANSIParser *parser) {
    parser->param_count = 0;
    parser->private = 0;
    parser->intermediate = 0;
    memset(parser->params, 0, sizeof(parser->params));
}

/* Tambahkan parameter ke parser */
static void ansi_add_param(ANSIParser *parser, int value) {
    if (parser->param_count < ANSI_MAX_PARAMS) {
        parser->params[parser->param_count++] = value;
    }
}

/* Parsing parameter numerik */
static void ansi_parse_params(ANSIParser *parser) {
    int value = 0;
    int i;
    
    for (i = 0; i < parser->seq_pos; i++) {
        char c = parser->seq_buf[i];
        
        if (c >= '0' && c <= '9') {
            value = value * 10 + (c - '0');
        } else if (c == ';') {
            ansi_add_param(parser, value);
            value = 0;
        }
    }
    
    /* Tambahkan parameter terakhir */
    ansi_add_param(parser, value);
}

/* Geser baris ke atas */
static void ansi_scroll_up(TerminalBuffer *term, int lines) {
    int i, j;
    
    for (i = 0; i < lines; i++) {
        /* Geser semua baris ke atas di area scroll */
        for (j = term->scroll_top; j < term->scroll_bottom; j++) {
            /* Copy baris berikutnya ke baris saat ini */
            memcpy(term->cells[j], term->cells[j + 1], 
                   term->cols * sizeof(TerminalCell));
        }
        
        /* Hapus baris terakhir di area scroll */
        for (j = 0; j < term->cols; j++) {
            term->cells[term->scroll_bottom][j].ch = ' ';
            ansi_init_attr(&term->cells[term->scroll_bottom][j].attr);
        }
    }
}

/* Geser baris ke bawah */
static void ansi_scroll_down(TerminalBuffer *term, int lines) {
    int i, j;
    
    for (i = 0; i < lines; i++) {
        /* Geser semua baris ke bawah di area scroll */
        for (j = term->scroll_bottom; j > term->scroll_top; j--) {
            /* Copy baris sebelumnya ke baris saat ini */
            memcpy(term->cells[j], term->cells[j - 1], 
                   term->cols * sizeof(TerminalCell));
        }
        
        /* Hapus baris pertama di area scroll */
        for (j = 0; j < term->cols; j++) {
            term->cells[term->scroll_top][j].ch = ' ';
            ansi_init_attr(&term->cells[term->scroll_top][j].attr);
        }
    }
}

/* Hapus area layar */
static void ansi_erase_area(TerminalBuffer *term, int start_row, int start_col, 
                           int end_row, int end_col) {
    int i, j;
    
    for (i = start_row; i <= end_row; i++) {
        for (j = start_col; j <= end_col; j++) {
            if (i >= 0 && i < term->rows && j >= 0 && j < term->cols) {
                term->cells[i][j].ch = ' ';
                ansi_init_attr(&term->cells[i][j].attr);
            }
        }
    }
}

/* Sisipkan karakter */
static void ansi_insert_char(TerminalBuffer *term, char ch) {
    int i;
    
    if (term->cursor_row >= 0 && term->cursor_row < term->rows &&
        term->cursor_col >= 0 && term->cursor_col < term->cols) {
        
        /* Geser karakter ke kanat */
        for (i = term->cols - 1; i > term->cursor_col; i--) {
            term->cells[term->cursor_row][i] = term->cells[term->cursor_row][i - 1];
        }
        
        /* Sisipkan karakter baru */
        term->cells[term->cursor_row][term->cursor_col].ch = ch;
        term->cells[term->cursor_row][term->cursor_col].attr = term->current_attr;
    }
}

/* Hapus karakter */
static void ansi_delete_char(TerminalBuffer *term, int count) {
    int i, j;
    
    if (term->cursor_row >= 0 && term->cursor_row < term->rows &&
        term->cursor_col >= 0 && term->cursor_col < term->cols) {
        
        /* Geser karakter ke kiri */
        for (i = term->cursor_col; i < term->cols - count; i++) {
            term->cells[term->cursor_row][i] = term->cells[term->cursor_row][i + count];
        }
        
        /* Hapus karakter di ujung */
        for (i = term->cols - count; i < term->cols; i++) {
            term->cells[term->cursor_row][i].ch = ' ';
            ansi_init_attr(&term->cells[term->cursor_row][i].attr);
        }
    }
}

/* Pindahkan kursor */
static void ansi_move_cursor(TerminalBuffer *term, int row, int col) {
    if (term->origin_mode) {
        /* Relative to scrolling region */
        row += term->scroll_top;
        col += 0; /* Column 0 is always left margin */
    }
    
    if (row >= 0 && row < term->rows) {
        term->cursor_row = row;
    }
    if (col >= 0 && col < term->cols) {
        term->cursor_col = col;
    }
}

/* Simpan kursor */
static void ansi_save_cursor(TerminalBuffer *term) {
    term->saved_cursor_row = term->cursor_row;
    term->saved_cursor_col = term->cursor_col;
}

/* Pulihkan kursor */
static void ansi_restore_cursor(TerminalBuffer *term) {
    term->cursor_row = term->saved_cursor_row;
    term->cursor_col = term->saved_cursor_col;
}

/* Set warna dari parameter SGR */
static void ansi_set_sgr_color(TerminalBuffer *term, int param) {
    switch (param) {
        case 0: /* Reset semua atribut */
            ansi_init_attr(&term->current_attr);
            break;
        case 1: /* Bold/bright */
            term->current_attr.bold = 1;
            term->current_attr.faint = 0;
            break;
        case 2: /* Faint/dim */
            term->current_attr.faint = 1;
            term->current_attr.bold = 0;
            break;
        case 3: /* Italic */
            term->current_attr.italic = 1;
            break;
        case 4: /* Underline */
            term->current_attr.underline = 1;
            break;
        case 5: /* Slow blink */
            term->current_attr.blink = 1;
            break;
        case 6: /* Rapid blink */
            term->current_attr.blink = 1;
            break;
        case 7: /* Reverse video */
            term->current_attr.reverse = 1;
            break;
        case 8: /* Concealed/hidden */
            term->current_attr.conceal = 1;
            break;
        case 9: /* Crossed out */
            term->current_attr.crossed_out = 1;
            break;
        case 22: /* Normal intensity */
            term->current_attr.bold = 0;
            term->current_attr.faint = 0;
            break;
        case 23: /* Not italic */
            term->current_attr.italic = 0;
            break;
        case 24: /* Not underline */
            term->current_attr.underline = 0;
            break;
        case 25: /* Not blinking */
            term->current_attr.blink = 0;
            break;
        case 27: /* Not reverse */
            term->current_attr.reverse = 0;
            break;
        case 28: /* Not concealed */
            term->current_attr.conceal = 0;
            break;
        case 29: /* Not crossed out */
            term->current_attr.crossed_out = 0;
            break;
        case 30: case 31: case 32: case 33: case 34: case 35: case 36: case 37:
            /* Set foreground color (16 warna) */
            term->current_attr.fg_type = 1;
            term->current_attr.fg_color = param - 30;
            break;
        case 38: /* Set foreground color (256/RGB) */
            if (term->parser->param_count > 1) {
                int color_type = term->parser->params[1];
                if (color_type == 5 && term->parser->param_count > 2) {
                    /* 256 warna */
                    term->current_attr.fg_type = 2;
                    term->current_attr.fg_color = term->parser->params[2];
                } else if (color_type == 2 && term->parser->param_count > 4) {
                    /* RGB */
                    term->current_attr.fg_type = 3;
                    term->current_attr.fg_rgb.r = term->parser->params[2];
                    term->current_attr.fg_rgb.g = term->parser->params[3];
                    term->current_attr.fg_rgb.b = term->parser->params[4];
                }
            }
            break;
        case 39: /* Default foreground */
            term->current_attr.fg_type = 0;
            term->current_attr.fg_color = 7;
            term->current_attr.fg_rgb = vga_colors[7];
            break;
        case 40: case 41: case 42: case 43: case 44: case 45: case 46: case 47:
            /* Set background color (16 warna) */
            term->current_attr.bg_type = 1;
            term->current_attr.bg_color = param - 40;
            break;
        case 48: /* Set background color (256/RGB) */
            if (term->parser->param_count > 1) {
                int color_type = term->parser->params[1];
                if (color_type == 5 && term->parser->param_count > 2) {
                    /* 256 warna */
                    term->current_attr.bg_type = 2;
                    term->current_attr.bg_color = term->parser->params[2];
                } else if (color_type == 2 && term->parser->param_count > 4) {
                    /* RGB */
                    term->current_attr.bg_type = 3;
                    term->current_attr.bg_rgb.r = term->parser->params[2];
                    term->current_attr.bg_rgb.g = term->parser->params[3];
                    term->current_attr.bg_rgb.b = term->parser->params[4];
                }
            }
            break;
        case 49: /* Default background */
            term->current_attr.bg_type = 0;
            term->current_attr.bg_color = 0;
            term->current_attr.bg_rgb = vga_colors[0];
            break;
        case 90: case 91: case 92: case 93: case 94: case 95: case 96: case 97:
            /* Set bright foreground color (16 warna) */
            term->current_attr.fg_type = 1;
            term->current_attr.fg_color = param - 90 + 8;
            break;
        case 100: case 101: case 102: case 103: case 104: case 105: case 106: case 107:
            /* Set bright background color (16 warna) */
            term->current_attr.bg_type = 1;
            term->current_attr.bg_color = param - 100 + 8;
            break;
    }
}

/* Dapatkan warna RGB dari atribut */
static RGBColor ansi_get_color(CharAttr *attr, int is_fg) {
    if (is_fg) {
        switch (attr->fg_type) {
            case 0: /* Default */
                return attr->fg_rgb;
            case 1: /* 16 warna */
                return vga_colors[attr->fg_color];
            case 2: /* 256 warna */
                return color256[attr->fg_color];
            case 3: /* RGB */
                return attr->fg_rgb;
        }
    } else {
        switch (attr->bg_type) {
            case 0: /* Default */
                return attr->bg_rgb;
            case 1: /* 16 warna */
                return vga_colors[attr->bg_color];
            case 2: /* 256 warna */
                return color256[attr->bg_color];
            case 3: /* RGB */
                return attr->bg_rgb;
        }
    }
    
    return vga_colors[0]; /* Default ke hitam */
}

/* Eksekusi sequence ANSI */
static void ansi_execute_sequence(TerminalBuffer *term, ANSIParser *parser, char final_char) {
    int param1, param2, param3;
    
    /* Parsing parameter jika perlu */
    if (parser->state == ANSI_STATE_CSI) {
        ansi_parse_params(parser);
    }
    
    /* Default parameter jika tidak ada */
    param1 = (parser->param_count > 0) ? parser->params[0] : 0;
    param2 = (parser->param_count > 1) ? parser->params[1] : 0;
    param3 = (parser->param_count > 2) ? parser->params[2] : 0;
    
    switch (final_char) {
        case 'A': /* Cursor up */
            ansi_move_cursor(term, term->cursor_row - (param1 ? param1 : 1), term->cursor_col);
            break;
        case 'B': /* Cursor down */
            ansi_move_cursor(term, term->cursor_row + (param1 ? param1 : 1), term->cursor_col);
            break;
        case 'C': /* Cursor forward */
            ansi_move_cursor(term, term->cursor_row, term->cursor_col + (param1 ? param1 : 1));
            break;
        case 'D': /* Cursor backward */
            ansi_move_cursor(term, term->cursor_row, term->cursor_col - (param1 ? param1 : 1));
            break;
        case 'E': /* Cursor next line */
            ansi_move_cursor(term, term->cursor_row + (param1 ? param1 : 1), 0);
            break;
        case 'F': /* Cursor previous line */
            ansi_move_cursor(term, term->cursor_row - (param1 ? param1 : 1), 0);
            break;
        case 'G': /* Cursor horizontal absolute */
            ansi_move_cursor(term, term->cursor_row, (param1 ? param1 - 1 : 0));
            break;
        case 'H': /* Cursor position */
        case 'f': /* Cursor position */
            ansi_move_cursor(term, (param1 ? param1 - 1 : 0), (param2 ? param2 - 1 : 0));
            break;
        case 'I': /* Cursor forward tabulation */
            /* Implementasi tab forward */
            break;
        case 'J': /* Erase in display */
            switch (param1) {
                case 0: /* Erase from cursor to end of screen */
                    ansi_erase_area(term, term->cursor_row, term->cursor_col, 
                                    term->rows - 1, term->cols - 1);
                    break;
                case 1: /* Erase from start to cursor */
                    ansi_erase_area(term, 0, 0, 
                                    term->cursor_row, term->cursor_col);
                    break;
                case 2: /* Erase entire screen */
                    ansi_erase_area(term, 0, 0, term->rows - 1, term->cols - 1);
                    break;
                case 3: /* Erase scrollback buffer */
                    /* Tidak didukung dalam implementasi sederhana */
                    break;
            }
            break;
        case 'K': /* Erase in line */
            switch (param1) {
                case 0: /* Erase from cursor to end of line */
                    ansi_erase_area(term, term->cursor_row, term->cursor_col, 
                                    term->cursor_row, term->cols - 1);
                    break;
                case 1: /* Erase from start to cursor */
                    ansi_erase_area(term, term->cursor_row, 0, 
                                    term->cursor_row, term->cursor_col);
                    break;
                case 2: /* Erase entire line */
                    ansi_erase_area(term, term->cursor_row, 0, 
                                    term->cursor_row, term->cols - 1);
                    break;
            }
            break;
        case 'L': /* Insert line */
            if (param1 == 0) param1 = 1;
            for (int i = 0; i < param1; i++) {
                ansi_scroll_down(term, 1);
            }
            break;
        case 'M': /* Delete line */
            if (param1 == 0) param1 = 1;
            for (int i = 0; i < param1; i++) {
                ansi_scroll_up(term, 1);
            }
            break;
        case 'P': /* Delete character */
            ansi_delete_char(term, param1 ? param1 : 1);
            break;
        case 'S': /* Scroll up */
            ansi_scroll_up(term, param1 ? param1 : 1);
            break;
        case 'T': /* Scroll down */
            ansi_scroll_down(term, param1 ? param1 : 1);
            break;
        case 'X': /* Erase character */
            ansi_erase_area(term, term->cursor_row, term->cursor_col, 
                            term->cursor_row, term->cursor_col + (param1 ? param1 - 1 : 0));
            break;
        case 'Z': /* Cursor backward tabulation */
            /* Implementasi tab backward */
            break;
        case 'a': /* Move cursor right */
            ansi_move_cursor(term, term->cursor_row, term->cursor_col + (param1 ? param1 : 1));
            break;
        case 'b': /* Repeat preceding character */
            /* Implementasi repeat character */
            break;
        case 'c': /* Send device attributes */
            /* Tidak didukung dalam implementasi sederhana */
            break;
        case 'd': /* Line position absolute */
            ansi_move_cursor(term, (param1 ? param1 - 1 : 0), term->cursor_col);
            break;
        case 'e': /* Move cursor down */
            ansi_move_cursor(term, term->cursor_row + (param1 ? param1 : 1), term->cursor_col);
            break;
        case 'h': /* Set mode */
            if (parser->private) {
                /* Private mode */
                switch (param1) {
                    case 1: /* Application cursor keys */
                        /* Tidak didukung dalam implementasi sederhana */
                        break;
                    case 3: /* 132 column mode */
                        /* Tidak didukung dalam implementasi sederhana */
                        break;
                    case 7: /* Auto wrap mode */
                        term->auto_wrap = 1;
                        break;
                    case 25: /* Show cursor */
                        term->cursor_visible = 1;
                        break;
                    case 47: /* Use alternate screen buffer */
                        /* Simpan layar saat ini */
                        if (!term->saved_screen) {
                            term->saved_screen = (TerminalCell **)malloc(term->rows * sizeof(TerminalCell *));
                            for (int i = 0; i < term->rows; i++) {
                                term->saved_screen[i] = (TerminalCell *)malloc(term->cols * sizeof(TerminalCell));
                                memcpy(term->saved_screen[i], term->cells[i], 
                                       term->cols * sizeof(TerminalCell));
                            }
                        }
                        term->screen_mode = 1;
                        break;
                    case 1000: /* Send mouse X & Y on button press and release */
                        term->mouse_mode = 2;
                        break;
                    case 1002: /* Use cell motion mouse tracking */
                        term->mouse_mode = 2;
                        break;
                    case 1003: /* Use all motion mouse tracking */
                        term->mouse_mode = 3;
                        break;
                    case 1004: /* Send focus in/out events */
                        /* Tidak didukung dalam implementasi sederhana */
                        break;
                    case 1005: /* Enable UTF-8 mouse encoding */
                        term->mouse_encoding = 1;
                        break;
                    case 1006: /* Enable SGR mouse encoding */
                        term->mouse_encoding = 2;
                        break;
                    case 2004: /* Set bracketed paste mode */
                        term->bracketed_paste = 1;
                        break;
                }
            } else {
                /* Standard mode */
                switch (param1) {
                    case 4: /* Insert mode */
                        term->insert_mode = 1;
                        break;
                    case 20: /* Automatic newline mode */
                        /* Tidak didukung dalam implementasi sederhana */
                        break;
                }
            }
            break;
        case 'l': /* Reset mode */
            if (parser->private) {
                /* Private mode */
                switch (param1) {
                    case 1: /* Normal cursor keys */
                        /* Tidak didukung dalam implementasi sederhana */
                        break;
                    case 3: /* 80 column mode */
                        /* Tidak didukung dalam implementasi sederhana */
                        break;
                    case 7: /* No auto wrap mode */
                        term->auto_wrap = 0;
                        break;
                    case 25: /* Hide cursor */
                        term->cursor_visible = 0;
                        break;
                    case 47: /* Use normal screen buffer */
                        /* Pulihkan layar tersimpan */
                        if (term->saved_screen) {
                            for (int i = 0; i < term->rows; i++) {
                                memcpy(term->cells[i], term->saved_screen[i], 
                                       term->cols * sizeof(TerminalCell));
                                free(term->saved_screen[i]);
                            }
                            free(term->saved_screen);
                            term->saved_screen = NULL;
                        }
                        term->screen_mode = 0;
                        break;
                    case 1000: /* Disable mouse tracking */
                        term->mouse_mode = 0;
                        break;
                    case 1002: /* Disable cell motion mouse tracking */
                        term->mouse_mode = 0;
                        break;
                    case 1003: /* Disable all motion mouse tracking */
                        term->mouse_mode = 0;
                        break;
                    case 1004: /* Disable focus in/out events */
                        /* Tidak didukung dalam implementasi sederhana */
                        break;
                    case 1005: /* Disable UTF-8 mouse encoding */
                        term->mouse_encoding = 0;
                        break;
                    case 1006: /* Disable SGR mouse encoding */
                        term->mouse_encoding = 0;
                        break;
                    case 2004: /* Reset bracketed paste mode */
                        term->bracketed_paste = 0;
                        break;
                }
            } else {
                /* Standard mode */
                switch (param1) {
                    case 4: /* Replace mode */
                        term->insert_mode = 0;
                        break;
                    case 20: /* Normal linefeed mode */
                        /* Tidak didukung dalam implementasi sederhana */
                        break;
                }
            }
            break;
        case 'm': /* Set graphics rendition */
            if (parser->param_count == 0) {
                ansi_set_sgr_color(term, 0);
            } else {
                for (int i = 0; i < parser->param_count; i++) {
                    ansi_set_sgr_color(term, parser->params[i]);
                }
            }
            break;
        case 'n': /* Device status report */
            /* Tidak didukung dalam implementasi sederhana */
            break;
        case 'q': /* Set cursor style */
            /* Tidak didukung dalam implementasi sederhana */
            break;
        case 'r': /* Set scrolling region */
            term->scroll_top = (param1 ? param1 - 1 : 0);
            term->scroll_bottom = (param2 ? param2 - 1 : term->rows - 1);
            if (term->scroll_top < 0) term->scroll_top = 0;
            if (term->scroll_bottom >= term->rows) term->scroll_bottom = term->rows - 1;
            if (term->scroll_top > term->scroll_bottom) {
                int temp = term->scroll_top;
                term->scroll_top = term->scroll_bottom;
                term->scroll_bottom = temp;
            }
            break;
        case 's': /* Save cursor position */
            ansi_save_cursor(term);
            break;
        case 't': /* Window manipulation */
            /* Tidak didukung dalam implementasi sederhana */
            break;
        case 'u': /* Restore cursor position */
            ansi_restore_cursor(term);
            break;
        case '`': /* Character position absolute */
            ansi_move_cursor(term, term->cursor_row, (param1 ? param1 - 1 : 0));
            break;
        case '@': /* Insert blank characters */
            /* Implementasi insert blank */
            break;
    }
}

/* Proses karakter untuk parser ANSI */
static void ansi_process_char(TerminalBuffer *term, ANSIParser *parser, char ch) {
    switch (parser->state) {
        case ANSI_STATE_NORMAL:
            if (ch == '\033') {
                parser->state = ANSI_STATE_ESCAPE;
                parser->seq_pos = 0;
                ansi_reset_params(parser);
            } else {
                /* Handle karakter normal */
                if (ch == '\n') {
                    term->cursor_row++;
                    term->cursor_col = 0;
                    if (term->cursor_row > term->scroll_bottom) {
                        ansi_scroll_up(term, 1);
                        term->cursor_row = term->scroll_bottom;
                    }
                } else if (ch == '\r') {
                    term->cursor_col = 0;
                } else if (ch == '\t') {
                    /* Tab */
                    int next_tab = term->cursor_col + 1;
                    while (next_tab < term->cols && !term->tab_stops[next_tab]) {
                        next_tab++;
                    }
                    if (next_tab >= term->cols) {
                        term->cursor_col = term->cols - 1;
                    } else {
                        term->cursor_col = next_tab;
                    }
                } else if (ch == '\b') {
                    if (term->cursor_col > 0) {
                        term->cursor_col--;
                    }
                } else if (ch >= 32 && ch < 127) {
                    /* Simpan karakter di buffer */
                    if (term->cursor_row >= 0 && term->cursor_row < term->rows &&
                        term->cursor_col >= 0 && term->cursor_col < term->cols) {
                        
                        if (term->insert_mode) {
                            ansi_insert_char(term, ch);
                        } else {
                            term->cells[term->cursor_row][term->cursor_col].ch = ch;
                            term->cells[term->cursor_row][term->cursor_col].attr = term->current_attr;
                        }
                        
                        term->cursor_col++;
                        
                        /* Auto wrap */
                        if (term->auto_wrap && term->cursor_col >= term->cols) {
                            term->cursor_col = 0;
                            term->cursor_row++;
                            if (term->cursor_row > term->scroll_bottom) {
                                ansi_scroll_up(term, 1);
                                term->cursor_row = term->scroll_bottom;
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
            } else if (ch == 'P') {
                parser->state = ANSI_STATE_STRING;
                parser->seq_pos = 0;
            } else if (ch == '_') {
                parser->state = ANSI_STATE_STRING;
                parser->seq_pos = 0;
            } else if (ch == '^') {
                parser->state = ANSI_STATE_STRING;
                parser->seq_pos = 0;
            } else {
                /* Handle single-character escape sequences */
                switch (ch) {
                    case '7': /* Save cursor */
                        ansi_save_cursor(term);
                        break;
                    case '8': /* Restore cursor */
                        ansi_restore_cursor(term);
                        break;
                    case 'D': /* Index (scroll down) */
                        ansi_scroll_down(term, 1);
                        break;
                    case 'E': /* Next line */
                        term->cursor_row++;
                        term->cursor_col = 0;
                        if (term->cursor_row > term->scroll_bottom) {
                            ansi_scroll_up(term, 1);
                            term->cursor_row = term->scroll_bottom;
                        }
                        break;
                    case 'H': /* Tab set */
                        if (term->cursor_col < term->cols) {
                            term->tab_stops[term->cursor_col] = 1;
                        }
                        break;
                    case 'M': /* Reverse index (scroll up) */
                        ansi_scroll_up(term, 1);
                        break;
                    case 'Z': /* Return terminal ID */
                        /* Tidak didukung dalam implementasi sederhana */
                        break;
                    case 'c': /* Reset terminal */
                        /* Reset terminal state */
                        ansi_init_attr(&term->current_attr);
                        term->cursor_row = 0;
                        term->cursor_col = 0;
                        term->cursor_visible = 1;
                        term->scroll_top = 0;
                        term->scroll_bottom = term->rows - 1;
                        term->origin_mode = 0;
                        term->auto_wrap = 1;
                        term->insert_mode = 0;
                        break;
                    case '=': /* Application keypad */
                        /* Tidak didukung dalam implementasi sederhana */
                        break;
                    case '>': /* Normal keypad */
                        /* Tidak didukung dalam implementasi sederhana */
                        break;
                    case '(': /* Designate G0 character set */
                        /* Tidak didukung dalam implementasi sederhana */
                        break;
                    case ')': /* Designate G1 character set */
                        /* Tidak didukung dalam implementasi sederhana */
                        break;
                }
                parser->state = ANSI_STATE_NORMAL;
            }
            break;
            
        case ANSI_STATE_CSI:
            if (parser->seq_pos < ANSI_MAX_SEQ_LEN - 1) {
                parser->seq_buf[parser->seq_pos++] = ch;
            }
            
            /* Cek apakah ini akhir dari sequence */
            if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
                ansi_execute_sequence(term, parser, ch);
                parser->state = ANSI_STATE_NORMAL;
            }
            break;
            
        case ANSI_STATE_OSC:
            /* Operating System Command */
            if (ch == '\033' && parser->seq_pos > 0 && parser->seq_buf[parser->seq_pos - 1] == '\007') {
                /* End of OSC sequence */
                parser->state = ANSI_STATE_NORMAL;
            } else if (ch == '\007') {
                /* End of OSC sequence */
                parser->state = ANSI_STATE_NORMAL;
            } else if (parser->seq_pos < ANSI_MAX_SEQ_LEN - 1) {
                parser->seq_buf[parser->seq_pos++] = ch;
            }
            break;
            
        case ANSI_STATE_STRING:
            /* String sequences (DCS, APC, PM, SOS) */
            if (ch == '\033' && parser->seq_pos > 0 && parser->seq_buf[parser->seq_pos - 1] == '\\') {
                /* End of string sequence */
                parser->state = ANSI_STATE_NORMAL;
            } else if (parser->seq_pos < ANSI_MAX_SEQ_LEN - 1) {
                parser->seq_buf[parser->seq_pos++] = ch;
            }
            break;
    }
}

/* Proses string ANSI */
static void ansi_process_string(TerminalBuffer *term, ANSIParser *parser, const char *str) {
    while (*str) {
        ansi_process_char(term, parser, *str++);
    }
}

/* Render terminal ke framebuffer */
static void ansi_render_terminal(FerminalBuffer *term, FBInfo *fb, 
                                 void (*draw_char_func)(FBInfo *, int, int, char, RGBColor, RGBColor)) {
    int i, j;
    
    /* Render semua karakter */
    for (i = 0; i < term->rows; i++) {
        for (j = 0; j < term->cols; j++) {
            char ch = term->cells[i][j].ch;
            CharAttr attr = term->cells[i][j].attr;
            
            /* Dapatkan warna foreground dan background */
            RGBColor fg = ansi_get_color(&attr, 1);
            RGBColor bg = ansi_get_color(&attr, 0);
            
            /* Handle reverse video */
            if (attr.reverse) {
                RGBColor temp = fg;
                fg = bg;
                bg = temp;
            }
            
            /* Handle bold/bright */
            if (attr.bold) {
                /* Tingkatkan kecerahan */
                fg.r = (fg.r < 170) ? fg.r + 85 : 255;
                fg.g = (fg.g < 170) ? fg.g + 85 : 255;
                fg.b = (fg.b < 170) ? fg.b + 85 : 255;
            }
            
            /* Handle faint/dim */
            if (attr.faint) {
                /* Kurangi kecerahan */
                fg.r = fg.r / 2;
                fg.g = fg.g / 2;
                fg.b = fg.b / 2;
            }
            
            /* Handle concealed/hidden */
            if (attr.conceal) {
                fg = bg; /* Sembunyikan teks dengan warna background */
            }
            
            /* Gambar karakter */
            draw_char_func(fb, j * 8, i * 16, ch, fg, bg);
        }
    }
    
    /* Render kursor jika visible */
    if (term->cursor_visible) {
        int cursor_x = term->cursor_col * 8;
        int cursor_y = term->cursor_row * 16;
        
        /* Gambar kursor sebagai garis bawah */
        for (i = 0; i < 8; i++) {
            if (cursor_x + i < fb->width && cursor_y + 15 < fb->height) {
                /* Akses piksel langsung untuk performa */
                unsigned char *pixel = (unsigned char *)fb->ptr + 
                                     (cursor_y + 15) * fb->line_length + 
                                     (cursor_x + i) * (fb->bpp / 8);
                
                RGBColor fg = ansi_get_color(&term->current_attr, 1);
                
                if (fb->bpp == 32) {
                    pixel[0] = fg.b;
                    pixel[1] = fg.g;
                    pixel[2] = fg.r;
                } else if (fb->bpp == 24) {
                    pixel[0] = fg.b;
                    pixel[1] = fg.g;
                    pixel[2] = fg.r;
                } else if (fb->bpp == 16) {
                    unsigned short color = ((fg.r >> 3) << 11) | 
                                          ((fg.g >> 2) << 5) | 
                                          (fg.b >> 3);
                    *((unsigned short*)pixel) = color;
                }
            }
        }
    }
}

#endif /* ANSI_ESCAPE_H */
