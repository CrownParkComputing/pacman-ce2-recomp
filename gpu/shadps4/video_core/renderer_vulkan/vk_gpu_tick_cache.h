// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <atomic>
#include <cstdint>

namespace Vulkan {

// A successful timeline wait proves only its requested tick complete. Another
// waiter or counter query may already have published a later completion.
inline void PublishWaitedGpuTick(std::atomic<std::uint64_t>& completed,
                                 std::uint64_t waited_tick, bool wait_succeeded) noexcept {
    if (!wait_succeeded) {
        return;
    }
    auto known = completed.load(std::memory_order_relaxed);
    while (known < waited_tick &&
           !completed.compare_exchange_weak(known, waited_tick, std::memory_order_release,
                                             std::memory_order_relaxed)) {
    }
}

// The callback performs one status query for this tick, never for later work.
// A pending/error result leaves the completion cache unchanged and never retries.
template <typename Poll>
bool PollGpuTick(std::atomic<std::uint64_t>& completed, std::uint64_t requested_tick, Poll&& poll) {
    if (completed.load(std::memory_order_acquire) >= requested_tick) {
        return true;
    }
    if (!poll(requested_tick)) {
        return false;
    }
    PublishWaitedGpuTick(completed, requested_tick, true);
    return true;
}

} // namespace Vulkan
