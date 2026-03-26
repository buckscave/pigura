/*
 * PIGURA OS - HID.C
 * =================
 * Implementasi parser HID (Human Interface Device).
 *
 * Berkas ini mengimplementasikan parsing HID report descriptor
 * dan processing HID reports untuk berbagai perangkat masukan.
 *
 * Kepatuhan standar:
 * - C90 (ANSI C89) dengan POSIX Safe Functions
 * - Mendukung semua arsitektur (x86, x86_64, ARM, ARMv7, ARM64)
 *
 * Versi: 1.0
 * Tanggal: 2025
 */

#include "masukan.h"
#include "../../inti/kernel.h"

/*
 * ===========================================================================
 * KONSTANTA HID
 * ===========================================================================
 */

/* HID Usage Pages */
#define HID_USAGE_PAGE_GENERIC_DESKTOP    0x01
#define HID_USAGE_PAGE_SIMULATION         0x02
#define HID_USAGE_PAGE_VR                 0x03
#define HID_USAGE_PAGE_SPORT              0x04
#define HID_USAGE_PAGE_GAME               0x05
#define HID_USAGE_PAGE_GENERIC_DEVICE     0x06
#define HID_USAGE_PAGE_KEYBOARD           0x07
#define HID_USAGE_PAGE_LED                0x08
#define HID_USAGE_PAGE_BUTTON             0x09
#define HID_USAGE_PAGE_ORDINAL            0x0A
#define HID_USAGE_PAGE_TELEPHONY          0x0B
#define HID_USAGE_PAGE_CONSUMER           0x0C
#define HID_USAGE_PAGE_DIGITIZER          0x0D
#define HID_USAGE_PAGE_PID                0x0F
#define HID_USAGE_PAGE_UNICODE            0x10
#define HID_USAGE_PAGE_VENDOR             0xFF

/* HID Usages - Generic Desktop */
#define HID_USAGE_POINTER                 0x01
#define HID_USAGE_MOUSE                   0x02
#define HID_USAGE_JOYSTICK                0x04
#define HID_USAGE_GAMEPAD                 0x05
#define HID_USAGE_KEYBOARD                0x06
#define HID_USAGE_KEYPAD                  0x07
#define HID_USAGE_X                       0x30
#define HID_USAGE_Y                       0x31
#define HID_USAGE_Z                       0x32
#define HID_USAGE_RX                      0x33
#define HID_USAGE_RY                      0x34
#define HID_USAGE_RZ                      0x35
#define HID_USAGE_SLIDER                  0x36
#define HID_USAGE_DIAL                    0x37
#define HID_USAGE_WHEEL                   0x38
#define HID_USAGE_HAT_SWITCH              0x39

/* HID Report Types */
#define HID_REPORT_TYPE_INPUT             0x01
#define HID_REPORT_TYPE_OUTPUT            0x02
#define HID_REPORT_TYPE_FEATURE           0x03

/* HID Main Items */
#define HID_ITEM_INPUT                    0x80
#define HID_ITEM_OUTPUT                   0x90
#define HID_ITEM_FEATURE                  0xB0
#define HID_ITEM_COLLECTION               0xA0
#define HID_ITEM_END_COLLECTION           0xC0

/* HID Global Items */
#define HID_ITEM_USAGE_PAGE               0x04
#define HID_ITEM_LOGICAL_MINIMUM          0x14
#define HID_ITEM_LOGICAL_MAXIMUM          0x24
#define HID_ITEM_PHYSICAL_MINIMUM         0x34
#define HID_ITEM_PHYSICAL_MAXIMUM         0x44
#define HID_ITEM_UNIT_EXPONENT            0x54
#define HID_ITEM_UNIT                     0x64
#define HID_ITEM_REPORT_SIZE              0x74
#define HID_ITEM_REPORT_ID                0x84
#define HID_ITEM_REPORT_COUNT             0x94
#define HID_ITEM_PUSH                     0xA4
#define HID_ITEM_POP                      0xB4

/* HID Local Items */
#define HID_ITEM_USAGE                    0x08
#define HID_ITEM_USAGE_MINIMUM            0x18
#define HID_ITEM_USAGE_MAXIMUM            0x28
#define HID_ITEM_DESIGNATOR_INDEX         0x38
#define HID_ITEM_DESIGNATOR_MINIMUM       0x48
#define HID_ITEM_DESIGNATOR_MAXIMUM       0x58
#define HID_ITEM_STRING_INDEX             0x78
#define HID_ITEM_STRING_MINIMUM           0x88
#define HID_ITEM_STRING_MAXIMUM           0x98
#define HID_ITEM_DELIMITER                0xA8

