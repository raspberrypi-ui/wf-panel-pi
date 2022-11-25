#include "../lxutils.h"

/*----------------------------------------------------------------------------*/
/* Plug-in global data                                                        */
/*----------------------------------------------------------------------------*/

#define MAX_NUM_SENSORS             10

typedef gint (*GetTempFunc) (char const *);

/* Private context for plugin */

typedef struct
{
    GdkRGBA foreground_color;			    /* Foreground colour for drawing area */
    GdkRGBA background_color;			    /* Background colour for drawing area */
    GdkRGBA low_throttle_color;			    /* Colour for bars with ARM freq cap */
    GdkRGBA high_throttle_color;			/* Colour for bars with throttling */
    GtkWidget *plugin;                      /* Back pointer to the widget */
    //LXPanel *panel;                         /* Back pointer to panel */
    int icon_size;                      /* Variables used under wf-panel */
    gboolean bottom;
    GtkWidget *da;				            /* Drawing area */
    cairo_surface_t *pixmap;				/* Pixmap to be drawn on drawing area */
    guint timer;				            /* Timer for periodic update */
    float *stats_cpu;			            /* Ring buffer of temperature values */
    int *stats_throttle;                    /* Ring buffer of throttle status */
    unsigned int ring_cursor;			    /* Cursor for ring buffer */
    guint pixmap_width;				        /* Width of drawing area pixmap; also size of ring buffer; does not include border size */
    guint pixmap_height;			        /* Height of drawing area pixmap; does not include border size */
    int lower_temp;                         /* Temperature of bottom of graph */
    int upper_temp;                         /* Temperature of top of graph */
    int numsensors;
    char *sensor_array[MAX_NUM_SENSORS];
    GetTempFunc get_temperature[MAX_NUM_SENSORS];
    gint temperature[MAX_NUM_SENSORS];
    //config_setting_t *settings;
    gboolean ispi;
} CPUTempPlugin;

extern void cputemp_init (CPUTempPlugin *up);
extern void cputemp_update_display (CPUTempPlugin *up);
extern gboolean cputemp_control_msg (CPUTempPlugin *up, const char *cmd);
