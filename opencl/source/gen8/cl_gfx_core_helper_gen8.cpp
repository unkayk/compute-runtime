/*
 * Copyright (C) 2020-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/source/gen8/hw_cmds.h"
#include "shared/source/helpers/populate_factory.h"

#include "opencl/source/helpers/cl_gfx_core_helper_base.inl"
#include "opencl/source/helpers/cl_gfx_core_helper_bdw_and_later.inl"

namespace NEO {

using Family = Gen8Family;
static auto gfxCore = IGFX_GEN8_CORE;

#include "opencl/source/helpers/cl_gfx_core_helper_factory_init.inl"

template class ClGfxCoreHelperHw<Family>;

} // namespace NEO