/* Collection Types */
#define HID_COLLECTION_PHYSICAL           0x00
#define HID_COLLECTION_APPLICATION        0x01
#define HID_COLLECTION_LOGICAL            0x02
#define HID_COLLECTION_REPORT             0x03
#define HID_COLLECTION_NAMED_ARRAY        0x04
#define HID_COLLECTION_USAGE_SWITCH       0x05
#define HID_COLLECTION_MODIFIER           0x06

/* Batasan */
#define HID_MAX_USAGES                    128
#define HID_MAX_REPORTS                   16
#define HID_MAX_COLLECTIONS               16
#define HID_STACK_DEPTH                   8

/*
 * ===========================================================================
 * STRUKTUR HID INTERNAL
 * ===========================================================================
 */

/* State parser HID */
typedef struct {
    tak_bertanda32_t usage_page;
    tak_bertanda32_t usage;
    tanda32_t logical_min;
    tanda32_t logical_max;
    tanda32_t physical_min;
    tanda32_t physical_max;
    tak_bertanda32_t unit;
    tak_bertanda32_t unit_exponent;
    tak_bertanda32_t report_size;
    tak_bertanda32_t report_id;
    tak_bertanda32_t report_count;
    tak_bertanda32_t usage_min;
    tak_bertanda32_t usage_max;
} hid_state_t;

/* Stack state untuk push/pop */
typedef struct {
    hid_state_t stack[HID_STACK_DEPTH];
    tak_bertanda32_t depth;
} hid_stack_t;

/* Collection HID */
typedef struct {
    tak_bertanda32_t tipe;
    tak_bertanda32_t usage_page;
    tak_bertanda32_t usage;
    tak_bertanda32_t parent_index;
} hid_collection_t;

/* Report field HID */
typedef struct {
    tak_bertanda32_t report_id;
    tak_bertanda32_t report_type;
    tak_bertanda32_t usage_page;
    tak_bertanda32_t usage;
    tak_bertanda32_t bit_offset;
    tak_bertanda32_t bit_size;
    tanda32_t logical_min;
    tanda32_t logical_max;
    tak_bertanda32_t flags;
    tak_bertanda32_t count;
} hid_field_t;

/* Konteks parser HID */
typedef struct {
    inputdev_t *dev;
    
    /* State saat ini */
    hid_state_t state;
    hid_stack_t stack;
    
    /* Collections */
    hid_collection_t collections[HID_MAX_COLLECTIONS];
    tak_bertanda32_t collection_count;
    tak_bertanda32_t collection_depth;
    
    /* Fields */
    hid_field_t fields[HID_MAX_USAGES];
    tak_bertanda32_t field_count;
    
    /* Report info */
    tak_bertanda32_t report_sizes[HID_MAX_REPORTS][3];
    tak_bertanda32_t report_count;
    
    /* Status */
    bool_t diinisialisasi;
    bool_t parsing_ok;
} hid_konteks_t;

/*
 * ===========================================================================
 * VARIABEL LOKAL
 * ===========================================================================
 */

static hid_konteks_t g_hid_konteks;
static bool_t g_hid_diinisialisasi = SALAH;

/*
 * ===========================================================================
 * FUNGSI INTERNAL
 * ===========================================================================
 */

/*
 * hid_item_size - Dapatkan ukuran item HID
 */
static tak_bertanda32_t hid_item_size(tak_bertanda8_t item)
{
    tak_bertanda32_t ukuran;
    
    ukuran = item & 0x03;
    
    switch (ukuran) {
    case 0:
        return 0;
    case 1:
        return 1;
    case 2:
        return 2;
    case 3:
        return 4;
    default:
        return 0;
    }
}

/*
 * hid_signed_value - Konversi nilai unsigned ke signed
 */
static tanda32_t hid_signed_value(tak_bertanda32_t nilai, 
                                   tak_bertanda32_t bit_size)
{
    tak_bertanda32_t mask;
    tak_bertanda32_t sign_bit;
    
    if (bit_size == 0 || bit_size >= 32) {
        return (tanda32_t)nilai;
    }
    
    sign_bit = 1UL << (bit_size - 1);
    mask = (1UL << bit_size) - 1;
    
    nilai = nilai & mask;
    
    if (nilai & sign_bit) {
        nilai = nilai | (~mask);
    }
    
    return (tanda32_t)nilai;
}

