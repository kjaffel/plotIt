#pragma once
#include <string>

class CommandLineCfg {

    public:
        static CommandLineCfg& get() {
            static CommandLineCfg instance;
            return instance;
        }

        bool ignore_scales = false;
        bool verbose = false;
        bool do_plots = true;
        bool do_yields = false;
        bool unblind = false;
        bool systematicsBreakdown = false;
        std::string era = "";

    private:
        CommandLineCfg() = default;

};
