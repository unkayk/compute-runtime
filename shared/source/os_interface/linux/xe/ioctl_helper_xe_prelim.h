/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "shared/source/os_interface/linux/xe/ioctl_helper_xe.h"

namespace NEO {

class IoctlHelperXePrelim : public IoctlHelperXe {
  public:
    using IoctlHelperXe::IoctlHelperXe;

  protected:
    void setContextProperties(const OsContextLinux &osContext, void *extProperties, uint32_t &extIndexInOut) override;
};

} // namespace NEO
