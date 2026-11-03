#pragma once

#include <array>
#include <cstddef>
#include <fstream>
#include <random>
#include <string>
#include <vector>

#include "shared_record.h"

enum class OperationType { Read, Write, String };

struct Operation {
    OperationType type;
    std::size_t field;
    int value;
};

constexpr std::size_t kKindCount = 2 * kFieldCount + 1;
constexpr std::size_t kStringKind = 2 * kFieldCount;

struct Distribution {
    std::string name;
    std::array<double, kKindCount> percents;
};

inline void generateFile(const std::string& path, const Distribution& dist, std::size_t count, std::mt19937& rng) {
    std::discrete_distribution<std::size_t> pickKind(dist.percents.begin(), dist.percents.end());
    std::ofstream out(path);
    for (std::size_t i = 0; i < count; ++i) {
        const std::size_t kind = pickKind(rng);
        if (kind == kStringKind) {
            out << "string\n";
        } else if (kind % 2 == 0) {
            out << "read " << kind / 2 << '\n';
        } else {
            out << "write " << kind / 2 << " 1\n";
        }
    }
}

inline std::vector<Operation> loadFile(const std::string& path) {
    std::vector<Operation> operations;
    std::ifstream in(path);
    std::string command;
    while (in >> command) {
        if (command == "string") {
            operations.push_back({OperationType::String, 0, 0});
        } else if (command == "read") {
            std::size_t field;
            in >> field;
            operations.push_back({OperationType::Read, field, 0});
        } else if (command == "write") {
            std::size_t field;
            int value;
            in >> field >> value;
            operations.push_back({OperationType::Write, field, value});
        }
    }
    return operations;
}

inline std::size_t execute(SharedRecord& record, const std::vector<Operation>& operations) {
    std::size_t checksum = 0;
    for (const Operation& op : operations) {
        switch (op.type) {
            case OperationType::Read:
                checksum += static_cast<std::size_t>(record.read(op.field));
                break;
            case OperationType::Write:
                record.write(op.field, op.value);
                break;
            case OperationType::String:
                checksum += static_cast<std::string>(record).size();
                break;
        }
    }
    return checksum;
}