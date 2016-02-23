#pragma once

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

    private:
        CommandLineCfg() = default;

};
