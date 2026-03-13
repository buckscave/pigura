#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

/* Konstanta untuk konfigurasi */
#define CONFIG_MAX_SECTIONS 32
#define CONFIG_MAX_KEYS 256
#define CONFIG_MAX_LINE_LENGTH 1024
#define CONFIG_MAX_PATH_LENGTH 256
#define CONFIG_MAX_VALUE_LENGTH 512
#define CONFIG_DEFAULT_FILE "config.ini"
#define CONFIG_BACKUP_EXT ".bak"

/* Tipe konfigurasi */
typedef enum {
    CONFIG_TYPE_STRING,
    CONFIG_TYPE_INT,
    CONFIG_TYPE_BOOL,
    CONFIG_TYPE_FLOAT,
    CONFIG_TYPE_COLOR,
    CONFIG_TYPE_UNKNOWN
} ConfigType;

/* Struktur warna RGB */
typedef struct {
    unsigned char r, g, b;
} ConfigColor;

/* Struktur nilai konfigurasi */
typedef struct {
    char key[CONFIG_MAX_VALUE_LENGTH];
    char value[CONFIG_MAX_VALUE_LENGTH];
    ConfigType type;
    int modified;
} ConfigValue;

/* Struktur section konfigurasi */
typedef struct {
    char name[CONFIG_MAX_VALUE_LENGTH];
    ConfigValue values[CONFIG_MAX_KEYS];
    int key_count;
} ConfigSection;

/* Struktur manager konfigurasi */
typedef struct {
    ConfigSection sections[CONFIG_MAX_SECTIONS];
    int section_count;
    char filename[CONFIG_MAX_PATH_LENGTH];
    int modified;
    int auto_save;
    int create_backup;
} ConfigManager;

/* Fungsi utilitas */
static int config_is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static int config_is_valid_key_char(char c) {
    return isalnum(c) || c == '_' || c == '-';
}

static int config_is_valid_section_char(char c) {
    return isalnum(c) || c == '_' || c == '-' || c == '.';
}

static char* config_trim(char *str) {
    char *start = str;
    char *end = str + strlen(str) - 1;
    
    /* Trim leading whitespace */
    while (start <= end && config_is_whitespace(*start)) {
        start++;
    }
    
    /* Trim trailing whitespace */
    while (end >= start && config_is_whitespace(*end)) {
        end--;
    }
    
    /* Null-terminate */
    *(end + 1) = '\0';
    
    return start;
}

static int config_string_to_bool(const char *str) {
    return (strcasecmp(str, "true") == 0 || 
            strcasecmp(str, "yes") == 0 || 
            strcasecmp(str, "1") == 0 ||
            strcasecmp(str, "on") == 0);
}

static int config_string_to_color(const char *str, ConfigColor *color) {
    /* Format: #RRGGBB */
    if (str[0] == '#' && strlen(str) == 7) {
        unsigned int r, g, b;
        if (sscanf(str + 1, "%2x%2x%2x", &r, &g, &b) == 3) {
            color->r = (unsigned char)r;
            color->g = (unsigned char)g;
            color->b = (unsigned char)b;
            return 1;
        }
    }
    
    /* Format: rgb(R,G,B) */
    if (strncmp(str, "rgb(", 4) == 0 && str[strlen(str) - 1] == ')') {
        unsigned int r, g, b;
        if (sscanf(str + 4, "%u,%u,%u", &r, &g, &b) == 3) {
            color->r = (unsigned char)r;
            color->g = (unsigned char)g;
            color->b = (unsigned char)b;
            return 1;
        }
    }
    
    /* Format: nama warna */
    if (strcasecmp(str, "black") == 0) {
        color->r = 0; color->g = 0; color->b = 0;
        return 1;
    } else if (strcasecmp(str, "white") == 0) {
        color->r = 255; color->g = 255; color->b = 255;
        return 1;
    } else if (strcasecmp(str, "red") == 0) {
        color->r = 255; color->g = 0; color->b = 0;
        return 1;
    } else if (strcasecmp(str, "green") == 0) {
        color->r = 0; color->g = 255; color->b = 0;
        return 1;
    } else if (strcasecmp(str, "blue") == 0) {
        color->r = 0; color->g = 0; color->b = 255;
        return 1;
    }
    
    return 0;
}

