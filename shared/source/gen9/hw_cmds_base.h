/*
 * Copyright (C) 2017-2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "shared/source/commands/bxml_generator_glue.h"
#include "shared/source/gen9/hw_info.h"
#include "shared/source/helpers/debug_helpers.h"

#include "igfxfmid.h"

#include <cstddef>

template <class T>
struct CmdParse;

namespace NEO {

struct GEN9 {
#include "shared/source/generated/gen9/hw_cmds_generated_gen9.inl"

    struct DataPortBindlessSurfaceExtendedMessageDescriptor {
        union {
            struct {
                uint32_t bindlessSurfaceOffset : 20;
                uint32_t reserved : 1;
                uint32_t executionUnitExtendedMessageDescriptorDefinition : 11;
            };
            uint32_t packed;
        };

        DataPortBindlessSurfaceExtendedMessageDescriptor() {
            packed = 0;
        }

        void setBindlessSurfaceOffset(uint32_t offsetInBindlessSurfaceHeapInBytes) {
            bindlessSurfaceOffset = offsetInBindlessSurfaceHeapInBytes >> 6;
        }

        uint32_t getBindlessSurfaceOffsetToPatch() {
            return bindlessSurfaceOffset << 12;
        }
    };

    static_assert(sizeof(DataPortBindlessSurfaceExtendedMessageDescriptor) == sizeof(DataPortBindlessSurfaceExtendedMessageDescriptor::packed), "");
};

struct SKLFamily : public GEN9 {
    using PARSE = CmdParse<SKLFamily>;
    using GfxFamily = SKLFamily;
    using WALKER_TYPE = GPGPU_WALKER;
    using VFE_STATE_TYPE = MEDIA_VFE_STATE;
    using XY_COPY_BLT = typename GfxFamily::XY_SRC_COPY_BLT;
    using MI_STORE_REGISTER_MEM_CMD = typename GfxFamily::MI_STORE_REGISTER_MEM;
    static const GPGPU_WALKER cmdInitGpgpuWalker;
    static const INTERFACE_DESCRIPTOR_DATA cmdInitInterfaceDescriptorData;
    static const MEDIA_INTERFACE_DESCRIPTOR_LOAD cmdInitMediaInterfaceDescriptorLoad;
    static const MEDIA_STATE_FLUSH cmdInitMediaStateFlush;
    static const MI_BATCH_BUFFER_END cmdInitBatchBufferEnd;
    static const MI_BATCH_BUFFER_START cmdInitBatchBufferStart;
    static const PIPE_CONTROL cmdInitPipeControl;
    static const MI_SEMAPHORE_WAIT cmdInitMiSemaphoreWait;
    static const RENDER_SURFACE_STATE cmdInitRenderSurfaceState;
    static const MI_LOAD_REGISTER_IMM cmdInitLoadRegisterImm;
    static const MI_LOAD_REGISTER_REG cmdInitLoadRegisterReg;
    static const MI_LOAD_REGISTER_MEM cmdInitLoadRegisterMem;
    static const MI_STORE_DATA_IMM cmdInitStoreDataImm;
    static const MI_STORE_REGISTER_MEM cmdInitStoreRegisterMem;
    static const MI_NOOP cmdInitNoop;
    static const MI_REPORT_PERF_COUNT cmdInitReportPerfCount;
    static const MI_ATOMIC cmdInitAtomic;
    static const PIPELINE_SELECT cmdInitPipelineSelect;
    static const MI_ARB_CHECK cmdInitArbCheck;
    static const MEDIA_VFE_STATE cmdInitMediaVfeState;
    static const STATE_BASE_ADDRESS cmdInitStateBaseAddress;
    static const MEDIA_SURFACE_STATE cmdInitMediaSurfaceState;
    static const SAMPLER_STATE cmdInitSamplerState;
    static const GPGPU_CSR_BASE_ADDRESS cmdInitGpgpuCsrBaseAddress;
    static const STATE_SIP cmdInitStateSip;
    static const BINDING_TABLE_STATE cmdInitBindingTableState;
    static const MI_USER_INTERRUPT cmdInitUserInterrupt;
    static const XY_SRC_COPY_BLT cmdInitXyCopyBlt;
    static const MI_FLUSH_DW cmdInitMiFlushDw;
    static const XY_COLOR_BLT cmdInitXyColorBlt;

    static constexpr bool supportsCmdSet(GFXCORE_FAMILY cmdSetBaseFamily) {
        return cmdSetBaseFamily == IGFX_GEN8_CORE;
    }
};

} // namespace NEO
