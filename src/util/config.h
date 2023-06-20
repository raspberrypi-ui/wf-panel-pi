#ifndef CONFIG_H
#define CONFIG_H

#define COL_NAME    0
#define COL_ID      1
#define COL_INDEX   2

typedef enum {
    CONF_NONE,
    CONF_BOOL,
    CONF_INT,
    CONF_STRING,
    CONF_COLOUR
} CONF_TYPE;

typedef struct {
    const char *plugin;
    const char *name;
    CONF_TYPE type;
    const char *label;
} conf_table_t;

#endif
