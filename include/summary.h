#pragma once

#include <types.h>

#include <map>
#include <string>
#include <vector>

namespace plotIt {

    struct SummaryItem {
        std::string name;
        size_t process_id;

        float events = 0;
        float events_uncertainty = 0;

        float efficiency = 0;
        float efficiency_uncertainty = 0;
    };

    class Summary {
        public:
            void add(const Type& type, const SummaryItem& item);
            void addSystematics(const Type& type, size_t process_id, const SummaryItem& item);

            std::vector<SummaryItem> get(const Type& type) const;
            std::vector<SummaryItem> getSystematics(const Type& type, size_t process_id) const;

        private:
            std::map<Type, std::vector<SummaryItem>> m_items;
            std::map<std::pair<Type, size_t>, std::vector<SummaryItem>> m_systematics_items;
    };

    class SummaryPrinter {
        public:
            virtual void print(const Summary& summary) const = 0;
    };

    class ConsoleSummaryPrinter: public SummaryPrinter {
        public:
            virtual void print(const Summary& summary) const override;

        private:
            void printItems(const Type& type, const Summary& summary, bool combineSystematics = true) const;
    };
}
