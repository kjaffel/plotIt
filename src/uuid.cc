#include <uuid.h>

#include <uuid/uuid.h>

std::string get_uuid() {
    uuid_t out;
    uuid_generate(out);

    std::string uuid;
    uuid.resize(37);

    uuid_unparse(out, &uuid[0]);

    // Remove null terminator
    uuid.resize(36);

    return uuid;
}
