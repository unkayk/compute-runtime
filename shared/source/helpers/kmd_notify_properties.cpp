/*
 * Copyright (C) 2018-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/source/helpers/kmd_notify_properties.h"

#include "shared/source/command_stream/queue_throttle.h"
#include "shared/source/command_stream/task_count_helper.h"
#include "shared/source/debug_settings/debug_settings_manager.h"

#include <chrono>
#include <cstdint>

using namespace NEO;

WaitParams KmdNotifyHelper::obtainTimeoutParams(bool quickKmdSleepRequest,
                                                TagAddressType currentHwTag,
                                                TaskCountType taskCountToWait,
                                                FlushStamp flushStampToWait,
                                                QueueThrottle throttle,
                                                bool kmdWaitModeActive,
                                                bool directSubmissionEnabled) {
    if (throttle == QueueThrottle::HIGH) {
        return WaitParams{true, false, false, 0};
    }

    if (flushStampToWait == 0) {
        return WaitParams{};
    }

    if (!kmdWaitModeActive) {
        return WaitParams{};
    }

    if (debugManager.flags.PowerSavingMode.get() || throttle == QueueThrottle::LOW) {
        return WaitParams{false, true, false, 1};
    }

    const int64_t taskCountDiff = (currentHwTag < taskCountToWait) ? static_cast<int64_t>(taskCountToWait - currentHwTag) : 1;
    if (!properties->enableKmdNotify && taskCountDiff > KmdNotifyConstants::minimumTaskCountDiffToCheckAcLine) {
        updateAcLineStatus();
    }

    quickKmdSleepRequest |= applyQuickKmdSleepForSporadicWait();
    WaitParams params;

    if (!properties->enableKmdNotify && !acLineConnected) {
        params.waitTimeout = KmdNotifyConstants::timeoutInMicrosecondsForDisconnectedAcLine;
    } else if (quickKmdSleepRequest && properties->enableQuickKmdSleep) {
        params.waitTimeout = properties->delayQuickKmdSleepMicroseconds;
    } else if (directSubmissionEnabled && properties->enableQuickKmdSleepForDirectSubmission) {
        params.waitTimeout = properties->delayQuickKmdSleepForDirectSubmissionMicroseconds;
    } else {
        params.waitTimeout = properties->delayKmdNotifyMicroseconds;
    }

    params.enableTimeout = (properties->enableKmdNotify || !acLineConnected);

    return params;
}

bool KmdNotifyHelper::applyQuickKmdSleepForSporadicWait() const {
    if (properties->enableQuickKmdSleepForSporadicWaits) {
        auto timeDiff = getMicrosecondsSinceEpoch() - lastWaitForCompletionTimestampUs.load();
        if (timeDiff > properties->delayQuickKmdSleepForSporadicWaitsMicroseconds) {
            return true;
        }
    }
    return false;
}

void KmdNotifyHelper::updateLastWaitForCompletionTimestamp() {
    lastWaitForCompletionTimestampUs = getMicrosecondsSinceEpoch();
}

int64_t KmdNotifyHelper::getMicrosecondsSinceEpoch() const {
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(now).count();
}

void KmdNotifyHelper::overrideFromDebugVariable(int32_t debugVariableValue, int64_t &destination) {
    if (debugVariableValue >= 0) {
        destination = static_cast<int64_t>(debugVariableValue);
    }
}

void KmdNotifyHelper::overrideFromDebugVariable(int32_t debugVariableValue, bool &destination) {
    if (debugVariableValue >= 0) {
        destination = !!(debugVariableValue);
    }
}
