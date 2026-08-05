#include <widget.hpp>

class WidgetSplit : public PanelWidget
{
    Gtk::HBox box;

  public:
    WidgetSplit (void);
    virtual void init (Gtk::HBox *container);
    virtual ~WidgetSplit () {}
};

extern "C" {
    PanelWidget *create () { return new WidgetSplit; }
    void destroy (PanelWidget *w) { delete w; }

    static constexpr conf_table_t conf_table[1] = {
        {CONF_TYPE_NONE,    NULL,   NULL,   NULL,   NULL }
    };

    const conf_table_t *config_params (void) { return conf_table; };
    const char *display_name (void) { return N_("Tray Split"); };
    const char *package_name (void) { return GETTEXT_PACKAGE; };
}


WidgetSplit::WidgetSplit (void)
{
}

void WidgetSplit::init (Gtk::HBox *container)
{
    box.set_name ("split");
    container->pack_start (box, false, false);
    box.show_all ();
}
