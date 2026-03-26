/*
 * PIGURA OS - MASUKAN.H
 * =====================
 * Header untuk subsistem perangkat masukan (input devices).
 *
 * Berkas ini mendefinisikan interface untuk keyboard, mouse,
 * touchscreen, joystick, gamepad, dan HID devices.
 *
 * Kepatuhan standar:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Mendukung semua arsitektur
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#ifndef PERANGKAT_MASUKAN_H
#define PERANGKAT_MASUKAN_H

/*
 * ===========================================================================
 * INCLUDE
 * ===========================================================================
 */

#include "../../inti/types.h"

/*
 * ===========================================================================
 * KONSTANTA MASUKAN
 * ===========================================================================
 */

/* Versi modul */
#define MASUKAN_VERSI_MAJOR 1
#define MASUKAN_VERSI_MINOR 0
#define MASUKAN_VERSI_PATCH 0

/* Magic numbers */
#define MASUKAN_MAGIC          0x4D534B4E  /* "MSKN" */
#define INPUTDEV_MAGIC         0x49504456  /* "IPDV" */
#define EVENT_MAGIC            0x49455654  /* "IEVT" */

/* Batas sistem */
#define INPUTDEV_NAMA_MAKS     32
#define INPUTDEV_MAKS          32
#define INPUT_EVENT_QUEUE      256
#define INPUT_AXIS_MAKS        8
#define INPUT_BUTTON_MAKS      32
#define INPUT_KEY_MAKS         256

/* Tipe device */
#define INPUT_TIPE_TIDAK_DIKETAHUI  0
#define INPUT_TIPE_KEYBOARD         1
#define INPUT_TIPE_MOUSE            2
#define INPUT_TIPE_TOUCHSCREEN      3
#define INPUT_TIPE_JOYSTICK         4
#define INPUT_TIPE_GAMEPAD          5
#define INPUT_TIPE_TOUCHPAD         6
#define INPUT_TIPE_TABLET           7
#define INPUT_TIPE_STYLUS           8

/* Status device */
#define INPUT_STATUS_TIDAK_ADA      0
#define INPUT_STATUS_TERDETEKSI     1
#define INPUT_STATUS_AKTIF          2
#define INPUT_STATUS_ERROR          3

/* Tipe event */
#define EVENT_TIPE_TIDAK_ADA        0
#define EVENT_TIPE_KEY              1
#define EVENT_TIPE_RELATIVE         2
#define EVENT_TIPE_ABSOLUTE         3
#define EVENT_TIPE_TOUCH            4