static void config_color_to_string(const ConfigColor *color, char *str, int max_len) {
    snprintf(str, max_len, "#%02X%02X%02X", color->r, color->g, color->b);
}

/* Inisialisasi konfigurasi manager */
static int config_init(ConfigManager *cm, const char *filename) {
    int i, j;
    
    /* Inisialisasi manager */
    cm->section_count = 0;
    cm->modified = 0;
    cm->auto_save = 1;
    cm->create_backup = 1;
    
    /* Set filename */
    if (filename) {
        strncpy(cm->filename, filename, CONFIG_MAX_PATH_LENGTH - 1);
        cm->filename[CONFIG_MAX_PATH_LENGTH - 1] = '\0';
    } else {
        strcpy(cm->filename, CONFIG_DEFAULT_FILE);
    }
    
    /* Inisialisasi section */
    for (i = 0; i < CONFIG_MAX_SECTIONS; i++) {
        strcpy(cm->sections[i].name, "");
        cm->sections[i].key_count = 0;
        
        for (j = 0; j < CONFIG_MAX_KEYS; j++) {
            strcpy(cm->sections[i].values[j].key, "");
            strcpy(cm->sections[i].values[j].value, "");
            cm->sections[i].values[j].type = CONFIG_TYPE_UNKNOWN;
            cm->sections[i].values[j].modified = 0;
        }
    }
    
    return 1;
}

/* Cari section berdasarkan nama */
static ConfigSection* config_find_section(ConfigManager *cm, const char *section_name) {
    int i;
    
    for (i = 0; i < cm->section_count; i++) {
        if (strcmp(cm->sections[i].name, section_name) == 0) {
            return &cm->sections[i];
        }
    }
    
    return NULL;
}

/* Tambah section baru */
static ConfigSection* config_add_section(ConfigManager *cm, const char *section_name) {
    if (cm->section_count >= CONFIG_MAX_SECTIONS) {
        return NULL;
    }
    
    ConfigSection *section = &cm->sections[cm->section_count];
    strncpy(section->name, section_name, CONFIG_MAX_VALUE_LENGTH - 1);
    section->name[CONFIG_MAX_VALUE_LENGTH - 1] = '\0';
    section->key_count = 0;
    
    cm->section_count++;
    cm->modified = 1;
    
    return section;
}

/* Cari key dalam section */
static ConfigValue* config_find_key(ConfigSection *section, const char *key) {
    int i;
    
    for (i = 0; i < section->key_count; i++) {
        if (strcmp(section->values[i].key, key) == 0) {
            return &section->values[i];
        }
    }
    
    return NULL;
}

/* Tambah key baru ke section */
static ConfigValue* config_add_key(ConfigSection *section, const char *key, 
                                 const char *value, ConfigType type) {
    if (section->key_count >= CONFIG_MAX_KEYS) {
        return NULL;
    }
    
    ConfigValue *cv = &section->values[section->key_count];
    strncpy(cv->key, key, CONFIG_MAX_VALUE_LENGTH - 1);
    cv->key[CONFIG_MAX_VALUE_LENGTH - 1] = '\0';
    strncpy(cv->value, value, CONFIG_MAX_VALUE_LENGTH - 1);
    cv->value[CONFIG_MAX_VALUE_LENGTH - 1] = '\0';
    cv->type = type;
    cv->modified = 1;
    
    section->key_count++;
    
    return cv;
}

