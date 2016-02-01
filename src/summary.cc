#include <colors.h>
#include <summary.h>
#include <utilities.h>

#include <boost/format.hpp>

namespace plotIt {
    void Summary::add(const Type& type, const SummaryItem& item) {
        m_items[type].push_back(item);
    }

    void Summary::addSystematics(const Type& type, const SummaryItem& item) {
        m_systematics_items[type].push_back(item);
    }

    std::vector<SummaryItem> Summary::get(const Type& type) const {
        auto it = m_items.find(type);
        if (it == m_items.end())
            return std::vector<SummaryItem>();

        return it->second;
    }

    std::vector<SummaryItem> Summary::getSystematics(const Type& type) const {
        auto it = m_systematics_items.find(type);
        if (it == m_systematics_items.end())
            return std::vector<SummaryItem>();

        return it->second;
    }

    void ConsoleSummaryPrinter::print(const Summary& summary) const {
        printItems(DATA, summary);
        printItems(MC, summary);
        printItems(SIGNAL, summary);
    }

    void ConsoleSummaryPrinter::printItems(const Type& type, const Summary& summary) const {
        using namespace boost;

        std::vector<SummaryItem> nominal = summary.get(type);
        std::vector<SummaryItem> systematics = summary.getSystematics(type);

        float nominal_events = 0;
        float nominal_events_uncertainty = 0;

        if (nominal.size() == 0)
            return;

        std::cout << Color::FG_MAGENTA << type_to_string(type) << Color::RESET << std::endl;

        // Print header
        std::cout << format(u8"%|1$50|    %|1$9|N ± %|1$6|ΔN") % u8" ";
        if (type != DATA) {
            std::cout << format("    %|1$7|ε ± %|1$6|Δε") % " ";
        }
        std::cout << std::endl;

        for (const auto& n: nominal) {
            std::cout << Color::FG_YELLOW << format("%|50|") % truncate(n.name, 50) << Color::RESET << "    " << format("%|10.2f| ± %|8.2f|") % n.events % n.events_uncertainty;
            if (type != DATA) {
                std::cout << "    " << format("%|8.4f| ± %|8.4f|") % (n.efficiency * 100) % (n.efficiency_uncertainty * 100);
            }
            std::cout << std::endl;

            nominal_events += n.events;
            nominal_events_uncertainty += n.events_uncertainty * n.events_uncertainty;
        }

        if (systematics.size()) {
            // Print systematics
            std::cout << format("%|1$50|    ---------------------") % " " << std::endl;
            for (const auto& n: systematics) {
                std::cout << Color::FG_YELLOW << format("%|50|") % truncate(n.name, 50) << Color::RESET << "    " << format("           ± %|8.2f|") % n.events_uncertainty;
                if (type != DATA) {
                    std::cout << "    " << format("%|8.2f| %%") % ((n.events_uncertainty / nominal_events) * 100);
                }
                std::cout << std::endl;

                nominal_events_uncertainty += n.events_uncertainty * n.events_uncertainty;
            }
        }

        // Print total sum
        std::cout << format("%|1$50|    ---------------------") % " " << std::endl;
        std::cout << format("%|50t|    %|10.2f| ± %|8.2f|") % nominal_events % std::sqrt(nominal_events_uncertainty) << std::endl;
    }
}
