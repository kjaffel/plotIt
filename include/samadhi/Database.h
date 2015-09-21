#pragma once

#include <memory>
#include <mutex>

#include <sqlpp11/mysql/mysql.h>

class Database {

    public:
        virtual ~Database();
        static Database& get();

        void connect(std::shared_ptr<sqlpp::mysql::connection_config> config);
        bool connected();

        float get_xsection(const std::string& sample_name);

        float get_normalization(const std::string& sample_name);

    private:
        static std::unique_ptr<Database> m_instance;
        static std::once_flag m_once_flag;

        std::shared_ptr<sqlpp::mysql::connection> m_connection;

        Database();
        Database(const Database& src) = delete;
        Database& operator=(Database& rhs) = delete;

        void assert_connected() {
            if (! connected())
                throw std::logic_error("Database not connected");
        }

};
