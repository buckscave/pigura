/*
 * PIGURA LIBC - CTYPE/CTYPE.C
 * ============================
 * Implementasi fungsi klasifikasi dan konversi karakter.
 *
 * Bagian dari Pigura C90 Library
 * Versi: 1.0
 */

#include <ctype.h>

/* ============================================================
 * TABEL KLASIFIKASI KARAKTER
 * ============================================================
 * Lookup table untuk klasifikasi karakter yang efisien.
 * Setiap byte dalam tabel berisi bitmask untuk klasifikasi.
 */

const unsigned char _ctype_table[256] = {
    /* 0x00-0x0F: Control characters */
    _CTYPE_CNTRL,                           /* 0x00 NUL */
    _CTYPE_CNTRL,                           /* 0x01 SOH */
    _CTYPE_CNTRL,                           /* 0x02 STX */
    _CTYPE_CNTRL,                           /* 0x03 ETX */
    _CTYPE_CNTRL,                           /* 0x04 EOT */
    _CTYPE_CNTRL,                           /* 0x05 ENQ */
    _CTYPE_CNTRL,                           /* 0x06 ACK */
    _CTYPE_CNTRL,                           /* 0x07 BEL */
    _CTYPE_CNTRL,                           /* 0x08 BS  */
    _CTYPE_CNTRL | _CTYPE_BLANK,            /* 0x09 HT  */
    _CTYPE_CNTRL | _CTYPE_SPACE,            /* 0x0A LF  */
    _CTYPE_CNTRL | _CTYPE_SPACE,            /* 0x0B VT  */
    _CTYPE_CNTRL | _CTYPE_SPACE,            /* 0x0C FF  */
    _CTYPE_CNTRL | _CTYPE_SPACE,            /* 0x0D CR  */
    _CTYPE_CNTRL,                           /* 0x0E SO  */
    _CTYPE_CNTRL,                           /* 0x0F SI  */
    
    /* 0x10-0x1F: Control characters */
    _CTYPE_CNTRL,                           /* 0x10 DLE */
    _CTYPE_CNTRL,                           /* 0x11 DC1 */
    _CTYPE_CNTRL,                           /* 0x12 DC2 */
    _CTYPE_CNTRL,                           /* 0x13 DC3 */
    _CTYPE_CNTRL,                           /* 0x14 DC4 */
    _CTYPE_CNTRL,                           /* 0x15 NAK */
    _CTYPE_CNTRL,                           /* 0x16 SYN */
    _CTYPE_CNTRL,                           /* 0x17 ETB */
    _CTYPE_CNTRL,                           /* 0x18 CAN */
    _CTYPE_CNTRL,                           /* 0x19 EM  */
    _CTYPE_CNTRL,                           /* 0x1A SUB */
    _CTYPE_CNTRL,                           /* 0x1B ESC */
    _CTYPE_CNTRL,                           /* 0x1C FS  */
    _CTYPE_CNTRL,                           /* 0x1D GS  */
    _CTYPE_CNTRL,                           /* 0x1E RS  */
    _CTYPE_CNTRL,                           /* 0x1F US  */
    
    /* 0x20-0x2F */
    _CTYPE_BLANK | _CTYPE_SPACE | _CTYPE_PUNCT,  /* 0x20 SPACE */
    _CTYPE_PUNCT,                           /* 0x21 !   */
    _CTYPE_PUNCT,                           /* 0x22 "   */
    _CTYPE_PUNCT,                           /* 0x23 #   */
    _CTYPE_PUNCT,                           /* 0x24 $   */
    _CTYPE_PUNCT,                           /* 0x25 %   */
    _CTYPE_PUNCT,                           /* 0x26 &   */
    _CTYPE_PUNCT,                           /* 0x27 '   */
    _CTYPE_PUNCT,                           /* 0x28 (   */
    _CTYPE_PUNCT,                           /* 0x29 )   */
    _CTYPE_PUNCT,                           /* 0x2A *   */
    _CTYPE_PUNCT,                           /* 0x2B +   */
    _CTYPE_PUNCT,                           /* 0x2C ,   */
    _CTYPE_PUNCT,                           /* 0x2D -   */
    _CTYPE_PUNCT,                           /* 0x2E .   */
    _CTYPE_PUNCT,                           /* 0x2F /   */
    
    /* 0x30-0x3F: Digits */
    _CTYPE_DIGIT | _CTYPE_XDIGIT,           /* 0x30 0   */
    _CTYPE_DIGIT | _CTYPE_XDIGIT,           /* 0x31 1   */
    _CTYPE_DIGIT | _CTYPE_XDIGIT,           /* 0x32 2   */
    _CTYPE_DIGIT | _CTYPE_XDIGIT,           /* 0x33 3   */
    _CTYPE_DIGIT | _CTYPE_XDIGIT,           /* 0x34 4   */
    _CTYPE_DIGIT | _CTYPE_XDIGIT,           /* 0x35 5   */
    _CTYPE_DIGIT | _CTYPE_XDIGIT,           /* 0x36 6   */
    _CTYPE_DIGIT | _CTYPE_XDIGIT,           /* 0x37 7   */
    _CTYPE_DIGIT | _CTYPE_XDIGIT,           /* 0x38 8   */
    _CTYPE_DIGIT | _CTYPE_XDIGIT,           /* 0x39 9   */
    _CTYPE_PUNCT,                           /* 0x3A :   */
    _CTYPE_PUNCT,                           /* 0x3B ;   */
    _CTYPE_PUNCT,                           /* 0x3C <   */
    _CTYPE_PUNCT,                           /* 0x3D =   */
    _CTYPE_PUNCT,                           /* 0x3E >   */
    _CTYPE_PUNCT,                           /* 0x3F ?   */
    
    /* 0x40-0x4F */
    _CTYPE_PUNCT,                           /* 0x40 @   */
    _CTYPE_UPPER | _CTYPE_XDIGIT,           /* 0x41 A   */
    _CTYPE_UPPER | _CTYPE_XDIGIT,           /* 0x42 B   */
    _CTYPE_UPPER | _CTYPE_XDIGIT,           /* 0x43 C   */
    _CTYPE_UPPER | _CTYPE_XDIGIT,           /* 0x44 D   */
    _CTYPE_UPPER | _CTYPE_XDIGIT,           /* 0x45 E   */
    _CTYPE_UPPER | _CTYPE_XDIGIT,           /* 0x46 F   */
    _CTYPE_UPPER,                           /* 0x47 G   */
    _CTYPE_UPPER,                           /* 0x48 H   */
    _CTYPE_UPPER,                           /* 0x49 I   */
    _CTYPE_UPPER,                           /* 0x4A J   */
    _CTYPE_UPPER,                           /* 0x4B K   */
    _CTYPE_UPPER,                           /* 0x4C L   */
    _CTYPE_UPPER,                           /* 0x4D M   */
    _CTYPE_UPPER,                           /* 0x4E N   */
    _CTYPE_UPPER,                           /* 0x4F O   */
    
    /* 0x50-0x5F */
    _CTYPE_UPPER,                           /* 0x50 P   */
    _CTYPE_UPPER,                           /* 0x51 Q   */
    _CTYPE_UPPER,                           /* 0x52 R   */
    _CTYPE_UPPER,                           /* 0x53 S   */
    _CTYPE_UPPER,                           /* 0x54 T   */
    _CTYPE_UPPER,                           /* 0x55 U   */
    _CTYPE_UPPER,                           /* 0x56 V   */
    _CTYPE_UPPER,                           /* 0x57 W   */
    _CTYPE_UPPER,                           /* 0x58 X   */
    _CTYPE_UPPER,                           /* 0x59 Y   */
    _CTYPE_UPPER,                           /* 0x5A Z   */
    _CTYPE_PUNCT,                           /* 0x5B [   */
    _CTYPE_PUNCT,                           /* 0x5C \   */
    _CTYPE_PUNCT,                           /* 0x5D ]   */
    _CTYPE_PUNCT,                           /* 0x5E ^   */
    _CTYPE_PUNCT,                           /* 0x5F _   */
    
    /* 0x60-0x6F */
    _CTYPE_PUNCT,                           /* 0x60 `   */
    _CTYPE_LOWER | _CTYPE_XDIGIT,           /* 0x61 a   */
    _CTYPE_LOWER | _CTYPE_XDIGIT,           /* 0x62 b   */
    _CTYPE_LOWER | _CTYPE_XDIGIT,           /* 0x63 c   */
    _CTYPE_LOWER | _CTYPE_XDIGIT,           /* 0x64 d   */
    _CTYPE_LOWER | _CTYPE_XDIGIT,           /* 0x65 e   */
    _CTYPE_LOWER | _CTYPE_XDIGIT,           /* 0x66 f   */
    _CTYPE_LOWER,                           /* 0x67 g   */
    _CTYPE_LOWER,                           /* 0x68 h   */
    _CTYPE_LOWER,                           /* 0x69 i   */
    _CTYPE_LOWER,                           /* 0x6A j   */
    _CTYPE_LOWER,                           /* 0x6B k   */
    _CTYPE_LOWER,                           /* 0x6C l   */
    _CTYPE_LOWER,                           /* 0x6D m   */
    _CTYPE_LOWER,                           /* 0x6E n   */
    _CTYPE_LOWER,                           /* 0x6F o   */
    
    /* 0x70-0x7F */
    _CTYPE_LOWER,                           /* 0x70 p   */
    _CTYPE_LOWER,                           /* 0x71 q   */
    _CTYPE_LOWER,                           /* 0x72 r   */
    _CTYPE_LOWER,                           /* 0x73 s   */
    _CTYPE_LOWER,                           /* 0x74 t   */
    _CTYPE_LOWER,                           /* 0x75 u   */
    _CTYPE_LOWER,                           /* 0x76 v   */
    _CTYPE_LOWER,                           /* 0x77 w   */
    _CTYPE_LOWER,                           /* 0x78 x   */
    _CTYPE_LOWER,                           /* 0x79 y   */
    _CTYPE_LOWER,                           /* 0x7A z   */
    _CTYPE_PUNCT,                           /* 0x7B {   */
    _CTYPE_PUNCT,                           /* 0x7C |   */
    _CTYPE_PUNCT,                           /* 0x7D }   */
    _CTYPE_PUNCT,                           /* 0x7E ~   */
    _CTYPE_CNTRL,                           /* 0x7F DEL */
    
    /* 0x80-0xFF: Extended ASCII (treated as non-ASCII) */
    /* Semua dianggap tidak terdefinisi untuk fungsi standar */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

/* ============================================================
 * FUNGSI KLASIFIKASI KARAKTER
 * ============================================================
 */

int isalnum(int c) {
    if (c < 0 || c > 255) {
        return 0;
    }
    return _ctype_table[(unsigned char)c] &
           (_CTYPE_UPPER | _CTYPE_LOWER | _CTYPE_DIGIT);
}

int isalpha(int c) {
    if (c < 0 || c > 255) {
        return 0;
    }
    return _ctype_table[(unsigned char)c] &
           (_CTYPE_UPPER | _CTYPE_LOWER);
}

int isblank(int c) {
    if (c < 0 || c > 255) {
        return 0;
    }
    return _ctype_table[(unsigned char)c] & _CTYPE_BLANK;
}

int iscntrl(int c) {
    if (c < 0 || c > 255) {
        return 0;
    }
    return _ctype_table[(unsigned char)c] & _CTYPE_CNTRL;
}

int isdigit(int c) {
    if (c < 0 || c > 255) {
        return 0;
    }
    return _ctype_table[(unsigned char)c] & _CTYPE_DIGIT;
}

int isgraph(int c) {
    if (c < 0 || c > 255) {
        return 0;
    }
    return _ctype_table[(unsigned char)c] &
           (_CTYPE_UPPER | _CTYPE_LOWER | _CTYPE_DIGIT | _CTYPE_PUNCT);
}

int islower(int c) {
    if (c < 0 || c > 255) {
        return 0;
    }
    return _ctype_table[(unsigned char)c] & _CTYPE_LOWER;
}

int isprint(int c) {
    if (c < 0 || c > 255) {
        return 0;
    }
    return _ctype_table[(unsigned char)c] &
           (_CTYPE_UPPER | _CTYPE_LOWER | _CTYPE_DIGIT |
            _CTYPE_PUNCT | _CTYPE_BLANK);
}

int ispunct(int c) {
    if (c < 0 || c > 255) {
        return 0;
    }
    return _ctype_table[(unsigned char)c] & _CTYPE_PUNCT;
}

int isspace(int c) {
    if (c < 0 || c > 255) {
        return 0;
    }
    return _ctype_table[(unsigned char)c] & _CTYPE_SPACE;
}

int isupper(int c) {
    if (c < 0 || c > 255) {
        return 0;
    }
    return _ctype_table[(unsigned char)c] & _CTYPE_UPPER;
}

int isxdigit(int c) {
    if (c < 0 || c > 255) {
        return 0;
    }
    return _ctype_table[(unsigned char)c] & _CTYPE_XDIGIT;
}

/* ============================================================
 * FUNGSI KONVERSI KARAKTER
 * ============================================================
 */

int tolower(int c) {
    if (c >= 'A' && c <= 'Z') {
        return c + ('a' - 'A');
    }
    return c;
}

int toupper(int c) {
    if (c >= 'a' && c <= 'z') {
        return c - ('a' - 'A');
    }
    return c;
}

/* ============================================================
 * FUNGSI NON-STANDAR
 * ============================================================
 */

int isascii(int c) {
    return (c >= 0 && c <= 127);
}

int toascii(int c) {
    return (c & 0x7F);
}

/* ============================================================
 * FUNGSI TAMBAHAN
 * ============================================================
 */

/*
 * isdigit_base - Cek digit dengan basis tertentu
 *
 * Parameter:
 *   c    - Karakter
 *   base - Basis (2-36)
 *
 * Return: Nilai digit, atau -1 jika bukan digit
 */
int isdigit_base(int c, int base) {
    int value;
    
    if (base < 2 || base > 36) {
        return -1;
    }
    
    if (c >= '0' && c <= '9') {
        value = c - '0';
    } else if (c >= 'A' && c <= 'Z') {
        value = c - 'A' + 10;
    } else if (c >= 'a' && c <= 'z') {
        value = c - 'a' + 10;
    } else {
        return -1;
    }
    
    if (value < base) {
        return value;
    }
    
    return -1;
}

/*
 * tolower_ascii - Konversi ke lowercase (ASCII only, branchless)
 */
int tolower_ascii(int c) {
    return c | ((c >= 'A' && c <= 'Z') << 5);
}

/*
 * toupper_ascii - Konversi ke uppercase (ASCII only, branchless)
 */
int toupper_ascii(int c) {
    return c & ~((c >= 'a' && c <= 'z') << 5);
}

/*
 * isvowel - Cek apakah karakter adalah huruf vokal
 */
int isvowel(int c) {
    int lower = tolower(c);
    return (lower == 'a' || lower == 'e' || lower == 'i' ||
            lower == 'o' || lower == 'u');
}

/*
 * isconsonant - Cek apakah karakter adalah huruf konsonan
 */
int isconsonant(int c) {
    return isalpha(c) && !isvowel(c);
}