/*
 * hid_read_value - Baca nilai dari descriptor
 */
static tak_bertanda32_t hid_read_value(const tak_bertanda8_t *data,
                                        tak_bertanda32_t offset,
                                        tak_bertanda32_t size)
{
    tak_bertanda32_t nilai;
    tak_bertanda32_t i;
    
    nilai = 0;
    
    for (i = 0; i < size; i++) {
        nilai |= ((tak_bertanda32_t)data[offset + i]) << (i * 8);
    }
    
    return nilai;
}

/*
 * hid_push_state - Push state ke stack
 */
static void hid_push_state(hid_konteks_t *ctx)
{
    if (ctx == NULL) {
        return;
    }
    
    if (ctx->stack.depth >= HID_STACK_DEPTH) {
        return;
    }
    
    ctx->stack.stack[ctx->stack.depth] = ctx->state;
    ctx->stack.depth++;
}

/*
 * hid_pop_state - Pop state dari stack
 */
static void hid_pop_state(hid_konteks_t *ctx)
{
    if (ctx == NULL) {
        return;
    }
    
    if (ctx->stack.depth == 0) {
        return;
    }
    
    ctx->stack.depth--;
    ctx->state = ctx->stack.stack[ctx->stack.depth];
}

/*
 * hid_reset_state - Reset state parser
 */
static void hid_reset_state(hid_state_t *state)
{
    if (state == NULL) {
        return;
    }
    
    state->usage_page = 0;
    state->usage = 0;
    state->logical_min = 0;
    state->logical_max = 0;
    state->physical_min = 0;
    state->physical_max = 0;
    state->unit = 0;
    state->unit_exponent = 0;
    state->report_size = 0;
    state->report_id = 0;
    state->report_count = 0;
    state->usage_min = 0;
    state->usage_max = 0;
}

/*
 * hid_add_field - Tambah field ke konteks
 */
