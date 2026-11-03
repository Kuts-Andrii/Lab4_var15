#pragma once

#include <cstddef>
#include <mutex>
#include <string>

constexpr std::size_t kFieldCount = 2;

class SharedRecord {
public:
    int read(std::size_t field) const {
        std::lock_guard<std::mutex> lock(fields_[field].mutex);
        return fields_[field].value;
    }

    void write(std::size_t field, int value) {
        std::lock_guard<std::mutex> lock(fields_[field].mutex);
        fields_[field].value = value;
    }

    explicit operator std::string() const {
        int value0, value1;
        {
            std::scoped_lock lock(fields_[0].mutex, fields_[1].mutex);
            value0 = fields_[0].value;
            value1 = fields_[1].value;
        }
        return std::to_string(value0) + " " + std::to_string(value1);
    }

private:
    struct alignas(64) Field {
        int value = 0;
        mutable std::mutex mutex;
    };

    Field fields_[kFieldCount];
};