/* Key codes - Keyboard */
#define KEY_TIDAK_ADA               0
#define KEY_ESC                     1
#define KEY_1                       2
#define KEY_2                       3
#define KEY_3                       4
#define KEY_4                       5
#define KEY_5                       6
#define KEY_6                       7
#define KEY_7                       8
#define KEY_8                       9
#define KEY_9                       10
#define KEY_0                       11
#define KEY_MINUS                   12
#define KEY_EQUAL                   13
#define KEY_BACKSPACE               14
#define KEY_TAB                     15
#define KEY_Q                       16
#define KEY_W                       17
#define KEY_E                       18
#define KEY_R                       19
#define KEY_T                       20
#define KEY_Y                       21
#define KEY_U                       22
#define KEY_I                       23
#define KEY_O                       24
#define KEY_P                       25
#define KEY_LEFTBRACE               26
#define KEY_RIGHTBRACE              27
#define KEY_ENTER                   28
#define KEY_LEFTCTRL                29
#define KEY_A                       30
#define KEY_S                       31
#define KEY_D                       32
#define KEY_F                       33
#define KEY_G                       34
#define KEY_H                       35
#define KEY_J                       36
#define KEY_K                       37
#define KEY_L                       38
#define KEY_SEMICOLON               39
#define KEY_APOSTROPHE              40
#define KEY_GRAVE                   41
#define KEY_LEFTSHIFT               42
#define KEY_BACKSLASH               43
#define KEY_Z                       44
#define KEY_X                       45
#define KEY_C                       46
#define KEY_V                       47
#define KEY_B                       48
#define KEY_N                       49
#define KEY_M                       50
#define KEY_COMMA                   51
#define KEY_DOT                     52
#define KEY_SLASH                   53
#define KEY_RIGHTSHIFT              54
#define KEY_KPASTERISK              55
#define KEY_LEFTALT                 56
#define KEY_SPACE                   57
#define KEY_CAPSLOCK                58
#define KEY_F1                      59
#define KEY_F2                      60
#define KEY_F3                      61
#define KEY_F4                      62
#define KEY_F5                      63
#define KEY_F6                      64
#define KEY_F7                      65
#define KEY_F8                      66
#define KEY_F9                      67
#define KEY_F10                     68
#define KEY_NUMLOCK                 69
#define KEY_SCROLLLOCK              70
#define KEY_KP7                     71
#define KEY_KP8                     72
#define KEY_KP9                     73
#define KEY_KPMINUS                 74
#define KEY_KP4                     75
#define KEY_KP5                     76
#define KEY_KP6                     77
#define KEY_KPPLUS                  78
#define KEY_KP1                     79
#define KEY_KP2                     80
#define KEY_KP3                     81
#define KEY_KP0                     82
#define KEY_KPDOT                   83
#define KEY_F11                     87
#define KEY_F12                     88

/* Extended keys */
#define KEY_RIGHTCTRL               97
#define KEY_RIGHTALT                100
#define KEY_HOME                    102
#define KEY_UP                      103
#define KEY_PAGEUP                  104
#define KEY_LEFT                    105
#define KEY_RIGHT                   106
#define KEY_END                     107
#define KEY_DOWN                    108
#define KEY_PAGEDOWN                109
#define KEY_INSERT                  110
#define KEY_DELETE                  111

/* Modifier flags */
#define KEY_MOD_NONE                0x00
#define KEY_MOD_CTRL                0x01
#define KEY_MOD_SHIFT               0x02
#define KEY_MOD_ALT                 0x04
#define KEY_MOD_SUPER               0x08
#define KEY_MOD_CAPSLOCK            0x10
#define KEY_MOD_NUMLOCK             0x20

/* Mouse buttons */
#define MOUSE_BUTTON_NONE           0
#define MOUSE_BUTTON_LEFT           1
#define MOUSE_BUTTON_RIGHT          2
#define MOUSE_BUTTON_MIDDLE         3
#define MOUSE_BUTTON_SIDE           4
#define MOUSE_BUTTON_EXTRA          5

/* Mouse axes */
#define MOUSE_AXIS_X                0
#define MOUSE_AXIS_Y                1
#define MOUSE_AXIS_WHEEL            2
#define MOUSE_AXIS_HWHEEL           3

/* Touch events */
#define TOUCH_EVENT_DOWN            1
#define TOUCH_EVENT_UP              2
#define TOUCH_EVENT_MOVE            3
#define TOUCH_EVENT_CANCEL          4

/* Joystick/Gamepad axes */
#define JOY_AXIS_X                  0
#define JOY_AXIS_Y                  1
#define JOY_AXIS_Z                  2
#define JOY_AXIS_RX                 3
#define JOY_AXIS_RY                 4
#define JOY_AXIS_RZ                 5
#define JOY_AXIS_THROTTLE           6
#define JOY_AXIS_RUDDER             7

/* Gamepad buttons */
#define GAMEPAD_A                   0
#define GAMEPAD_B                   1
#define GAMEPAD_X                   2
#define GAMEPAD_Y                   3
#define GAMEPAD_LB                  4
#define GAMEPAD_RB                  5
#define GAMEPAD_BACK                6
#define GAMEPAD_START               7
#define GAMEPAD_GUIDE               8
#define GAMEPAD_LEFTSTICK           9
#define GAMEPAD_RIGHTSTICK          10
#define GAMEPAD_DPAD_UP             11
#define GAMEPAD_DPAD_DOWN           12
#define GAMEPAD_DPAD_LEFT           13
#define GAMEPAD_DPAD_RIGHT          14

