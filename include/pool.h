#pragma once

#include <memory>
#include <vector>

#include <TObject.h>

namespace plotIt {
    class TemporaryPool {
        public:
            static TemporaryPool& get() {
                static TemporaryPool s_instance;

                return s_instance;
            }

            void add(const std::shared_ptr<TObject>& object) {
                m_temporaryObjects.push_back(object);
            }

            void addRuntime(const std::shared_ptr<TObject>& object) {
                m_temporaryObjectsRuntime.push_back(object);
            }

            void clear() {
                m_temporaryObjects.clear();
            }

            TemporaryPool(TemporaryPool const&) = delete;             // Copy construct
            TemporaryPool(TemporaryPool&&) = delete;                  // Move construct
            TemporaryPool& operator=(TemporaryPool const&) = delete;  // Copy assign
            TemporaryPool& operator=(TemporaryPool &&) = delete;      // Move assign

        protected:
            TemporaryPool() = default;

        private:
            std::vector<std::shared_ptr<TObject>> m_temporaryObjects;
            std::vector<std::shared_ptr<TObject>> m_temporaryObjectsRuntime;
    };
}
