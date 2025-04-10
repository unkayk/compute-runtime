/*
 * Copyright (C) 2021-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/source/os_interface/windows/gdi_interface.h"
#include "shared/source/os_interface/windows/wddm/wddm.h"

namespace NEO {
NTSTATUS Wddm::createNTHandle(const D3DKMT_HANDLE *resourceHandle, HANDLE *ntHandle) {
    return getGdi()->shareObjects(1, resourceHandle, nullptr, SHARED_ALLOCATION_WRITE, ntHandle);
}
bool Wddm::getReadOnlyFlagValue(const void *cpuPtr) const {
    return false;
}
bool Wddm::isReadOnlyFlagFallbackSupported() const {
    return false;
}
} // namespace NEO