/*
 * ===========================================================================
 * STRUKTUR EVENT
 * ===========================================================================
 */

/* Event keyboard */
typedef struct {
    tak_bertanda32_t kode;          /* Key code */
    tak_bertanda32_t status;        /* 0 = release, 1 = press, 2 = repeat */
    tak_bertanda32_t modifier;      /* Modifier flags */
    tak_bertanda32_t scancode;      /* Hardware scancode */
    tak_bertanda32_t unicode;       /* Unicode character */
} keyboard_event_t;

/* Event mouse */
typedef struct {
    tak_bertanda32_t tombol;        /* Button mask */
    tak_bertanda32_t tombol_status; /* Button change mask */
    tanda32_t rel_x;                /* Relative X movement */
    tanda32_t rel_y;                /* Relative Y movement */
    tanda32_t wheel;                /* Wheel movement */
    tanda32_t hwheel;               /* Horizontal wheel */
    tak_bertanda32_t abs_x;         /* Absolute X position */
    tak_bertanda32_t abs_y;         /* Absolute Y position */
} mouse_event_t;

/* Event touch */
typedef struct {
    tak_bertanda32_t id;            /* Touch ID */
    tak_bertanda32_t tipe;          /* Event type */
    tak_bertanda32_t x;             /* X position */
    tak_bertanda32_t y;             /* Y position */
    tak_bertanda32_t pressure;      /* Pressure (0-255) */
    tak_bertanda32_t area;          /* Touch area */
} touch_event_t;

/* Event joystick */
typedef struct {
    tak_bertanda32_t tombol;        /* Button mask */
    tak_bertanda32_t tombol_change; /* Button change mask */
    tanda32_t axis[INPUT_AXIS_MAKS]; /* Axis values */
    tak_bertanda32_t axis_count;    /* Number of axes */
} joystick_event_t;

/* Generic input event */
typedef struct {
    tak_bertanda32_t magic;         /* Magic number */
    tak_bertanda64_t timestamp;     /* Timestamp (microseconds) */
    tak_bertanda32_t tipe;          /* Event type */
    tak_bertanda32_t device_id;     /* Source device ID */
    
    union {
        keyboard_event_t key;
        mouse_event_t mouse;
        touch_event_t touch;
        joystick_event_t joystick;
    } data;
    
} input_event_t;

/*
 * ===========================================================================
 * STRUKTUR DEVICE
 * ===========================================================================
 */

typedef struct inputdev inputdev_t;

/* Callback untuk event */
typedef void (*input_event_callback_t)(inputdev_t *dev, input_event_t *event);

/* Callback untuk interrupt */
typedef void (*input_irq_handler_t)(inputdev_t *dev);

struct inputdev {
    tak_bertanda32_t magic;         /* Magic number */
    tak_bertanda32_t id;            /* Device ID */
    char nama[INPUTDEV_NAMA_MAKS];  /* Device name */
    
    /* Tipe dan status */
    tak_bertanda32_t tipe;          /* Device type */
    tak_bertanda32_t status;        /* Status */
    tak_bertanda32_t flags;         /* Flags */
    
    /* Kemampuan */
    tak_bertanda32_t num_keys;      /* Number of keys */
    tak_bertanda32_t num_buttons;   /* Number of buttons */
    tak_bertanda32_t num_axes;      /* Number of axes */
    tak_bertanda32_t num_touches;   /* Max touch points */
    
    /* Range untuk absolute axes */
    tanda32_t abs_min_x;
    tanda32_t abs_max_x;
    tanda32_t abs_min_y;
    tanda32_t abs_max_y;
    