static status_t hid_add_field(hid_konteks_t *ctx, tak_bertanda32_t tipe)
{
    hid_field_t *field;
    tak_bertanda32_t i;
    
    if (ctx == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (ctx->field_count >= HID_MAX_USAGES) {
        return STATUS_PENUH;
    }
    
    for (i = 0; i < ctx->state.report_count; i++) {
        if (ctx->field_count >= HID_MAX_USAGES) {
            break;
        }
        
        field = &ctx->fields[ctx->field_count];
        
        field->report_id = ctx->state.report_id;
        field->report_type = tipe;
        field->usage_page = ctx->state.usage_page;
        field->usage = ctx->state.usage_min + i;
        field->bit_size = ctx->state.report_size;
        field->logical_min = ctx->state.logical_min;
        field->logical_max = ctx->state.logical_max;
        field->count = ctx->state.report_count;
        
        ctx->field_count++;
    }
    
    return STATUS_BERHASIL;
}

/*
 * hid_add_collection - Tambah collection baru
 */
static status_t hid_add_collection(hid_konteks_t *ctx, tak_bertanda32_t tipe)
{
    hid_collection_t *coll;
    
    if (ctx == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (ctx->collection_count >= HID_MAX_COLLECTIONS) {
        return STATUS_PENUH;
    }
    
    coll = &ctx->collections[ctx->collection_count];
    
    coll->tipe = tipe;
    coll->usage_page = ctx->state.usage_page;
    coll->usage = ctx->state.usage;
    
    if (ctx->collection_depth > 0) {
        coll->parent_index = ctx->collection_count - 1;
    } else {
        coll->parent_index = INDEX_INVALID;
    }
    
    ctx->collection_count++;
    ctx->collection_depth++;
    
    return STATUS_BERHASIL;
}

/*
 * hid_end_collection - Akhiri collection
 */
static void hid_end_collection(hid_konteks_t *ctx)
{
    if (ctx == NULL) {
        return;
    }
    
    if (ctx->collection_depth > 0) {
        ctx->collection_depth--;
    }
}

/*
 * hid_detect_device_type - Deteksi tipe device dari usage
 */
static tak_bertanda32_t hid_detect_device_type(hid_konteks_t *ctx)
{
    tak_bertanda32_t i;
    hid_collection_t *coll;
    
    if (ctx == NULL) {
        return INPUT_TIPE_TIDAK_DIKETAHUI;
    }
    
    for (i = 0; i < ctx->collection_count; i++) {
        coll = &ctx->collections[i];
        
        if (coll->usage_page == HID_USAGE_PAGE_GENERIC_DESKTOP) {
            switch (coll->usage) {
            case HID_USAGE_KEYBOARD:
                return INPUT_TIPE_KEYBOARD;
            case HID_USAGE_MOUSE:
                return INPUT_TIPE_MOUSE;
            case HID_USAGE_JOYSTICK:
                return INPUT_TIPE_JOYSTICK;
            case HID_USAGE_GAMEPAD:
                return INPUT_TIPE_GAMEPAD;
            default:
                break;
            }
        }
        
        if (coll->usage_page == HID_USAGE_PAGE_DIGITIZER) {
            return INPUT_TIPE_TOUCHSCREEN;
        }
    }
    
    return INPUT_TIPE_TIDAK_DIKETAHUI;
}

/*
 * hid_count_capabilities - Hitung kapabilitas device
 */
static void hid_count_capabilities(hid_konteks_t *ctx)
{
    tak_bertanda32_t i;
    hid_field_t *field;
    
    if (ctx == NULL || ctx->dev == NULL) {
        return;
    }
    
    for (i = 0; i < ctx->field_count; i++) {
        field = &ctx->fields[i];
        
        if (field->usage_page == HID_USAGE_PAGE_KEYBOARD) {
            ctx->dev->num_keys += field->count;
        }
        
        if (field->usage_page == HID_USAGE_PAGE_BUTTON) {
            ctx->dev->num_buttons += field->count;
        }
        
        if (field->usage_page == HID_USAGE_PAGE_GENERIC_DESKTOP) {
            if (field->usage >= HID_USAGE_X && field->usage <= HID_USAGE_RZ) {
                ctx->dev->num_axes++;
            }
        }
    }
}

/*
 * hid_parse_item - Parse satu item HID
 */
static status_t hid_parse_item(hid_konteks_t *ctx, tak_bertanda8_t item,
                                const tak_bertanda8_t *data,
                                tak_bertanda32_t *offset)
{
    tak_bertanda32_t ukuran;
    tak_bertanda32_t nilai;
    tak_bertanda8_t tipe;
    tak_bertanda8_t tag;
    
    if (ctx == NULL || data == NULL || offset == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    ukuran = hid_item_size(item);
    tipe = item & 0x0C;
    tag = item & 0xF0;
    
    nilai = hid_read_value(data, *offset, ukuran);
    *offset += ukuran;
    
    switch (tipe) {
    case 0x00:  /* Main items */
        switch (tag) {
        case 0x80:  /* Input */
            hid_add_field(ctx, HID_REPORT_TYPE_INPUT);
            break;
        case 0x90:  /* Output */
            hid_add_field(ctx, HID_REPORT_TYPE_OUTPUT);
            break;
        case 0xB0:  /* Feature */
            hid_add_field(ctx, HID_REPORT_TYPE_FEATURE);
            break;
        case 0xA0:  /* Collection */
            hid_add_collection(ctx, nilai);
            break;
        case 0xC0:  /* End Collection */
            hid_end_collection(ctx);
            break;
        default:
            break;
        }
        break;
        
    case 0x04:  /* Global items */
        switch (tag) {
        case 0x00:  /* Usage Page */
            ctx->state.usage_page = nilai;
            break;
        case 0x10:  /* Logical Minimum */
            ctx->state.logical_min = (tanda32_t)nilai;
            break;
        case 0x20:  /* Logical Maximum */
            ctx->state.logical_max = (tanda32_t)nilai;
            break;
        case 0x30:  /* Physical Minimum */
            ctx->state.physical_min = (tanda32_t)nilai;
            break;
        case 0x40:  /* Physical Maximum */
            ctx->state.physical_max = (tanda32_t)nilai;
            break;
        case 0x50:  /* Unit Exponent */
            ctx->state.unit_exponent = nilai;
            break;
        case 0x60:  /* Unit */
            ctx->state.unit = nilai;
            break;
        case 0x70:  /* Report Size */
            ctx->state.report_size = nilai;
            break;
        case 0x80:  /* Report ID */
            ctx->state.report_id = nilai;
            break;
        case 0x90:  /* Report Count */
            ctx->state.report_count = nilai;
            break;
        case 0xA0:  /* Push */
            hid_push_state(ctx);
            break;
        case 0xB0:  /* Pop */
            hid_pop_state(ctx);
            break;
        default:
            break;
        }
        break;
        
    case 0x08:  /* Local items */
        switch (tag) {
        case 0x00:  /* Usage */
            if (ukuran <= 2) {
                ctx->state.usage = nilai;
            } else {
                ctx->state.usage_page = (nilai >> 16) & 0xFFFF;
                ctx->state.usage = nilai & 0xFFFF;
            }
            ctx->state.usage_min = ctx->state.usage;
            ctx->state.usage_max = ctx->state.usage;
            break;
        case 0x10:  /* Usage Minimum */
            ctx->state.usage_min = nilai;
            break;
        case 0x20:  /* Usage Maximum */
            ctx->state.usage_max = nilai;
            break;
        default:
            break;
        }
        break;
        
    default:
        break;
    }
    
    return STATUS_BERHASIL;
}

/*
 * hid_read_field - Baca nilai field dari report
 */
static tanda32_t hid_read_field(const tak_bertanda8_t *report,
                                 ukuran_t report_len,
                                 tak_bertanda32_t byte_offset,
                                 tak_bertanda32_t bit_offset,
                                 tak_bertanda32_t bit_size)
{
    tak_bertanda32_t nilai;
    tak_bertanda32_t mask;
    tak_bertanda32_t byte_idx;
    tak_bertanda32_t bit_idx;
    tak_bertanda32_t bits_read;
    tak_bertanda32_t shift;
    
    nilai = 0;
    bits_read = 0;
    byte_idx = byte_offset;
    bit_idx = bit_offset;
    
    while (bits_read < bit_size && byte_idx < report_len) {
        shift = bit_idx;
        
        if (bits_read + (8 - shift) <= bit_size) {
            nilai |= ((tak_bertanda32_t)report[byte_idx] >> shift) << bits_read;
            bits_read += (8 - shift);
            byte_idx++;
            bit_idx = 0;
        } else {
            mask = (1UL << (bit_size - bits_read)) - 1;
            nilai |= ((tak_bertanda32_t)(report[byte_idx] >> shift) & mask) 
                     << bits_read;
            bits_read = bit_size;
        }
    }
    
    return hid_signed_value(nilai, bit_size);
}

/*
 * ===========================================================================
 * FUNGSI PUBLIK
 * ===========================================================================
 */

/*
 * hid_init - Inisialisasi HID parser
 */
status_t hid_init(void)
{
    if (g_hid_diinisialisasi) {
        return STATUS_SUDAH_ADA;
    }
    
    kernel_memset(&g_hid_konteks, 0, sizeof(hid_konteks_t));
    
    hid_reset_state(&g_hid_konteks.state);
    g_hid_konteks.stack.depth = 0;
    g_hid_konteks.collection_count = 0;
    g_hid_konteks.collection_depth = 0;
    g_hid_konteks.field_count = 0;
    g_hid_konteks.report_count = 0;
    g_hid_konteks.diinisialisasi = BENAR;
    g_hid_konteks.parsing_ok = SALAH;
    
    g_hid_diinisialisasi = BENAR;
    
    return STATUS_BERHASIL;
}

/*
 * hid_parse - Parse HID report descriptor
 */
status_t hid_parse(inputdev_t *dev, const tak_bertanda8_t *descriptor,
                    ukuran_t len)
{
    hid_konteks_t *ctx;
    tak_bertanda32_t offset;
    status_t hasil;
    
    if (!g_hid_diinisialisasi) {
        hasil = hid_init();
        if (hasil != STATUS_BERHASIL) {
            return hasil;
        }
    }
    
    if (dev == NULL || descriptor == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (len == 0) {
        return STATUS_PARAM_UKURAN;
    }
    
    ctx = &g_hid_konteks;
    
    /* Reset konteks untuk parsing baru */
    ctx->dev = dev;
    ctx->collection_count = 0;
    ctx->collection_depth = 0;
    ctx->field_count = 0;
    ctx->report_count = 0;
    ctx->parsing_ok = SALAH;
    
    hid_reset_state(&ctx->state);
    ctx->stack.depth = 0;
    
    /* Parse descriptor */
    offset = 0;
    while (offset < len) {
        tak_bertanda8_t item;
        
        item = descriptor[offset];
        offset++;
        
        hasil = hid_parse_item(ctx, item, descriptor, &offset);
        if (hasil != STATUS_BERHASIL) {
            return hasil;
        }
    }
    
    /* Deteksi tipe device */
    dev->tipe = hid_detect_device_type(ctx);
    
    /* Hitung kapabilitas */
    hid_count_capabilities(ctx);
    
    ctx->parsing_ok = BENAR;
    
    return STATUS_BERHASIL;
}

/*
 * hid_process_report - Process HID report
 */
status_t hid_process_report(inputdev_t *dev, const tak_bertanda8_t *report,
                             ukuran_t len)
{
    hid_konteks_t *ctx;
    hid_field_t *field;
    input_event_t event;
    tak_bertanda32_t i;
    tak_bertanda32_t bit_offset;
    tanda32_t nilai;
    
    if (!g_hid_diinisialisasi) {
        return STATUS_GAGAL;
    }
    
    if (dev == NULL || report == NULL) {
        return STATUS_PARAM_NULL;
    }
    
    if (len == 0) {
        return STATUS_PARAM_UKURAN;
    }
    
    ctx = &g_hid_konteks;
    
    if (!ctx->parsing_ok) {
        return STATUS_GAGAL;
    }
    
    if (ctx->dev != dev) {
        return STATUS_PARAM_INVALID;
    }
    
    /* Inisialisasi event */
    kernel_memset(&event, 0, sizeof(input_event_t));
    event.magic = EVENT_MAGIC;
    event.timestamp = kernel_get_ticks();
    event.device_id = dev->id;
    
    bit_offset = 0;
    
    /* Proses setiap field */
    for (i = 0; i < ctx->field_count; i++) {
        field = &ctx->fields[i];
        
        if (field->report_type != HID_REPORT_TYPE_INPUT) {
            continue;
        }
        
        nilai = hid_read_field(report, len, bit_offset / 8, 
                               bit_offset % 8, field->bit_size);
        
        bit_offset += field->bit_size;
        
        /* Proses berdasarkan usage page */
        switch (field->usage_page) {
        case HID_USAGE_PAGE_KEYBOARD:
            /* Keyboard key */
            if (nilai > 0) {
                event.tipe = EVENT_TIPE_KEY;
                event.data.key.kode = field->usage;
                event.data.key.status = 1;
                event.data.key.scancode = nilai;
                
                masukan_event_push(&event);
                
                if (dev->callback != NULL) {
                    dev->callback(dev, &event);
                }
            }
            break;
            
        case HID_USAGE_PAGE_BUTTON:
            /* Button */
            event.tipe = EVENT_TIPE_KEY;
            event.data.key.kode = field->usage;
            event.data.key.status = (nilai > 0) ? 1 : 0;
            
            dev->button_state &= ~(1UL << (field->usage - 1));
            if (nilai > 0) {
                dev->button_state |= (1UL << (field->usage - 1));
            }
            
            masukan_event_push(&event);
            
            if (dev->callback != NULL) {
                dev->callback(dev, &event);
            }
            break;
            
        case HID_USAGE_PAGE_GENERIC_DESKTOP:
            /* Axis */
            switch (field->usage) {
            case HID_USAGE_X:
                dev->axis_value[JOY_AXIS_X] = nilai;
                break;
            case HID_USAGE_Y:
                dev->axis_value[JOY_AXIS_Y] = nilai;
                break;
            case HID_USAGE_Z:
                dev->axis_value[JOY_AXIS_Z] = nilai;
                break;
            case HID_USAGE_RX:
                dev->axis_value[JOY_AXIS_RX] = nilai;
                break;
            case HID_USAGE_RY:
                dev->axis_value[JOY_AXIS_RY] = nilai;
                break;
            case HID_USAGE_RZ:
                dev->axis_value[JOY_AXIS_RZ] = nilai;
                break;
            case HID_USAGE_WHEEL:
                event.tipe = EVENT_TIPE_RELATIVE;
                event.data.mouse.wheel = nilai;
                masukan_event_push(&event);
                break;
            default:
                break;
            }
            
            event.tipe = EVENT_TIPE_ABSOLUTE;
            masukan_event_push(&event);
            
            if (dev->callback != NULL) {
                dev->callback(dev, &event);
            }
            break;
            
        case HID_USAGE_PAGE_DIGITIZER:
            /* Touch */
            event.tipe = EVENT_TIPE_TOUCH;
            event.data.touch.id = field->usage - 1;
            event.data.touch.pressure = (tak_bertanda32_t)nilai;
            
            masukan_event_push(&event);
            
            if (dev->callback != NULL) {
                dev->callback(dev, &event);
            }
            break;
            
        default:
            break;
        }
    }
    
    return STATUS_BERHASIL;
}
