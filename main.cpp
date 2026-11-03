#include <chrono>
#include <cstddef>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "shared_record.h"
#include "workload.h"

namespace {
constexpr std::size_t kTotalOperations = 6'000'000;
constexpr std::size_t kPartCount = 6;
constexpr std::size_t kOperationsPerPart = kTotalOperations / kPartCount;
constexpr std::size_t kMaxThreads = 3;
constexpr std::size_t kRepeats = 5;
const std::string kWorkloadDir = "workloads";

const std::vector<Distribution> kDistributions = {
    {"a_matching", {10, 5, 50, 10, 25}},
    {"b_equal", {20, 20, 20, 20, 20}},
    {"c_mismatch", {1, 90, 1, 3, 5}},
};

using ThreadData = std::vector<std::vector<Operation>>;

std::string partPath(const Distribution& distribution, std::size_t part) {
    return kWorkloadDir + "/" + distribution.name + "_part" + std::to_string(part) + ".txt";
}

void generateAllFiles() {
    std::filesystem::create_directories(kWorkloadDir);
    std::mt19937 rng(15);
    for (const auto& distribution : kDistributions) {
        for (std::size_t part = 0; part < kPartCount; ++part) {
            generateFile(partPath(distribution, part), distribution, kOperationsPerPart, rng);
        }
    }
}

ThreadData loadParts(const Distribution& distribution) {
    ThreadData parts;
    for (std::size_t part = 0; part < kPartCount; ++part) {
        parts.push_back(loadFile(partPath(distribution, part)));
    }
    return parts;
}

ThreadData splitBetweenThreads(const ThreadData& parts, std::size_t threadCount) {
    ThreadData perThread(threadCount);
    const std::size_t partsPerThread = parts.size() / threadCount;
    for (std::size_t p = 0; p < parts.size(); ++p) {
        auto& target = perThread[p / partsPerThread];
        target.insert(target.end(), parts[p].begin(), parts[p].end());
    }
    return perThread;
}

double runOnce(const ThreadData& perThread, std::size_t& checksum) {
    SharedRecord record;
    std::vector<std::size_t> results(perThread.size(), 0);
    const auto start = std::chrono::steady_clock::now();
    {
        std::vector<std::jthread> threads;
        for (std::size_t t = 0; t < perThread.size(); ++t) {
            threads.emplace_back([&, t] { results[t] = execute(record, perThread[t]); });
        }
    }
    const auto finish = std::chrono::steady_clock::now();

    for (std::size_t value : results) checksum += value;
    return std::chrono::duration<double>(finish - start).count();
}

double averageTime(const ThreadData& perThread, std::size_t& checksum) {
    double total = 0;
    for (std::size_t i = 0; i < kRepeats; ++i) {
        total += runOnce(perThread, checksum);
    }
    return total / kRepeats;
}

}

int main() {
#ifdef NDEBUG
    std::cout << "Build: Release\n";
#else
    std::cout << "Build: Debug (timings are not valid)\n";
#endif
    std::cout << "Generating workload files...\n";
    generateAllFiles();

    std::cout << "\nAverage time of " << kRepeats << " runs, seconds\n"
              << std::left << std::setw(14) << "file" << std::right;
    for (std::size_t threads = 1; threads <= kMaxThreads; ++threads) {
        std::cout << std::setw(10) << std::to_string(threads) + " thr";
    }
    std::cout << '\n' << std::fixed << std::setprecision(4);

    std::size_t checksum = 0;
    for (const auto& distribution : kDistributions) {
        const ThreadData parts = loadParts(distribution);
        std::cout << std::left << std::setw(14) << distribution.name << std::right;
        for (std::size_t threads = 1; threads <= kMaxThreads; ++threads) {
            const ThreadData perThread = splitBetweenThreads(parts, threads);
            std::cout << std::setw(10) << averageTime(perThread, checksum) << std::flush;
        }
        std::cout << '\n';
    }
    std::cout << "\nchecksum: " << checksum << '\n';
}