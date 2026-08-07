#include <widget.hpp>

class WidgetSpacing : public PanelWidget
{
    Gtk::HBox box;

  public:
    WidgetSpacing (int val);
    virtual void widget_init (Gtk::HBox *container);
    virtual ~WidgetSpacing () {}
};

extern "C" {
    PanelWidget *create (int val) { return new WidgetSpacing(val); }
    void destroy (PanelWidget *w) { delete w; }

    static constexpr conf_table_t conf_table[2] = {
        {CONF_TYPE_INT,     "width",    N_("Width in pixels"),  NULL,   "4"     },
        {CONF_TYPE_NONE,    NULL,       NULL,                   NULL,   NULL    }
    };

    const conf_table_t *config_params (void) { return conf_table; };
    const char *display_name (void) { return N_("Spacer"); };
    const char *package_name (void) { return GETTEXT_PACKAGE; };
}

WidgetSpacing::WidgetSpacing (int val)
{
    box.set_size_request (val, 1);
}

void WidgetSpacing::widget_init (Gtk::HBox *container)
{
    box.set_name ("spacing");
    container->pack_start (box, false, false);
    box.show_all ();
}
