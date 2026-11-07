#pragma once

#define ENUM_OSTREAM_CASE(name) \
        case name: \
            return os << #name;
