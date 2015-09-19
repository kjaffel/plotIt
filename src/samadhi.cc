#include <samadhi/Database.h>
#include <samadhi/Tables.h>

#include <sqlpp11/mysql/mysql.h>

#include <iostream>

namespace mysql = sqlpp::mysql;

std::unique_ptr<Database> Database::m_instance;
std::once_flag Database::m_once_flag;

Database::Database() {

}

Database::~Database() {

}

Database& Database::get() {
    std::call_once(m_once_flag, [] { m_instance.reset(new Database()); });
    return *m_instance.get();
}

bool Database::connected() {
    return m_connection.get() != nullptr;
}

void Database::connect(std::shared_ptr<sqlpp::mysql::connection_config> config) {
    m_connection = std::make_shared<mysql::connection>(config); 
}

float Database::get_xsection(const std::string& sample_name) {

    assert_connected();

    mysql::connection& db = *m_connection;

    dataset::dataset dataset;
    sample::sample sample;

    auto query = select(dataset.xsection).from(sample.join(dataset).on(sample.source_dataset_id == dataset.dataset_id)).where(sample.name == sample_name);

    const auto& result = db(query);
    if (result.empty()) {
        throw std::invalid_argument("Sample '" + sample_name + "' not found in the database");
    }

    return result.front().xsection;
}

float Database::get_normalization(const std::string& sample_name) {

    assert_connected();

    mysql::connection& db = *m_connection;

    sample::sample sample;

    auto query = select(sample.normalization).from(sample).where(sample.name == sample_name);

    const auto& result = db(query);
    if (result.empty()) {
        throw std::invalid_argument("Sample '" + sample_name + "' not found in the database");
    }

    return result.front().normalization;
}
