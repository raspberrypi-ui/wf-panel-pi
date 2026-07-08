#pragma once

#include "config/option-wrapper.hpp"
#include "panel-app.hpp"

/**
 * An implementation of wf::base_option_wrapper_t for wf-shell-app based
 * programs.
 */
template<class Type>
class WfOption : public wf::base_option_wrapper_t<Type>
{
  public:
    WfOption(const std::string& option_name)
    {
        this->load_option(option_name);
    }

  protected:
    std::shared_ptr<wf::config::option_base_t> load_raw_option(const std::string& name) override
    {
        return PanelApp::get_config_option (name);
    }
};