    /* State saat ini */
    tak_bertanda32_t key_state;     /* Current key modifiers */
    tak_bertanda32_t button_state;  /* Current button state */
    tanda32_t axis_value[INPUT_AXIS_MAKS];
    tak_bertanda32_t touch_count;
    
    /* Event queue */
    input_event_t event_queue[INPUT_EVENT_QUEUE];
    tak_bertanda32_t queue_head;
    tak_bertanda32_t queue_tail;
    tak_bertanda32_t queue_count;
    
    /* Callback */
    input_event_callback_t callback;
    input_irq_handler_t irq_handler;
    
    /* Private data */
    void *priv;
    
    /* Next device */
    inputdev_t *berikutnya;
};

/*
 * ===========================================================================
 * STRUKTUR KONTEKS MASUKAN
 * ===========================================================================
 */

typedef struct {
    tak_bertanda32_t magic;         /* Magic number */
    
    /* Device list */
    inputdev_t *device_list;
    tak_bertanda32_t device_count;
    
    /* Global event queue */
    input_event_t global_queue[INPUT_EVENT_QUEUE * 2];
    tak_bertanda32_t global_head;
    tak_bertanda32_t global_tail;
    
    /* Keyboard state */
    tak_bertanda32_t led_state;     /* LED state (numlock, capslock, etc) */
    tak_bertanda32_t repeat_delay;  /* Key repeat delay (ms) */
    tak_bertanda32_t repeat_rate;   /* Key repeat rate */
    
    /* Mouse state */
    tak_bertanda32_t mouse_x;       /* Current mouse X */
    tak_bertanda32_t mouse_y;       /* Current mouse Y */
    tak_bertanda32_t mouse_buttons; /* Current button state */
    
    /* Statistics */
    tak_bertanda64_t total_events;
    
    /* Status */
    bool_t diinisialisasi;
    
} masukan_konteks_t;

/*
 * ===========================================================================
 * VARIABEL GLOBAL
 * ===========================================================================
 */

extern masukan_konteks_t g_masukan_konteks;
extern bool_t g_masukan_diinisialisasi;

/*
 * ===========================================================================
 * FUNGSI INISIALISASI
 * ===========================================================================
 */

/*
 * masukan_init - Inisialisasi subsistem masukan
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t masukan_init(void);

/*
 * masukan_shutdown - Matikan subsistem masukan
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t masukan_shutdown(void);

/*
 * masukan_konteks_dapatkan - Dapatkan konteks masukan
 *
 * Return: Pointer ke konteks atau NULL
 */
masukan_konteks_t *masukan_konteks_dapatkan(void);

/*
 * ===========================================================================
 * FUNGSI DEVICE
 * ===========================================================================
 */

/*
 * inputdev_register - Registrasi device masukan
 *
 * Parameter:
 *   nama - Nama device
 *   tipe - Tipe device
 *
 * Return: Pointer ke device atau NULL
 */
inputdev_t *inputdev_register(const char *nama, tak_bertanda32_t tipe);

/*
 * inputdev_unregister - Unregister device
 *
 * Parameter:
 *   dev - Pointer ke device
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t inputdev_unregister(inputdev_t *dev);

/*
 * inputdev_cari - Cari device berdasarkan ID
 *
 * Parameter:
 *   id - Device ID
 *
 * Return: Pointer ke device atau NULL
 */
inputdev_t *inputdev_cari(tak_bertanda32_t id);

/*
 * inputdev_cari_tipe - Cari device berdasarkan tipe
 *
 * Parameter:
 *   tipe - Tipe device
 *   indeks - Index (untuk multiple devices)
 *
 * Return: Pointer ke device atau NULL
 */
inputdev_t *inputdev_cari_tipe(tak_bertanda32_t tipe,
                                tak_bertanda32_t indeks);

