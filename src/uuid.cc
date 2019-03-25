#include <uuid.h>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

std::string get_uuid() {
    boost::uuids::random_generator gen;
    boost::uuids::uuid u = gen();

    return boost::uuids::to_string(u);
}