/* Parse file konfigurasi */
static int config_parse_file(ConfigManager *cm, const char *filename) {
    FILE *file;
    char line[CONFIG_MAX_LINE_LENGTH];
    ConfigSection *current_section = NULL;
    int line_num = 0;
    
    file = fopen(filename, "r");
    if (!file) {
        return 0;
    }
    
    while (fgets(line, sizeof(line), file)) {
        line_num++;
        
        /* Trim whitespace */
        char *trimmed = config_trim(line);
        
        /* Skip empty lines and comments */
        if (trimmed[0] == '\0' || trimmed[0] == '#') {
            continue;
        }
        
        /* Parse section */
        if (trimmed[0] == '[' && trimmed[strlen(trimmed) - 1] == ']') {
            /* Extract section name */
            trimmed[strlen(trimmed) - 1] = '\0';
            char *section_name = trimmed + 1;
            
            /* Validate section name */
            int valid = 1;
            for (char *p = section_name; *p; p++) {
                if (!config_is_valid_section_char(*p)) {
                    valid = 0;
                    break;
                }
            }
            
            if (valid) {
                current_section = config_find_section(cm, section_name);
                if (!current_section) {
                    current_section = config_add_section(cm, section_name);
                }
            }
            continue;
        }
        
        /* Parse key=value */
        char *equal = strchr(trimmed, '=');
        if (equal && current_section) {
            /* Split key and value */
            *equal = '\0';
            char *key = config_trim(trimmed);
            char *value = config_trim(equal + 1);
            
            /* Validate key name */
            int valid = 1;
            for (char *p = key; *p; p++) {
                if (!config_is_valid_key_char(*p)) {
                    valid = 0;
                    break;
                }
            }
            
            if (valid && strlen(key) > 0) {
                ConfigValue *cv = config_find_key(current_section, key);
                if (!cv) {
                    /* Determine type based on value */
                    ConfigType type = CONFIG_TYPE_STRING;
                    if (config_string_to_bool(value) || 
                        strcmp(value, "0") == 0 || strcmp(value, "1") == 0) {
                        type = CONFIG_TYPE_BOOL;
                    } else {
                        char *endptr;
                        strtol(value, &endptr, 10);
                        if (*endptr == '\0') {
                            type = CONFIG_TYPE_INT;
                        } else {
                            strtof(value, &endptr);
                            if (*endptr == '\0') {
                                type = CONFIG_TYPE_FLOAT;
                            }
                        }
                    }
                    
                    cv = config_add_key(current_section, key, value, type);
                } else {
                    strcpy(cv->value, value);
                    cv->modified = 0;
                }
            }
        }
    }
    
    fclose(file);
    return 1;
}

/* Muat konfigurasi dari file */
static int config_load(ConfigManager *cm, const char *filename) {
    if (filename) {
        strncpy(cm->filename, filename, CONFIG_MAX_PATH_LENGTH - 1);
        cm->filename[CONFIG_MAX_PATH_LENGTH - 1] = '\0';
    }
    
    /* Clear existing configuration */
    cm->section_count = 0;
    
    /* Parse file */
    if (!config_parse_file(cm, cm->filename)) {
        return 0;
    }
    
    cm->modified = 0;
    return 1;
}

/* Simpan konfigurasi ke file */
static int config_save(ConfigManager *cm, const char *filename) {
    FILE *file;
    char backup_filename[CONFIG_MAX_PATH_LENGTH];
    int i, j;
    
    if (filename) {
        strncpy(cm->filename, filename, CONFIG_MAX_PATH_LENGTH - 1);
        cm->filename[CONFIG_MAX_PATH_LENGTH - 1] = '\0';
    }
    
    /* Create backup if enabled */
    if (cm->create_backup) {
        strcpy(backup_filename, cm->filename);
        strcat(backup_filename, CONFIG_BACKUP_EXT);
        rename(cm->filename, backup_filename);
    }
    
    file = fopen(cm->filename, "w");
    if (!file) {
        return 0;
    }
    
    /* Write sections */
    for (i = 0; i < cm->section_count; i++) {
        ConfigSection *section = &cm->sections[i];
        
        /* Write section header */
        fprintf(file, "[%s]\n", section->name);
        
        /* Write key-value pairs */
        for (j = 0; j < section->key_count; j++) {
            ConfigValue *cv = &section->values[j];
            fprintf(file, "%s = %s\n", cv->key, cv->value);
        }
        
        /* Empty line between sections */
        if (i < cm->section_count - 1) {
            fprintf(file, "\n");
        }
    }
    
    fclose(file);
    cm->modified = 0;
    return 1;
}