/*
 * inputdev_set_callback - Set callback untuk event
 *
 * Parameter:
 *   dev - Pointer ke device
 *   callback - Fungsi callback
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t inputdev_set_callback(inputdev_t *dev,
                                input_event_callback_t callback);

/*
 * ===========================================================================
 * FUNGSI EVENT
 * ===========================================================================
 */

/*
 * masukan_event_push - Push event ke queue
 *
 * Parameter:
 *   event - Event yang akan di-push
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t masukan_event_push(input_event_t *event);

/*
 * masukan_event_pop - Pop event dari queue
 *
 * Parameter:
 *   event - Buffer untuk event
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t masukan_event_pop(input_event_t *event);

/*
 * masukan_event_peek - Peek event tanpa menghapus
 *
 * Parameter:
 *   event - Buffer untuk event
 *
 * Return: STATUS_BERHASIL jika ada event
 */
status_t masukan_event_peek(input_event_t *event);

/*
 * masukan_event_count - Dapatkan jumlah event dalam queue
 *
 * Return: Jumlah event
 */
tak_bertanda32_t masukan_event_count(void);

/*
 * masukan_event_clear - Bersihkan event queue
 */
void masukan_event_clear(void);

/*
 * ===========================================================================
 * FUNGSI KEYBOARD
 * ===========================================================================
 */

/*
 * papanketik_init - Inisialisasi subsistem keyboard
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t papanketik_init(void);

/*
 * papanketik_event - Proses keyboard event
 *
 * Parameter:
 *   scancode - Scancode dari hardware
 *   status - Status (press/release)
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t papanketik_event(tak_bertanda32_t scancode, tak_bertanda32_t status);

/*
 * papanketik_set_leds - Set LED keyboard
 *
 * Parameter:
 *   leds - LED flags (numlock, capslock, scrolllock)
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t papanketik_set_leds(tak_bertanda32_t leds);

/*
 * papanketik_get_state - Dapatkan state keyboard
 *
 * Return: Modifier flags
 */
tak_bertanda32_t papanketik_get_state(void);

/*
 * papanketik_translate - Translate scancode ke unicode
 *
 * Parameter:
 *   scancode - Scancode
 *   modifier - Modifier flags
 *
 * Return: Unicode character atau 0
 */
tak_bertanda32_t papanketik_translate(tak_bertanda32_t scancode,
                                       tak_bertanda32_t modifier);

/*
 * ===========================================================================
 * FUNGSI MOUSE
 * ===========================================================================
 */

/*
 * tetikus_init - Inisialisasi subsistem mouse
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t tetikus_init(void);

/*
 * tetikus_event - Proses mouse event
 *
 * Parameter:
 *   buttons - Button state
 *   dx - Delta X
 *   dy - Delta Y
 *   wheel - Wheel delta
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t tetikus_event(tak_bertanda32_t buttons, tanda32_t dx,
                        tanda32_t dy, tanda32_t wheel);

/*
 * tetikus_get_position - Dapatkan posisi mouse
 *
 * Parameter:
 *   x - Pointer untuk X
 *   y - Pointer untuk Y
 */
void tetikus_get_position(tak_bertanda32_t *x, tak_bertanda32_t *y);

/*
 * tetikus_set_position - Set posisi mouse
 *
 * Parameter:
 *   x - X position
 *   y - Y position
 */
void tetikus_set_position(tak_bertanda32_t x, tak_bertanda32_t y);

/*
 * tetikus_get_buttons - Dapatkan state tombol
 *
 * Return: Button mask
 */
tak_bertanda32_t tetikus_get_buttons(void);

/*
 * ===========================================================================
 * FUNGSI TOUCHSCREEN
 * ===========================================================================
 */

