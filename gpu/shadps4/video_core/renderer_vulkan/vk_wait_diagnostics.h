// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <source_location>
#include <string_view>

namespace Vulkan {
class WaitDiagnostics {
public:
    struct Entry {
        std::string_view file{}, operation{};
        unsigned line{}, calls{};
        uint64_t operation_ns{}, wait_ns{}, max_operation_ns{}, max_wait_ns{};
    };
    void Add(std::string_view file, unsigned line, uint64_t operation_ns, uint64_t wait,
             std::string_view operation = "GPU timeline") {
        std::scoped_lock lock{mutex};
        auto* entry = &entries.back();
        for (unsigned i = 0; i + 1 < entries.size(); ++i) {
            if (!entries[i].calls || (entries[i].file == file && entries[i].line == line &&
                                      entries[i].operation == operation)) {
                entry = &entries[i];
                entry->file = file;
                entry->line = line;
                entry->operation = operation;
                break;
            }
        }
        if (entry == &entries.back()) {
            entry->file = "other wait sites";
            entry->operation = "other";
        }
        ++entry->calls;
        entry->operation_ns += operation_ns;
        entry->wait_ns += wait;
        entry->max_operation_ns = std::max(entry->max_operation_ns, operation_ns);
        entry->max_wait_ns = std::max(entry->max_wait_ns, wait);
    }
    std::array<Entry, 64> Take() {
        std::scoped_lock lock{mutex};
        const auto result = entries;
        entries = {};
        return result;
    }
    static bool Enabled() {
        static const bool enabled = [] {
            const char* value = std::getenv("TEMPEST_PERF_DIAG");
            return value && value[0] == '1';
        }();
        return enabled;
    }
    static WaitDiagnostics& Get() {
        static WaitDiagnostics stats;
        return stats;
    }
    enum class Kind { HostCall, Wait };
    // Host calls can block internally; only known synchronization waits use Kind::Wait.
    // Nested entries describe the same interval from different sites and are not additive.
    class Timer {
    public:
        explicit Timer(Kind kind, std::string_view operation,
                       std::source_location caller = std::source_location::current())
            : enabled{Enabled()}, kind{kind}, operation{operation}, caller{caller},
              start{enabled ? Clock::now() : Clock::time_point{}} {}
        ~Timer() { Finish(); }
        Timer(const Timer&) = delete;
        Timer& operator=(const Timer&) = delete;
        void Finish() {
            if (!enabled) return;
            const auto elapsed =
                std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - start).count();
            enabled = false;
            Get().Add(caller.file_name(), caller.line(), kind == Kind::HostCall ? elapsed : 0,
                      kind == Kind::Wait ? elapsed : 0, operation);
        }
    private:
        using Clock = std::chrono::steady_clock;
        bool enabled;
        Kind kind;
        std::string_view operation;
        std::source_location caller;
        Clock::time_point start;
    };
    static void Report() {
        if (!Enabled()) return;
        auto rows = Get().Take();
        std::sort(rows.begin(), rows.end(), [](const Entry& a, const Entry& b) {
            return a.operation_ns + a.wait_ns > b.operation_ns + b.wait_ns;
        });
        for (const auto& row : rows) {
            if (!row.calls) continue;
            const auto filename = row.file.substr(row.file.find_last_of("/\\") + 1);
            std::printf("Wait profile: %.*s:%u [%.*s]; %u calls, host call %.3f ms (max %.3f ms), wait %.3f ms (max %.3f ms)\n",
                        int(filename.size()), filename.data(), row.line,
                        int(row.operation.size()), row.operation.data(), row.calls,
                        row.operation_ns / 1e6, row.max_operation_ns / 1e6,
                        row.wait_ns / 1e6, row.max_wait_ns / 1e6);
        }
    }
private:
    std::mutex mutex;
    std::array<Entry, 64> entries{};
};
}