/* Get string value */
static const char* config_get_string(ConfigManager *cm, const char *section, 
                                     const char *key, const char *default_value) {
    ConfigSection *cs = config_find_section(cm, section);
    if (!cs) {
        return default_value;
    }
    
    ConfigValue *cv = config_find_key(cs, key);
    if (!cv) {
        return default_value;
    }
    
    return cv->value;
}

/* Get integer value */
static int config_get_int(ConfigManager *cm, const char *section, 
                          const char *key, int default_value) {
    const char *str = config_get_string(cm, section, key, NULL);
    if (!str) {
        return default_value;
    }
    
    char *endptr;
    long value = strtol(str, &endptr, 10);
    if (*endptr != '\0') {
        return default_value;
    }
    
    return (int)value;
}

/* Get boolean value */
static int config_get_bool(ConfigManager *cm, const char *section, 
                           const char *key, int default_value) {
    const char *str = config_get_string(cm, section, key, NULL);
    if (!str) {
        return default_value;
    }
    
    return config_string_to_bool(str);
}

/* Get float value */
static float config_get_float(ConfigManager *cm, const char *section, 
                              const char *key, float default_value) {
    const char *str = config_get_string(cm, section, key, NULL);
    if (!str) {
        return default_value;
    }
    
    char *endptr;
    float value = strtof(str, &endptr);
    if (*endptr != '\0') {
        return default_value;
    }
    
    return value;
}

/* Get color value */
static int config_get_color(ConfigManager *cm, const char *section, 
                            const char *key, ConfigColor *default_color) {
    const char *str = config_get_string(cm, section, key, NULL);
    if (!str || !default_color) {
        return 0;
    }
    
    return config_string_to_color(str, default_color);
}

/* Set string value */
static int config_set_string(ConfigManager *cm, const char *section, 
                             const char *key, const char *value) {
    ConfigSection *cs = config_find_section(cm, section);
    if (!cs) {
        cs = config_add_section(cm, section);
        if (!cs) {
            return 0;
        }
    }
    
    ConfigValue *cv = config_find_key(cs, key);
    if (!cv) {
        cv = config_add_key(cs, key, value, CONFIG_TYPE_STRING);
        if (!cv) {
            return 0;
        }
    } else {
        strncpy(cv->value, value, CONFIG_MAX_VALUE_LENGTH - 1);
        cv->value[CONFIG_MAX_VALUE_LENGTH - 1] = '\0';
        cv->type = CONFIG_TYPE_STRING;
        cv->modified = 1;
    }
    
    cm->modified = 1;
    
    /* Auto-save if enabled */
    if (cm->auto_save) {
        config_save(cm, NULL);
    }
    
    return 1;
}

/* Set integer value */
static int config_set_int(ConfigManager *cm, const char *section, 
                          const char *key, int value) {
    char str[32];
    snprintf(str, sizeof(str), "%d", value);
    return config_set_string(cm, section, key, str);
}

/* Set boolean value */
static int config_set_bool(ConfigManager *cm, const char *section, 
                           const char *key, int value) {
    return config_set_string(cm, section, key, value ? "true" : "false");
}

/* Set float value */
static int config_set_float(ConfigManager *cm, const char *section, 
                              const char *key, float value) {
    char str[32];
    snprintf(str, sizeof(str), "%f", value);
    return config_set_string(cm, section, key, str);
}

/* Set color value */
static int config_set_color(ConfigManager *cm, const char *section, 
                            const char *key, const ConfigColor *color) {
    char str[16];
    config_color_to_string(color, str, sizeof(str));
    return config_set_string(cm, section, key, str);
}

/* Set auto-save */
static void config_set_auto_save(ConfigManager *cm, int auto_save) {
    cm->auto_save = auto_save;
}

/* Set create backup */
static void config_set_create_backup(ConfigManager *cm, int create_backup) {
    cm->create_backup = create_backup;
}

/* Check if configuration is modified */
static int config_is_modified(ConfigManager *cm) {
    return cm->modified;
}

/* Get configuration filename */
static const char* config_get_filename(ConfigManager *cm) {
    return cm->filename;
}

/* Free configuration manager */
static void config_free(ConfigManager *cm) {
    /* Nothing to free, all memory is statically allocated */
    cm->section_count = 0;
    cm->modified = 0;
}

#endif /* CONFIG_H */