/*
 * layarsentuh_init - Inisialisasi subsistem touchscreen
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t layarsentuh_init(void);

/*
 * layarsentuh_event - Proses touch event
 *
 * Parameter:
 *   id - Touch ID
 *   tipe - Event type
 *   x - X position
 *   y - Y position
 *   pressure - Pressure
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t layarsentuh_event(tak_bertanda32_t id, tak_bertanda32_t tipe,
                            tak_bertanda32_t x, tak_bertanda32_t y,
                            tak_bertanda32_t pressure);

/*
 * layarsentuh_get_touches - Dapatkan touch points aktif
 *
 * Parameter:
 *   touches - Buffer untuk touch events
 *   max_count - Maximum touches
 *
 * Return: Jumlah touch points
 */
tak_bertanda32_t layarsentuh_get_touches(touch_event_t *touches,
                                          tak_bertanda32_t max_count);

/*
 * ===========================================================================
 * FUNGSI JOYSTICK
 * ===========================================================================
 */

/*
 * joystick_init - Inisialisasi subsistem joystick
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t joystick_init(void);

/*
 * joystick_event - Proses joystick event
 *
 * Parameter:
 *   id - Device ID
 *   axis - Axis values array
 *   buttons - Button mask
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t joystick_event(tak_bertanda32_t id, tanda32_t *axis,
                         tak_bertanda32_t buttons);

/*
 * ===========================================================================
 * FUNGSI GAMEPAD
 * ===========================================================================
 */

/*
 * gamepad_init - Inisialisasi subsistem gamepad
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t gamepad_init(void);

/*
 * gamepad_event - Proses gamepad event
 *
 * Parameter:
 *   id - Device ID
 *   buttons - Button mask
 *   left_x - Left stick X (-32768 to 32767)
 *   left_y - Left stick Y
 *   right_x - Right stick X
 *   right_y - Right stick Y
 *   triggers - L2/R2 trigger values
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t gamepad_event(tak_bertanda32_t id, tak_bertanda32_t buttons,
                        tanda16_t left_x, tanda16_t left_y,
                        tanda16_t right_x, tanda16_t right_y,
                        tak_bertanda32_t triggers);

/*
 * gamepad_rumble - Set rumble/vibration
 *
 * Parameter:
 *   id - Device ID
 *   weak - Weak motor strength (0-65535)
 *   strong - Strong motor strength (0-65535)
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t gamepad_rumble(tak_bertanda32_t id, tak_bertanda16_t weak,
                         tak_bertanda16_t strong);

/*
 * ===========================================================================
 * FUNGSI HID
 * ===========================================================================
 */

/*
 * hid_init - Inisialisasi HID parser
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t hid_init(void);

/*
 * hid_parse - Parse HID report descriptor
 *
 * Parameter:
 *   dev - Device
 *   descriptor - HID descriptor
 *   len - Descriptor length
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t hid_parse(inputdev_t *dev, const tak_bertanda8_t *descriptor,
                    ukuran_t len);

/*
 * hid_process_report - Process HID report
 *
 * Parameter:
 *   dev - Device
 *   report - Report data
 *   len - Report length
 *
 * Return: STATUS_BERHASIL jika berhasil
 */
status_t hid_process_report(inputdev_t *dev, const tak_bertanda8_t *report,
                             ukuran_t len);

/*
 * ===========================================================================
 * FUNGSI UTILITAS
 * ===========================================================================
 */

/*
 * masukan_nama_tipe - Dapatkan nama tipe device
 *
 * Parameter:
 *   tipe - Tipe device
 *
 * Return: String nama tipe
 */
const char *masukan_nama_tipe(tak_bertanda32_t tipe);

/*
 * masukan_nama_key - Dapatkan nama key
 *
 * Parameter:
 *   kode - Key code
 *
 * Return: String nama key
 */
const char *masukan_nama_key(tak_bertanda32_t kode);

/*
 * masukan_cetak_info - Cetak informasi masukan
 */
void masukan_cetak_info(void);

/*
 * masukan_cetak_event - Cetak event (debug)
 *
 * Parameter:
 *   event - Event yang akan dicetak
 */
void masukan_cetak_event(input_event_t *event);

#endif /* PERANGKAT_MASUKAN_H */
