#include <colors.h>
#include <commandlinecfg.h>
#include <summary.h>
#include <utilities.h>

#include <boost/format.hpp>

namespace plotIt {
    void Summary::add(const Type& type, const SummaryItem& item) {
        m_items[type].push_back(item);
    }

    void Summary::addSystematics(const Type& type, size_t process_id, const SummaryItem& item) {
        m_systematics_items[std::make_pair(type, process_id)].push_back(item);
    }

    std::vector<SummaryItem> Summary::get(const Type& type) const {
        auto it = m_items.find(type);
        if (it == m_items.end())
            return std::vector<SummaryItem>();

        return it->second;
    }

    std::vector<SummaryItem> Summary::getSystematics(const Type& type, size_t process_id) const {
        auto it = m_systematics_items.find(std::make_pair(type, process_id));
        if (it == m_systematics_items.end())
            return std::vector<SummaryItem>();

        return it->second;
    }

    void ConsoleSummaryPrinter::print(const Summary& summary) const {
        printItems(DATA, summary);
        printItems(MC, summary, !CommandLineCfg::get().systematicsBreakdown);
        printItems(SIGNAL, summary, false);
    }

    void ConsoleSummaryPrinter::printItems(const Type& type, const Summary& summary, bool combineSystematics/* = true*/) const {
        using namespace boost;

        std::vector<SummaryItem> nominal = summary.get(type);
        std::map<std::string, float> systematics;

        float nominal_events = 0;
        float nominal_events_uncertainty = 0;

        if (nominal.size() == 0)
            return;

        std::cout << Color::FG_MAGENTA << type_to_string(type) << Color::RESET << std::endl;

        // Print header
        std::cout << format(u8"%|1$50|    %|1$9|N ± %|1$6|ΔN") % u8" ";
        //if (type != DATA) {
            //std::cout << format("    %|1$7|ε ± %|1$6|Δε") % " ";
        //}
        std::cout << std::endl;

        for (size_t i = 0; i < nominal.size(); i++) {
            const auto& n = nominal[i];

            std::cout << Color::FG_YELLOW << format("%|50|") % truncate(n.name, 50) << Color::RESET << "    " << format("%|10.2f| ± %|8.2f|") % n.events % n.events_uncertainty;
            //if (type != DATA) {
                //std::cout << "    " << format("%|8.4f| ± %|8.4f|") % (n.efficiency * 100) % (n.efficiency_uncertainty * 100);
            //}
            std::cout << std::endl;

            nominal_events += n.events;
            nominal_events_uncertainty += n.events_uncertainty * n.events_uncertainty;

            // Systematics
            // Systematics with same name are correlated between process, so simply sum
            // the number of events
            auto systematics_item = summary.getSystematics(type, n.process_id);
            for (const auto& item: systematics_item) {
                systematics[item.name] += item.events_uncertainty;
            }

            if (!combineSystematics && systematics_item.size()) {
                // Print systematics
                std::cout << format("%|1$50|    ---------------------") % " " << std::endl;
                float process_systematics_uncertainty = 0;
                for (const auto& s: systematics_item) {
                    std::cout << Color::FG_YELLOW << format("%|50|") % truncate(s.name, 50) << Color::RESET << "    " << format("           ± %|8.2f|") % s.events_uncertainty;
                    if (type != DATA) {
                        std::cout << "    " << format("%|8.2f| %%") % ((s.events_uncertainty / n.events) * 100);
                    }
                    std::cout << std::endl;

                    process_systematics_uncertainty += s.events_uncertainty * s.events_uncertainty;
                }

                std::cout << format("%|1$50|    ---------------------") % " " << std::endl;
                std::cout << format("%|50t|    %|10.2f| ± %|8.2f|") % n.events % std::sqrt(n.events_uncertainty * n.events_uncertainty + process_systematics_uncertainty) << std::endl;

                if (i != (nominal.size() - 1))
                    std::cout << std::endl;
                    //std::cout << format("%|1$50|    ---------------------") % " " << std::endl;
            }
        }

        if (type == MC && systematics.size()) {
            // Print systematics summary
            std::cout << std::endl << Color::FG_GREEN << format("%|51|") % (type_to_string(type) + " total (± stat.)") << Color::RESET << format("    %|10.2f| ± %|8.2f|") % nominal_events % std::sqrt(nominal_events_uncertainty) << std::endl;
            std::cout << format("%|1$50|") % " " << "    ---------------------" << std::endl;
            for (const auto& n: systematics) {
                std::cout << Color::FG_YELLOW << format("%|50|") % truncate(n.first, 50) << Color::RESET << "    " << format("           ± %|8.2f|") % n.second;
                if (type != DATA) {
                    std::cout << "    " << format("%|8.2f| %%") % ((n.second / nominal_events) * 100);
                }
                std::cout << std::endl;

                nominal_events_uncertainty += n.second * n.second;
            }
        }

        if (type == SIGNAL)
            return;

        // Print total sum
        std::cout << format("%|1$50|    ---------------------") % " " << std::endl;

        std::cout << Color::FG_GREEN;

        if (!systematics.empty())
            std::cout << format("%|51|") % (type_to_string(type) + " total (± stat. + syst.)");
        else
            std::cout << format("%|51|") % (type_to_string(type) + " total (± stat.)");

        std::cout << Color::RESET << format("    %|10.2f| ± %|8.2f|") % nominal_events % std::sqrt(nominal_events_uncertainty) << std::endl;
    }
}
