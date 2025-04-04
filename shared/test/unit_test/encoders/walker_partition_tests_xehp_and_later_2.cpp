/*
 * Copyright (C) 2021-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/test/common/cmd_parse/gen_cmd_parse.h"
#include "shared/test/common/helpers/default_hw_info.h"
#include "shared/test/common/helpers/unit_test_helper.h"
#include "shared/test/common/mocks/mock_device.h"
#include "shared/test/common/mocks/mock_execution_environment.h"
#include "shared/test/common/test_macros/hw_test.h"
#include "shared/test/unit_test/encoders/walker_partition_fixture_xehp_and_later.h"

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenProgramRegisterCommandWhenItIsCalledThenLoadRegisterImmIsSetUnderPointer) {
    uint32_t registerOffset = 120u;
    uint32_t registerValue = 542u;
    auto expectedUsedSize = sizeof(WalkerPartition::LOAD_REGISTER_IMM<FamilyType>);
    void *loadRegisterImmediateAddress = cmdBufferAddress;
    WalkerPartition::programRegisterWithValue<FamilyType>(cmdBufferAddress, registerOffset, totalBytesProgrammed, registerValue);
    auto loadRegisterImmediate = genCmdCast<WalkerPartition::LOAD_REGISTER_IMM<FamilyType> *>(loadRegisterImmediateAddress);

    ASSERT_NE(nullptr, loadRegisterImmediate);
    EXPECT_EQ(expectedUsedSize, totalBytesProgrammed);
    EXPECT_EQ(registerOffset, loadRegisterImmediate->getRegisterOffset());
    EXPECT_EQ(registerValue, loadRegisterImmediate->getDataDword());
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenWalkerPartitionWhenConstructCommandBufferIsCalledWithoutBatchBufferEndThenBatchBufferEndIsNotProgrammed) {
    using WalkerType = typename FamilyType::DefaultWalkerType;

    testArgs.partitionCount = 16u;
    testArgs.tileCount = 4u;
    checkForProperCmdBufferAddressOffset = false;
    uint64_t gpuVirtualAddress = 0x8000123000;
    WalkerType walker;
    walker = FamilyType::template getInitGpuWalker<WalkerType>();
    walker.setPartitionType(WalkerType::PARTITION_TYPE::PARTITION_TYPE_X);

    auto device = std::unique_ptr<MockDevice>(MockDevice::createWithNewExecutionEnvironment<MockDevice>(defaultHwInfo.get()));

    WalkerPartition::constructDynamicallyPartitionedCommandBuffer<FamilyType>(cmdBuffer,
                                                                              nullptr,
                                                                              gpuVirtualAddress,
                                                                              &walker,
                                                                              totalBytesProgrammed,
                                                                              testArgs,
                                                                              *device);
    auto totalProgrammedSize = computeControlSectionOffset<FamilyType, WalkerType>(testArgs) +
                               sizeof(BatchBufferControlData);
    EXPECT_EQ(totalProgrammedSize, totalBytesProgrammed);
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenEstimationWhenItIsCalledThenProperSizeIsReturned) {
    using WalkerType = typename FamilyType::DefaultWalkerType;

    testArgs.partitionCount = 16u;
    auto expectedUsedSize = sizeof(WalkerPartition::LOAD_REGISTER_IMM<FamilyType>) +
                            sizeof(WalkerPartition::MI_ATOMIC<FamilyType>) * 2 +
                            sizeof(WalkerPartition::LOAD_REGISTER_REG<FamilyType>) +
                            sizeof(WalkerPartition::MI_SET_PREDICATE<FamilyType>) * 2 +
                            sizeof(WalkerPartition::BATCH_BUFFER_START<FamilyType>) * 3 +
                            sizeof(WalkerType) +
                            sizeof(WalkerPartition::PIPE_CONTROL<FamilyType>) +
                            sizeof(WalkerPartition::BatchBufferControlData) +
                            sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>);

    testArgs.emitBatchBufferEnd = false;
    auto size = estimateSpaceRequiredInCommandBuffer<FamilyType, WalkerType>(testArgs);
    EXPECT_EQ(expectedUsedSize, size);

    testArgs.emitBatchBufferEnd = true;
    size = estimateSpaceRequiredInCommandBuffer<FamilyType, WalkerType>(testArgs);
    EXPECT_EQ(expectedUsedSize + sizeof(WalkerPartition::BATCH_BUFFER_END<FamilyType>), size);
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenEstimationWhenPartitionCountIs4ThenSizeIsProperlyEstimated) {
    using WalkerType = typename FamilyType::DefaultWalkerType;

    testArgs.partitionCount = 4u;
    auto expectedUsedSize = sizeof(WalkerPartition::LOAD_REGISTER_IMM<FamilyType>) +
                            sizeof(WalkerPartition::MI_ATOMIC<FamilyType>) * 2 +
                            sizeof(WalkerPartition::LOAD_REGISTER_REG<FamilyType>) +
                            sizeof(WalkerPartition::MI_SET_PREDICATE<FamilyType>) * 2 +
                            sizeof(WalkerPartition::BATCH_BUFFER_START<FamilyType>) * 3 +
                            sizeof(WalkerType) +
                            sizeof(WalkerPartition::PIPE_CONTROL<FamilyType>) +
                            sizeof(WalkerPartition::BatchBufferControlData) +
                            sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>);

    testArgs.emitBatchBufferEnd = false;
    auto size = estimateSpaceRequiredInCommandBuffer<FamilyType, WalkerType>(testArgs);
    EXPECT_EQ(expectedUsedSize, size);

    testArgs.emitBatchBufferEnd = true;
    size = estimateSpaceRequiredInCommandBuffer<FamilyType, WalkerType>(testArgs);
    EXPECT_EQ(expectedUsedSize + sizeof(WalkerPartition::BATCH_BUFFER_END<FamilyType>), size);
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenEstimationAndSynchronizeBeforeExecutionWhenItIsCalledThenProperSizeIsReturned) {
    using WalkerType = typename FamilyType::DefaultWalkerType;

    testArgs.partitionCount = 16u;
    testArgs.emitBatchBufferEnd = false;
    auto expectedUsedSize = sizeof(WalkerPartition::LOAD_REGISTER_IMM<FamilyType>) +
                            sizeof(WalkerPartition::MI_ATOMIC<FamilyType>) * 2 +
                            sizeof(WalkerPartition::LOAD_REGISTER_REG<FamilyType>) +
                            sizeof(WalkerPartition::MI_SET_PREDICATE<FamilyType>) * 2 +
                            sizeof(WalkerPartition::BATCH_BUFFER_START<FamilyType>) * 3 +
                            sizeof(WalkerType) +
                            sizeof(WalkerPartition::PIPE_CONTROL<FamilyType>) +
                            sizeof(WalkerPartition::BatchBufferControlData) +
                            sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>);
    auto expectedDelta = sizeof(WalkerPartition::MI_ATOMIC<FamilyType>) +
                         sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>);

    testArgs.synchronizeBeforeExecution = false;
    auto size = estimateSpaceRequiredInCommandBuffer<FamilyType, WalkerType>(testArgs);
    EXPECT_EQ(expectedUsedSize, size);

    testArgs.synchronizeBeforeExecution = true;
    size = estimateSpaceRequiredInCommandBuffer<FamilyType, WalkerType>(testArgs);
    EXPECT_EQ(expectedUsedSize + expectedDelta, size);
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenStaticPartitioningEstimationWhenItIsCalledThenProperSizeIsReturned) {
    using WalkerType = typename FamilyType::DefaultWalkerType;

    testArgs.partitionCount = 16u;
    const auto expectedUsedSize = sizeof(WalkerPartition::LOAD_REGISTER_MEM<FamilyType>) +
                                  sizeof(WalkerType) +
                                  sizeof(WalkerPartition::PIPE_CONTROL<FamilyType>) +
                                  sizeof(WalkerPartition::MI_ATOMIC<FamilyType>) +
                                  sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>) +
                                  sizeof(WalkerPartition::BATCH_BUFFER_START<FamilyType>) +
                                  sizeof(WalkerPartition::StaticPartitioningControlSection);

    testArgs.emitBatchBufferEnd = false;
    testArgs.staticPartitioning = true;

    auto size = estimateSpaceRequiredInCommandBuffer<FamilyType, WalkerType>(testArgs);
    EXPECT_EQ(expectedUsedSize, size);

    testArgs.emitBatchBufferEnd = true;
    size = estimateSpaceRequiredInCommandBuffer<FamilyType, WalkerType>(testArgs);
    EXPECT_EQ(expectedUsedSize, size);
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenStaticPartitioningEstimationAndSynchronizeBeforeExecutionWhenItIsCalledThenProperSizeIsReturned) {
    using WalkerType = typename FamilyType::DefaultWalkerType;

    testArgs.partitionCount = 16u;
    testArgs.emitBatchBufferEnd = false;
    const auto expectedUsedSize = sizeof(WalkerPartition::LOAD_REGISTER_MEM<FamilyType>) +
                                  sizeof(WalkerType) +
                                  sizeof(WalkerPartition::PIPE_CONTROL<FamilyType>) +
                                  sizeof(WalkerPartition::MI_ATOMIC<FamilyType>) +
                                  sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>) +
                                  sizeof(WalkerPartition::BATCH_BUFFER_START<FamilyType>) +
                                  sizeof(WalkerPartition::StaticPartitioningControlSection);

    testArgs.staticPartitioning = true;
    testArgs.synchronizeBeforeExecution = false;

    auto size = estimateSpaceRequiredInCommandBuffer<FamilyType, WalkerType>(testArgs);
    EXPECT_EQ(expectedUsedSize, size);

    testArgs.synchronizeBeforeExecution = true;
    const auto preExecutionSynchronizationSize = sizeof(WalkerPartition::MI_ATOMIC<FamilyType>) + sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>);

    size = estimateSpaceRequiredInCommandBuffer<FamilyType, WalkerType>(testArgs);
    EXPECT_EQ(expectedUsedSize + preExecutionSynchronizationSize, size);
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenEstimationSelfCleanupSectionsWhenItIsCalledThenProperSizeIsReturned) {
    using WalkerType = typename FamilyType::DefaultWalkerType;

    testArgs.partitionCount = 16u;
    testArgs.emitBatchBufferEnd = false;
    testArgs.synchronizeBeforeExecution = false;
    testArgs.emitSelfCleanup = true;

    auto expectedUsedSize = sizeof(WalkerPartition::LOAD_REGISTER_IMM<FamilyType>) +
                            sizeof(WalkerPartition::MI_ATOMIC<FamilyType>) * 2 +
                            sizeof(WalkerPartition::LOAD_REGISTER_REG<FamilyType>) +
                            sizeof(WalkerPartition::MI_SET_PREDICATE<FamilyType>) * 2 +
                            sizeof(WalkerPartition::BATCH_BUFFER_START<FamilyType>) * 3 +
                            sizeof(WalkerType) +
                            sizeof(WalkerPartition::MI_STORE_DATA_IMM<FamilyType>) +
                            sizeof(WalkerPartition::PIPE_CONTROL<FamilyType>) +
                            sizeof(WalkerPartition::BatchBufferControlData) +
                            sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>) +
                            sizeof(WalkerPartition::MI_ATOMIC<FamilyType>) * 2 +
                            sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>) * 2 +
                            sizeof(WalkerPartition::MI_STORE_DATA_IMM<FamilyType>) * 3;

    auto size = estimateSpaceRequiredInCommandBuffer<FamilyType, WalkerType>(testArgs);
    EXPECT_EQ(expectedUsedSize, size);
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenEstimationSelfCleanupSectionsWhenAtomicsUsedForSelfCleanupThenProperSizeIsReturned) {
    using WalkerType = typename FamilyType::DefaultWalkerType;

    testArgs.partitionCount = 16u;
    testArgs.emitBatchBufferEnd = false;
    testArgs.synchronizeBeforeExecution = false;
    testArgs.emitSelfCleanup = true;
    testArgs.useAtomicsForSelfCleanup = true;

    auto expectedUsedSize = sizeof(WalkerPartition::LOAD_REGISTER_IMM<FamilyType>) +
                            sizeof(WalkerPartition::MI_ATOMIC<FamilyType>) * 2 +
                            sizeof(WalkerPartition::LOAD_REGISTER_REG<FamilyType>) +
                            sizeof(WalkerPartition::MI_SET_PREDICATE<FamilyType>) * 2 +
                            sizeof(WalkerPartition::BATCH_BUFFER_START<FamilyType>) * 3 +
                            sizeof(WalkerType) +
                            sizeof(WalkerPartition::MI_ATOMIC<FamilyType>) +
                            sizeof(WalkerPartition::PIPE_CONTROL<FamilyType>) +
                            sizeof(WalkerPartition::BatchBufferControlData) +
                            sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>) +
                            sizeof(WalkerPartition::MI_ATOMIC<FamilyType>) * 2 +
                            sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>) * 2 +
                            sizeof(WalkerPartition::MI_ATOMIC<FamilyType>) * 3;

    auto size = estimateSpaceRequiredInCommandBuffer<FamilyType, WalkerType>(testArgs);
    EXPECT_EQ(expectedUsedSize, size);
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenProgramWparidPredicationMaskWhenItIsCalledWithWrongInputThenFalseIsReturnedAndNothingIsProgrammed) {
    EXPECT_FALSE(programWparidMask<FamilyType>(cmdBufferAddress, totalBytesProgrammed, 3));
    EXPECT_FALSE(programWparidMask<FamilyType>(cmdBufferAddress, totalBytesProgrammed, 5));
    EXPECT_FALSE(programWparidMask<FamilyType>(cmdBufferAddress, totalBytesProgrammed, 17));
    EXPECT_FALSE(programWparidMask<FamilyType>(cmdBufferAddress, totalBytesProgrammed, 32));
    EXPECT_FALSE(programWparidMask<FamilyType>(cmdBufferAddress, totalBytesProgrammed, 15));
    EXPECT_FALSE(programWparidMask<FamilyType>(cmdBufferAddress, totalBytesProgrammed, 11));
    EXPECT_FALSE(programWparidMask<FamilyType>(cmdBufferAddress, totalBytesProgrammed, 9));
    EXPECT_EQ(0u, totalBytesProgrammed);
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenProgramWparidPredicationMaskWhenItIsCalledWithPartitionCountThenProperMaskIsSet) {
    auto wparidMaskProgrammingLocation = cmdBufferAddress;
    EXPECT_TRUE(programWparidMask<FamilyType>(cmdBufferAddress, totalBytesProgrammed, 16));
    auto expectedUsedSize = sizeof(WalkerPartition::LOAD_REGISTER_IMM<FamilyType>);
    EXPECT_EQ(expectedUsedSize, totalBytesProgrammed);

    auto expectedMask = 0xFFF0u;
    auto expectedRegister = 0x21FCu;

    auto loadRegisterImmediate = genCmdCast<WalkerPartition::LOAD_REGISTER_IMM<FamilyType> *>(wparidMaskProgrammingLocation);
    ASSERT_NE(nullptr, loadRegisterImmediate);
    EXPECT_EQ(expectedRegister, loadRegisterImmediate->getRegisterOffset());
    EXPECT_EQ(expectedMask, loadRegisterImmediate->getDataDword());

    EXPECT_TRUE(programWparidMask<FamilyType>(cmdBufferAddress, totalBytesProgrammed, 8));
    wparidMaskProgrammingLocation = ptrOffset(wparidMaskProgrammingLocation, sizeof(LOAD_REGISTER_IMM<FamilyType>));
    loadRegisterImmediate = genCmdCast<WalkerPartition::LOAD_REGISTER_IMM<FamilyType> *>(wparidMaskProgrammingLocation);
    expectedMask = 0xFFF8u;
    EXPECT_EQ(expectedMask, loadRegisterImmediate->getDataDword());

    EXPECT_TRUE(programWparidMask<FamilyType>(cmdBufferAddress, totalBytesProgrammed, 4));
    wparidMaskProgrammingLocation = ptrOffset(wparidMaskProgrammingLocation, sizeof(LOAD_REGISTER_IMM<FamilyType>));
    loadRegisterImmediate = genCmdCast<WalkerPartition::LOAD_REGISTER_IMM<FamilyType> *>(wparidMaskProgrammingLocation);
    expectedMask = 0xFFFCu;
    EXPECT_EQ(expectedMask, loadRegisterImmediate->getDataDword());

    EXPECT_TRUE(programWparidMask<FamilyType>(cmdBufferAddress, totalBytesProgrammed, 2));
    wparidMaskProgrammingLocation = ptrOffset(wparidMaskProgrammingLocation, sizeof(LOAD_REGISTER_IMM<FamilyType>));
    loadRegisterImmediate = genCmdCast<WalkerPartition::LOAD_REGISTER_IMM<FamilyType> *>(wparidMaskProgrammingLocation);
    expectedMask = 0xFFFEu;
    EXPECT_EQ(expectedMask, loadRegisterImmediate->getDataDword());
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenProgramPredicationOnWhenItIsProgrammedThenCommandBufferContainsCorrectCommand) {
    auto expectedUsedSize = sizeof(WalkerPartition::MI_SET_PREDICATE<FamilyType>);

    void *miSetPredicateAddress = cmdBufferAddress;
    programWparidPredication<FamilyType>(cmdBufferAddress, totalBytesProgrammed, true);
    auto miSetPredicate = genCmdCast<WalkerPartition::MI_SET_PREDICATE<FamilyType> *>(miSetPredicateAddress);

    ASSERT_NE(nullptr, miSetPredicate);
    EXPECT_EQ(expectedUsedSize, totalBytesProgrammed);
    EXPECT_EQ(miSetPredicate->getPredicateEnableWparid(), MI_SET_PREDICATE<FamilyType>::PREDICATE_ENABLE_WPARID::PREDICATE_ENABLE_WPARID_NOOP_ON_NON_ZERO_VALUE);
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenProgramPredicationOffWhenItIsProgrammedThenCommandBufferContainsCorrectCommand) {
    auto expectedUsedSize = sizeof(WalkerPartition::MI_SET_PREDICATE<FamilyType>);

    void *miSetPredicateAddress = cmdBufferAddress;
    programWparidPredication<FamilyType>(cmdBufferAddress, totalBytesProgrammed, false);
    EXPECT_EQ(expectedUsedSize, totalBytesProgrammed);
    auto miSetPredicate = genCmdCast<WalkerPartition::MI_SET_PREDICATE<FamilyType> *>(miSetPredicateAddress);
    ASSERT_NE(nullptr, miSetPredicate);
    EXPECT_EQ(miSetPredicate->getPredicateEnableWparid(), MI_SET_PREDICATE<FamilyType>::PREDICATE_ENABLE_WPARID::PREDICATE_ENABLE_WPARID_NOOP_NEVER);
    EXPECT_EQ(miSetPredicate->getPredicateEnable(), MI_SET_PREDICATE<FamilyType>::PREDICATE_ENABLE::PREDICATE_ENABLE_PREDICATE_DISABLE);
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenProgramWaitForSemaphoreWhenitisProgrammedThenAllFieldsAreSetCorrectly) {
    auto expectedUsedSize = sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>);
    uint64_t gpuAddress = 0x6432100llu;
    uint32_t compareData = 1u;

    void *semaphoreWaitAddress = cmdBufferAddress;
    programWaitForSemaphore<FamilyType>(cmdBufferAddress,
                                        totalBytesProgrammed,
                                        gpuAddress,
                                        compareData,
                                        MI_SEMAPHORE_WAIT<FamilyType>::COMPARE_OPERATION::COMPARE_OPERATION_SAD_NOT_EQUAL_SDD);
    auto semaphoreWait = genCmdCast<WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType> *>(semaphoreWaitAddress);
    EXPECT_EQ(expectedUsedSize, totalBytesProgrammed);

    ASSERT_NE(nullptr, semaphoreWait);
    EXPECT_EQ(compareData, semaphoreWait->getSemaphoreDataDword());
    EXPECT_EQ(gpuAddress, semaphoreWait->getSemaphoreGraphicsAddress());
    EXPECT_EQ(MI_SEMAPHORE_WAIT<FamilyType>::COMPARE_OPERATION::COMPARE_OPERATION_SAD_NOT_EQUAL_SDD, semaphoreWait->getCompareOperation());
    EXPECT_EQ(MI_SEMAPHORE_WAIT<FamilyType>::WAIT_MODE::WAIT_MODE_POLLING_MODE, semaphoreWait->getWaitMode());
    EXPECT_EQ(MI_SEMAPHORE_WAIT<FamilyType>::MEMORY_TYPE::MEMORY_TYPE_PER_PROCESS_GRAPHICS_ADDRESS, semaphoreWait->getMemoryType());
    EXPECT_EQ(MI_SEMAPHORE_WAIT<FamilyType>::REGISTER_POLL_MODE::REGISTER_POLL_MODE_MEMORY_POLL, semaphoreWait->getRegisterPollMode());
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenMiAtomicWhenItIsProgrammedThenAllFieldsAreSetCorrectly) {
    auto expectedUsedSize = sizeof(WalkerPartition::MI_ATOMIC<FamilyType>);
    uint64_t gpuAddress = 0xFFFFFFDFEEDBAC10llu;

    void *miAtomicAddress = cmdBufferAddress;
    programMiAtomic<FamilyType>(cmdBufferAddress,
                                totalBytesProgrammed, gpuAddress, true, MI_ATOMIC<FamilyType>::ATOMIC_OPCODES::ATOMIC_4B_INCREMENT);

    auto miAtomic = genCmdCast<WalkerPartition::MI_ATOMIC<FamilyType> *>(miAtomicAddress);
    ASSERT_NE(nullptr, miAtomic);
    EXPECT_EQ(expectedUsedSize, totalBytesProgrammed);
    EXPECT_EQ(MI_ATOMIC<FamilyType>::ATOMIC_OPCODES::ATOMIC_4B_INCREMENT, miAtomic->getAtomicOpcode());
    EXPECT_EQ(0u, miAtomic->getDataSize());
    EXPECT_TRUE(miAtomic->getCsStall());
    EXPECT_EQ(MI_ATOMIC<FamilyType>::MEMORY_TYPE::MEMORY_TYPE_PER_PROCESS_GRAPHICS_ADDRESS, miAtomic->getMemoryType());
    EXPECT_TRUE(miAtomic->getReturnDataControl());
    EXPECT_FALSE(miAtomic->getWorkloadPartitionIdOffsetEnable());
    auto memoryAddress = UnitTestHelper<FamilyType>::getAtomicMemoryAddress(*miAtomic);

    // bits 48-63 are zeroed
    EXPECT_EQ((gpuAddress & 0xFFFFFFFFFFFF), memoryAddress);
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenMiLoadRegisterRegWhenItIsProgrammedThenCommandIsProperlySet) {
    auto expectedUsedSize = sizeof(WalkerPartition::LOAD_REGISTER_REG<FamilyType>);
    void *loadRegisterRegAddress = cmdBufferAddress;
    WalkerPartition::programMiLoadRegisterReg<FamilyType>(cmdBufferAddress, totalBytesProgrammed, generalPurposeRegister1, wparidCCSOffset);
    auto loadRegisterReg = genCmdCast<WalkerPartition::LOAD_REGISTER_REG<FamilyType> *>(loadRegisterRegAddress);
    ASSERT_NE(nullptr, loadRegisterReg);
    EXPECT_EQ(expectedUsedSize, totalBytesProgrammed);

    EXPECT_TRUE(loadRegisterReg->getMmioRemapEnableDestination());
    EXPECT_TRUE(loadRegisterReg->getMmioRemapEnableSource());
    EXPECT_EQ(generalPurposeRegister1, loadRegisterReg->getSourceRegisterAddress());
    EXPECT_EQ(wparidCCSOffset, loadRegisterReg->getDestinationRegisterAddress());
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenProgramPipeControlCommandWhenItIsProgrammedThenItIsProperlySet) {
    auto expectedUsedSize = sizeof(WalkerPartition::PIPE_CONTROL<FamilyType>);
    void *pipeControlCAddress = cmdBufferAddress;
    PipeControlArgs args;
    args.dcFlushEnable = true;
    WalkerPartition::programPipeControlCommand<FamilyType>(cmdBufferAddress, totalBytesProgrammed, args);
    auto pipeControl = genCmdCast<WalkerPartition::PIPE_CONTROL<FamilyType> *>(pipeControlCAddress);
    ASSERT_NE(nullptr, pipeControl);
    EXPECT_EQ(expectedUsedSize, totalBytesProgrammed);

    EXPECT_TRUE(pipeControl->getCommandStreamerStallEnable());
    EXPECT_TRUE(pipeControl->getDcFlushEnable());
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenProgramPipeControlCommandWhenItIsProgrammedWithDcFlushFalseThenExpectDcFlushFlagFalse) {
    auto expectedUsedSize = sizeof(WalkerPartition::PIPE_CONTROL<FamilyType>);
    void *pipeControlCAddress = cmdBufferAddress;
    PipeControlArgs args;
    args.dcFlushEnable = false;
    WalkerPartition::programPipeControlCommand<FamilyType>(cmdBufferAddress, totalBytesProgrammed, args);
    auto pipeControl = genCmdCast<WalkerPartition::PIPE_CONTROL<FamilyType> *>(pipeControlCAddress);
    ASSERT_NE(nullptr, pipeControl);
    EXPECT_EQ(expectedUsedSize, totalBytesProgrammed);

    EXPECT_TRUE(pipeControl->getCommandStreamerStallEnable());
    EXPECT_FALSE(pipeControl->getDcFlushEnable());
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenProgramPipeControlCommandWhenItIsProgrammedWithDebugDoNotFlushThenItIsProperlySetWithoutDcFlush) {
    DebugManagerStateRestore restore;
    debugManager.flags.DoNotFlushCaches.set(true);
    auto expectedUsedSize = sizeof(WalkerPartition::PIPE_CONTROL<FamilyType>);
    void *pipeControlCAddress = cmdBufferAddress;
    PipeControlArgs args;
    args.dcFlushEnable = true;
    WalkerPartition::programPipeControlCommand<FamilyType>(cmdBufferAddress, totalBytesProgrammed, args);
    auto pipeControl = genCmdCast<WalkerPartition::PIPE_CONTROL<FamilyType> *>(pipeControlCAddress);
    ASSERT_NE(nullptr, pipeControl);
    EXPECT_EQ(expectedUsedSize, totalBytesProgrammed);

    EXPECT_TRUE(pipeControl->getCommandStreamerStallEnable());
    EXPECT_FALSE(pipeControl->getDcFlushEnable());
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenProgramBatchBufferStartCommandWhenItIsCalledThenCommandIsProgrammedCorrectly) {
    auto expectedUsedSize = sizeof(WalkerPartition::BATCH_BUFFER_START<FamilyType>);
    uint64_t gpuAddress = 0xFFFFFFDFEEDBAC10llu;

    void *batchBufferStartAddress = cmdBufferAddress;
    WalkerPartition::programMiBatchBufferStart<FamilyType>(cmdBufferAddress, totalBytesProgrammed, gpuAddress, true, false);
    auto batchBufferStart = genCmdCast<WalkerPartition::BATCH_BUFFER_START<FamilyType> *>(batchBufferStartAddress);
    ASSERT_NE(nullptr, batchBufferStart);
    EXPECT_EQ(expectedUsedSize, totalBytesProgrammed);

    // bits 48-63 are zeroed
    EXPECT_EQ((gpuAddress & 0xFFFFFFFFFFFF), batchBufferStart->getBatchBufferStartAddress());

    EXPECT_TRUE(batchBufferStart->getPredicationEnable());
    EXPECT_FALSE(batchBufferStart->getEnableCommandCache());
    EXPECT_EQ(BATCH_BUFFER_START<FamilyType>::SECOND_LEVEL_BATCH_BUFFER::SECOND_LEVEL_BATCH_BUFFER_FIRST_LEVEL_BATCH, batchBufferStart->getSecondLevelBatchBuffer());
    EXPECT_EQ(BATCH_BUFFER_START<FamilyType>::ADDRESS_SPACE_INDICATOR::ADDRESS_SPACE_INDICATOR_PPGTT, batchBufferStart->getAddressSpaceIndicator());
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenProgramComputeWalkerWhenItIsCalledThenWalkerIsProperlyProgrammed) {
    using WalkerType = typename FamilyType::DefaultWalkerType;

    auto device = std::unique_ptr<NEO::MockDevice>(NEO::MockDevice::createWithNewExecutionEnvironment<NEO::MockDevice>(NEO::defaultHwInfo.get()));

    auto expectedUsedSize = sizeof(WalkerType);
    WalkerType walker;
    walker = FamilyType::template getInitGpuWalker<WalkerType>();
    walker.setThreadGroupIdXDimension(7u);
    walker.setThreadGroupIdYDimension(10u);
    walker.setThreadGroupIdZDimension(11u);

    walker.setPartitionType(WalkerType::PARTITION_TYPE::PARTITION_TYPE_X);
    void *walkerCommandAddress = cmdBufferAddress;
    WalkerPartition::WalkerPartitionArgs args = {};
    args.partitionCount = 2;
    args.tileCount = 2;
    args.threadGroupCount = 7 * 10 * 11;
    programPartitionedWalker<FamilyType>(cmdBufferAddress, totalBytesProgrammed, &walker, args, *device);
    auto walkerCommand = genCmdCast<WalkerType *>(walkerCommandAddress);

    ASSERT_NE(nullptr, walkerCommand);
    EXPECT_EQ(expectedUsedSize, totalBytesProgrammed);
    EXPECT_TRUE(walkerCommand->getWorkloadPartitionEnable());
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_X, walkerCommand->getPartitionType());
    EXPECT_EQ(4u, walkerCommand->getPartitionSize());

    walker.setPartitionType(WalkerType::PARTITION_TYPE::PARTITION_TYPE_Y);
    walkerCommandAddress = cmdBufferAddress;
    args = {};
    args.partitionCount = 2;
    args.tileCount = 2;
    args.threadGroupCount = 7 * 10 * 11;
    programPartitionedWalker<FamilyType>(cmdBufferAddress, totalBytesProgrammed, &walker, args, *device);
    walkerCommand = genCmdCast<WalkerType *>(walkerCommandAddress);

    ASSERT_NE(nullptr, walkerCommand);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_Y, walkerCommand->getPartitionType());
    EXPECT_EQ(5u, walkerCommand->getPartitionSize());

    walker.setPartitionType(WalkerType::PARTITION_TYPE::PARTITION_TYPE_Z);
    walkerCommandAddress = cmdBufferAddress;
    args = {};
    args.partitionCount = 2;
    args.tileCount = 2;
    args.threadGroupCount = 7 * 10 * 11;
    programPartitionedWalker<FamilyType>(cmdBufferAddress, totalBytesProgrammed, &walker, args, *device);
    walkerCommand = genCmdCast<WalkerType *>(walkerCommandAddress);

    ASSERT_NE(nullptr, walkerCommand);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_Z, walkerCommand->getPartitionType());
    EXPECT_EQ(6u, walkerCommand->getPartitionSize());

    // if we program with partition Count == 1 then do not trigger partition stuff
    walker.setPartitionType(WalkerType::PARTITION_TYPE::PARTITION_TYPE_DISABLED);
    walkerCommandAddress = cmdBufferAddress;
    args = {};
    args.partitionCount = 1;
    args.tileCount = 2;
    args.threadGroupCount = 7 * 10 * 11;
    programPartitionedWalker<FamilyType>(cmdBufferAddress, totalBytesProgrammed, &walker, args, *device);
    walkerCommand = genCmdCast<WalkerType *>(walkerCommandAddress);

    ASSERT_NE(nullptr, walkerCommand);
    EXPECT_EQ(6u, walkerCommand->getPartitionSize());
    EXPECT_TRUE(walkerCommand->getWorkloadPartitionEnable());
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_DISABLED, walkerCommand->getPartitionType());
}
HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenWalkerWhenComputePartitionCountIsCalledThenDefaultSizeAndTypeIsReturned) {
    using WalkerType = typename FamilyType::DefaultWalkerType;

    WalkerType walker;
    walker = FamilyType::template getInitGpuWalker<WalkerType>();
    walker.setThreadGroupIdXDimension(16u);

    bool staticPartitioning = false;
    auto partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 2u, false, &staticPartitioning);
    EXPECT_FALSE(staticPartitioning);
    EXPECT_EQ(2u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_X, walker.getPartitionType());
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenWalkerWithNonUniformStartWhenComputePartitionCountIsCalledThenPartitionsAreDisabled) {
    using WalkerType = typename FamilyType::DefaultWalkerType;

    WalkerType walker;
    walker = FamilyType::template getInitGpuWalker<WalkerType>();
    walker.setThreadGroupIdStartingX(1u);

    bool staticPartitioning = false;
    auto partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 4u, false, &staticPartitioning);
    EXPECT_FALSE(staticPartitioning);
    EXPECT_EQ(1u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_DISABLED, walker.getPartitionType());

    walker.setThreadGroupIdStartingX(0u);
    walker.setThreadGroupIdStartingY(1u);

    partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 4u, false, &staticPartitioning);
    EXPECT_FALSE(staticPartitioning);
    EXPECT_EQ(1u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_DISABLED, walker.getPartitionType());

    walker.setThreadGroupIdStartingY(0u);
    walker.setThreadGroupIdStartingZ(1u);

    partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 4u, false, &staticPartitioning);
    EXPECT_FALSE(staticPartitioning);
    EXPECT_EQ(1u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_DISABLED, walker.getPartitionType());
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenWalkerWithDifferentWorkgroupCountsWhenPartitionCountIsObtainedThenHighestDimensionIsPartitioned) {
    using WalkerType = typename FamilyType::DefaultWalkerType;

    WalkerType walker;
    walker = FamilyType::template getInitGpuWalker<WalkerType>();
    walker.setThreadGroupIdXDimension(64u);
    walker.setThreadGroupIdYDimension(64u);
    walker.setThreadGroupIdZDimension(64u);

    bool staticPartitioning = false;
    auto partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 4u, false, &staticPartitioning);
    EXPECT_FALSE(staticPartitioning);
    EXPECT_EQ(4u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_X, walker.getPartitionType());

    walker.setThreadGroupIdYDimension(65u);
    walker.setPartitionType(WalkerType::PARTITION_TYPE::PARTITION_TYPE_DISABLED);
    partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 4u, false, &staticPartitioning);
    EXPECT_FALSE(staticPartitioning);
    EXPECT_EQ(4u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_Y, walker.getPartitionType());

    walker.setThreadGroupIdZDimension(66u);
    walker.setPartitionType(WalkerType::PARTITION_TYPE::PARTITION_TYPE_DISABLED);
    partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 4u, false, &staticPartitioning);
    EXPECT_FALSE(staticPartitioning);
    EXPECT_EQ(4u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_Z, walker.getPartitionType());
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenDisabledMinimalPartitionSizeWhenComputePartitionSizeThenProperValueIsReturned) {
    using WalkerType = typename FamilyType::DefaultWalkerType;

    WalkerType walker;
    walker = FamilyType::template getInitGpuWalker<WalkerType>();
    walker.setThreadGroupIdXDimension(64u);
    walker.setThreadGroupIdYDimension(64u);
    walker.setThreadGroupIdZDimension(64u);

    DebugManagerStateRestore restorer;
    debugManager.flags.SetMinimalPartitionSize.set(0);

    bool staticPartitioning = false;
    auto partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 4u, false, &staticPartitioning);
    EXPECT_FALSE(staticPartitioning);
    EXPECT_EQ(16u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_X, walker.getPartitionType());

    walker.setThreadGroupIdYDimension(65u);
    walker.setPartitionType(WalkerType::PARTITION_TYPE::PARTITION_TYPE_DISABLED);
    partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 4u, false, &staticPartitioning);
    EXPECT_FALSE(staticPartitioning);
    EXPECT_EQ(16u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_Y, walker.getPartitionType());

    walker.setThreadGroupIdZDimension(66u);
    walker.setPartitionType(WalkerType::PARTITION_TYPE::PARTITION_TYPE_DISABLED);
    partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 4u, false, &staticPartitioning);
    EXPECT_FALSE(staticPartitioning);
    EXPECT_EQ(16u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_Z, walker.getPartitionType());
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenWalkerWithDifferentWorkgroupCountsWhenPartitionCountIsObtainedThenPartitionCountIsClampedToHighestDimension) {
    using WalkerType = typename FamilyType::DefaultWalkerType;

    WalkerType walker;
    walker = FamilyType::template getInitGpuWalker<WalkerType>();
    walker.setThreadGroupIdXDimension(15u);
    walker.setThreadGroupIdYDimension(7u);
    walker.setThreadGroupIdZDimension(4u);

    bool staticPartitioning = false;
    auto partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 4u, false, &staticPartitioning);
    EXPECT_FALSE(staticPartitioning);
    EXPECT_EQ(4u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_X, walker.getPartitionType());
    walker.setThreadGroupIdXDimension(1u);
    walker.setPartitionType(WalkerType::PARTITION_TYPE::PARTITION_TYPE_DISABLED);

    partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 4u, false, &staticPartitioning);
    EXPECT_FALSE(staticPartitioning);
    EXPECT_EQ(4u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_Y, walker.getPartitionType());

    walker.setThreadGroupIdYDimension(1u);
    walker.setPartitionType(WalkerType::PARTITION_TYPE::PARTITION_TYPE_DISABLED);

    partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 4u, false, &staticPartitioning);
    EXPECT_FALSE(staticPartitioning);
    EXPECT_EQ(4u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_Z, walker.getPartitionType());
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenWalkerWithPartitionTypeHintWhenPartitionCountIsObtainedThenSuggestedTypeIsUsedForPartition) {
    using WalkerType = typename FamilyType::DefaultWalkerType;

    DebugManagerStateRestore restore{};

    WalkerType walker;
    walker = FamilyType::template getInitGpuWalker<WalkerType>();
    walker.setThreadGroupIdXDimension(8u);
    walker.setThreadGroupIdYDimension(4u);
    walker.setThreadGroupIdZDimension(2u);

    debugManager.flags.ExperimentalSetWalkerPartitionType.set(-1);
    bool staticPartitioning = false;
    auto partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 4u, false, &staticPartitioning);
    EXPECT_FALSE(staticPartitioning);
    EXPECT_EQ(4u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_X, walker.getPartitionType());

    debugManager.flags.ExperimentalSetWalkerPartitionType.set(static_cast<int32_t>(WalkerType::PARTITION_TYPE::PARTITION_TYPE_X));
    partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 4u, false, &staticPartitioning);
    EXPECT_FALSE(staticPartitioning);
    EXPECT_EQ(4u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_X, walker.getPartitionType());

    debugManager.flags.ExperimentalSetWalkerPartitionType.set(static_cast<int32_t>(WalkerType::PARTITION_TYPE::PARTITION_TYPE_Y));
    partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 4u, false, &staticPartitioning);
    EXPECT_FALSE(staticPartitioning);
    EXPECT_EQ(4u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_Y, walker.getPartitionType());

    debugManager.flags.ExperimentalSetWalkerPartitionType.set(static_cast<int32_t>(WalkerType::PARTITION_TYPE::PARTITION_TYPE_Z));
    partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 4u, false, &staticPartitioning);
    EXPECT_FALSE(staticPartitioning);
    EXPECT_EQ(2u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_Z, walker.getPartitionType());
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenInvalidPartitionTypeIsRequestedWhenPartitionCountIsObtainedThenFail) {
    using WalkerType = typename FamilyType::DefaultWalkerType;

    DebugManagerStateRestore restore{};

    WalkerType walker;
    walker = FamilyType::template getInitGpuWalker<WalkerType>();
    walker.setThreadGroupIdXDimension(8u);
    walker.setThreadGroupIdYDimension(4u);
    walker.setThreadGroupIdZDimension(2u);

    debugManager.flags.ExperimentalSetWalkerPartitionType.set(0);
    bool staticPartitioning = false;
    EXPECT_ANY_THROW(computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 4u, false, &staticPartitioning));
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenWalkerWithSmallXDimensionSizeWhenPartitionCountIsObtainedThenPartitionCountIsAdujsted) {
    using WalkerType = typename FamilyType::DefaultWalkerType;

    WalkerType walker;
    walker = FamilyType::template getInitGpuWalker<WalkerType>();
    walker.setThreadGroupIdXDimension(32u);
    walker.setThreadGroupIdYDimension(1024u);
    walker.setThreadGroupIdZDimension(1u);

    bool staticPartitioning = false;
    auto partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 2u, false, &staticPartitioning);
    EXPECT_FALSE(staticPartitioning);
    EXPECT_EQ(2u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_Y, walker.getPartitionType());
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenWalkerWithBigXDimensionSizeWhenPartitionCountIsObtainedThenPartitionCountIsNotAdjusted) {
    using WalkerType = typename FamilyType::DefaultWalkerType;

    WalkerType walker;
    walker = FamilyType::template getInitGpuWalker<WalkerType>();
    walker.setThreadGroupIdXDimension(16384u);
    walker.setThreadGroupIdYDimension(1u);
    walker.setThreadGroupIdZDimension(1u);

    bool staticPartitioning = false;
    auto partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 2u, false, &staticPartitioning);
    EXPECT_FALSE(staticPartitioning);
    EXPECT_EQ(16u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_X, walker.getPartitionType());
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenCustomMinimalPartitionSizeWhenComputePartitionCountThenProperValueIsReturned) {
    using WalkerType = typename FamilyType::DefaultWalkerType;

    WalkerType walker;
    walker = FamilyType::template getInitGpuWalker<WalkerType>();
    walker.setThreadGroupIdXDimension(16384u);
    walker.setThreadGroupIdYDimension(1u);
    walker.setThreadGroupIdZDimension(1u);

    DebugManagerStateRestore restorer;
    debugManager.flags.SetMinimalPartitionSize.set(4096);

    bool staticPartitioning = false;
    auto partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 2u, false, &staticPartitioning);
    EXPECT_FALSE(staticPartitioning);
    EXPECT_EQ(4u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_X, walker.getPartitionType());
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenWalkerWithPartitionTypeProgrammedWhenPartitionCountIsObtainedAndItEqualsOneThenPartitionMechanismIsDisabled) {
    using WalkerType = typename FamilyType::DefaultWalkerType;
    using WalkerType = typename FamilyType::DefaultWalkerType;
    WalkerType walker;
    walker = FamilyType::template getInitGpuWalker<WalkerType>();
    walker.setThreadGroupIdXDimension(1u);
    walker.setThreadGroupIdYDimension(1u);
    walker.setThreadGroupIdZDimension(1u);

    bool staticPartitioning = false;
    auto partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 4u, false, &staticPartitioning);
    EXPECT_FALSE(staticPartitioning);
    EXPECT_EQ(1u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_DISABLED, walker.getPartitionType());
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenXDimensionIsNotLargetAnd2DImagesAreUsedWhenPartitionTypeIsObtainedThenSelectXDimension) {
    using WalkerType = typename FamilyType::DefaultWalkerType;
    WalkerType walker;
    walker = FamilyType::template getInitGpuWalker<WalkerType>();
    walker.setThreadGroupIdXDimension(8u);
    walker.setThreadGroupIdYDimension(64u);
    walker.setThreadGroupIdZDimension(16u);

    bool staticPartitioning = false;
    computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 4u, false, &staticPartitioning);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_Y, walker.getPartitionType());

    computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::x, 4u, false, &staticPartitioning);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_X, walker.getPartitionType());
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenStaticPartitioningAndNonPartitionableWalkerWhenPartitionCountIsObtainedThenAllowPartitioning) {
    using WalkerType = typename FamilyType::DefaultWalkerType;
    WalkerType walker;
    walker = FamilyType::template getInitGpuWalker<WalkerType>();
    walker.setThreadGroupIdXDimension(1u);
    walker.setThreadGroupIdYDimension(1u);
    walker.setThreadGroupIdZDimension(1u);

    bool staticPartitioning = false;
    auto partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::x, 4u, true, &staticPartitioning);
    EXPECT_TRUE(staticPartitioning);
    EXPECT_EQ(4u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_X, walker.getPartitionType());
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenStaticPartitioningAndPartitionableWalkerWhenPartitionCountIsObtainedThenAllowPartitioning) {
    using WalkerType = typename FamilyType::DefaultWalkerType;
    WalkerType walker;
    walker = FamilyType::template getInitGpuWalker<WalkerType>();
    walker.setThreadGroupIdXDimension(1u);
    walker.setThreadGroupIdYDimension(2u);
    walker.setThreadGroupIdZDimension(1u);

    bool staticPartitioning = false;
    auto partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 4u, true, &staticPartitioning);
    EXPECT_TRUE(staticPartitioning);
    EXPECT_EQ(4u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_Y, walker.getPartitionType());
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenStaticPartitioningAndBigPartitionCountProgrammedInWalkerWhenPartitionCountIsObtainedThenNumberOfPartitionsIsEqualToNumberOfTiles) {
    using WalkerType = typename FamilyType::DefaultWalkerType;
    WalkerType walker;
    walker = FamilyType::template getInitGpuWalker<WalkerType>();
    walker.setThreadGroupIdXDimension(1u);
    walker.setThreadGroupIdYDimension(16384u);
    walker.setThreadGroupIdZDimension(1u);

    bool staticPartitioning = false;
    auto partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 4u, true, &staticPartitioning);
    EXPECT_TRUE(staticPartitioning);
    EXPECT_EQ(4u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_Y, walker.getPartitionType());
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenStaticPartitioningAndAndNonUniformStartProgrammedInWalkerWhenPartitionCountIsObtainedThenDoNotAllowStaticPartitioningAndSetPartitionCountToOne) {
    using WalkerType = typename FamilyType::DefaultWalkerType;
    WalkerType walker;
    walker = FamilyType::template getInitGpuWalker<WalkerType>();
    walker.setThreadGroupIdXDimension(1u);
    walker.setThreadGroupIdYDimension(16384u);
    walker.setThreadGroupIdZDimension(1u);
    walker.setThreadGroupIdStartingX(0);
    walker.setThreadGroupIdStartingY(0);
    walker.setThreadGroupIdStartingZ(1);

    bool staticPartitioning = false;
    auto partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 4u, true, &staticPartitioning);
    EXPECT_FALSE(staticPartitioning);
    EXPECT_EQ(1u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_DISABLED, walker.getPartitionType());
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenStaticPartitioningAndPartitionTypeHintIsUsedWhenPartitionCountIsObtainedThenUseRequestedPartitionType) {
    using WalkerType = typename FamilyType::DefaultWalkerType;
    DebugManagerStateRestore restore{};
    WalkerType walker;
    walker = FamilyType::template getInitGpuWalker<WalkerType>();
    walker.setThreadGroupIdXDimension(1u);
    walker.setThreadGroupIdYDimension(16384u);
    walker.setThreadGroupIdZDimension(1u);

    bool staticPartitioning = false;
    auto partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 4u, true, &staticPartitioning);
    EXPECT_TRUE(staticPartitioning);
    EXPECT_EQ(4u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_Y, walker.getPartitionType());

    debugManager.flags.ExperimentalSetWalkerPartitionType.set(static_cast<int32_t>(WalkerType::PARTITION_TYPE::PARTITION_TYPE_Z));
    staticPartitioning = false;
    partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 4u, true, &staticPartitioning);
    EXPECT_TRUE(staticPartitioning);
    EXPECT_EQ(4u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_Z, walker.getPartitionType());
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenStaticPartitioningWhenZDimensionIsNotDivisibleByTwoButIsAboveThreasholThenItIsSelected) {
    using WalkerType = typename FamilyType::DefaultWalkerType;
    DebugManagerStateRestore restore{};
    WalkerType walker;
    walker = FamilyType::template getInitGpuWalker<WalkerType>();
    walker.setThreadGroupIdXDimension(1u);
    walker.setThreadGroupIdYDimension(16384u);
    walker.setThreadGroupIdZDimension(2u);

    bool staticPartitioning = true;
    auto partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 2u, true, &staticPartitioning);
    EXPECT_TRUE(staticPartitioning);
    EXPECT_EQ(2u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_Z, walker.getPartitionType());

    debugManager.flags.WalkerPartitionPreferHighestDimension.set(0);

    partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 2u, true, &staticPartitioning);
    EXPECT_TRUE(staticPartitioning);
    EXPECT_EQ(2u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_Y, walker.getPartitionType());
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenStaticPartitioningWhenYDimensionIsDivisibleByTwoThenItIsSelected) {
    using WalkerType = typename FamilyType::DefaultWalkerType;
    DebugManagerStateRestore restore{};
    WalkerType walker;
    walker = FamilyType::template getInitGpuWalker<WalkerType>();
    walker.setThreadGroupIdXDimension(16384u);
    walker.setThreadGroupIdYDimension(2u);
    walker.setThreadGroupIdZDimension(1u);

    bool staticPartitioning = true;
    auto partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 2u, true, &staticPartitioning);
    EXPECT_TRUE(staticPartitioning);
    EXPECT_EQ(2u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_Y, walker.getPartitionType());

    debugManager.flags.WalkerPartitionPreferHighestDimension.set(0);

    partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 2u, true, &staticPartitioning);
    EXPECT_TRUE(staticPartitioning);
    EXPECT_EQ(2u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_X, walker.getPartitionType());
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenStaticPartitioningWhenZDimensionIsDivisibleByTwoThenItIsSelected) {
    using WalkerType = typename FamilyType::DefaultWalkerType;
    DebugManagerStateRestore restore{};
    WalkerType walker;
    walker = FamilyType::template getInitGpuWalker<WalkerType>();
    walker.setThreadGroupIdXDimension(512u);
    walker.setThreadGroupIdYDimension(512u);
    walker.setThreadGroupIdZDimension(513u);

    bool staticPartitioning = true;
    auto partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 2u, true, &staticPartitioning);
    EXPECT_TRUE(staticPartitioning);
    EXPECT_EQ(2u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_Z, walker.getPartitionType());

    debugManager.flags.WalkerPartitionPreferHighestDimension.set(0);

    partitionCount = computePartitionCountAndSetPartitionType<FamilyType>(&walker, NEO::RequiredPartitionDim::none, 2u, true, &staticPartitioning);
    EXPECT_TRUE(staticPartitioning);
    EXPECT_EQ(2u, partitionCount);
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_Z, walker.getPartitionType());
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenSelfCleanupSectionWhenDebugForceDisableCrossTileSyncThenSelfCleanupOverridesDebugAndAddsOwnCleanupSection) {
    using WalkerType = typename FamilyType::DefaultWalkerType;
    using PostSyncType = decltype(FamilyType::template getPostSyncType<WalkerType>());
    MockExecutionEnvironment mockExecutionEnvironment{};
    mockExecutionEnvironment.incRefInternal();

    testArgs.crossTileAtomicSynchronization = false;
    testArgs.partitionCount = 16u;
    testArgs.dcFlushEnable = MemorySynchronizationCommands<FamilyType>::getDcFlushEnable(true, *mockExecutionEnvironment.rootDeviceEnvironments[0]);

    checkForProperCmdBufferAddressOffset = false;
    testArgs.emitSelfCleanup = true;
    uint64_t gpuVirtualAddress = 0x8000123000;
    uint64_t postSyncAddress = 0x8000456000;
    WalkerType walker;
    walker = FamilyType::template getInitGpuWalker<WalkerType>();
    walker.setPartitionType(WalkerType::PARTITION_TYPE::PARTITION_TYPE_X);
    auto &postSync = walker.getPostSync();
    postSync.setOperation(PostSyncType::OPERATION::OPERATION_WRITE_TIMESTAMP);
    postSync.setDestinationAddress(postSyncAddress);
    uint32_t totalBytesProgrammed = 0u;

    auto expectedCommandUsedSize = sizeof(WalkerPartition::LOAD_REGISTER_IMM<FamilyType>) +
                                   sizeof(WalkerPartition::MI_ATOMIC<FamilyType>) * 2 +
                                   sizeof(WalkerPartition::LOAD_REGISTER_REG<FamilyType>) +
                                   sizeof(WalkerPartition::MI_SET_PREDICATE<FamilyType>) * 2 +
                                   sizeof(WalkerPartition::BATCH_BUFFER_START<FamilyType>) * 3 +
                                   sizeof(WalkerPartition::PIPE_CONTROL<FamilyType>) +
                                   sizeof(WalkerType) +
                                   sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>) +
                                   sizeof(WalkerPartition::MI_STORE_DATA_IMM<FamilyType>);

    auto walkerSectionCommands = sizeof(WalkerPartition::BATCH_BUFFER_START<FamilyType>) +
                                 sizeof(WalkerType);

    auto offset = computeControlSectionOffset<FamilyType, WalkerType>(testArgs);
    EXPECT_EQ(expectedCommandUsedSize, offset);

    auto cleanupSectionOffset = expectedCommandUsedSize + sizeof(BatchBufferControlData);

    auto totalProgrammedSize = cleanupSectionOffset + 3 * sizeof(WalkerPartition::MI_STORE_DATA_IMM<FamilyType>) +
                               2 * sizeof(WalkerPartition::MI_ATOMIC<FamilyType>) +
                               2 * sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>);

    auto device = std::make_unique<MockDevice>(&mockExecutionEnvironment, 0);

    testArgs.tileCount = 4u;
    WalkerPartition::constructDynamicallyPartitionedCommandBuffer<FamilyType>(cmdBuffer,
                                                                              nullptr,
                                                                              gpuVirtualAddress,
                                                                              &walker,
                                                                              totalBytesProgrammed,
                                                                              testArgs,
                                                                              *device);

    EXPECT_EQ(totalProgrammedSize, totalBytesProgrammed);
    auto wparidMaskProgrammingLocation = cmdBufferAddress;

    auto expectedMask = 0xFFF0u;
    auto expectedRegister = 0x21FCu;

    auto loadRegisterImmediate = genCmdCast<WalkerPartition::LOAD_REGISTER_IMM<FamilyType> *>(wparidMaskProgrammingLocation);
    ASSERT_NE(nullptr, loadRegisterImmediate);
    EXPECT_EQ(expectedRegister, loadRegisterImmediate->getRegisterOffset());
    EXPECT_EQ(expectedMask, loadRegisterImmediate->getDataDword());
    auto parsedOffset = sizeof(WalkerPartition::LOAD_REGISTER_IMM<FamilyType>);

    auto miAtomic = genCmdCast<WalkerPartition::MI_ATOMIC<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miAtomic);
    auto miAtomicAddress = gpuVirtualAddress + expectedCommandUsedSize;
    auto miAtomicProgrammedAddress = UnitTestHelper<FamilyType>::getAtomicMemoryAddress(*miAtomic);
    EXPECT_EQ(miAtomicAddress, miAtomicProgrammedAddress);
    EXPECT_TRUE(miAtomic->getReturnDataControl());
    EXPECT_EQ(MI_ATOMIC<FamilyType>::ATOMIC_OPCODES::ATOMIC_4B_INCREMENT, miAtomic->getAtomicOpcode());
    parsedOffset += sizeof(WalkerPartition::MI_ATOMIC<FamilyType>);

    auto loadRegisterReg = genCmdCast<WalkerPartition::LOAD_REGISTER_REG<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, loadRegisterReg);
    EXPECT_TRUE(loadRegisterReg->getMmioRemapEnableDestination());
    EXPECT_TRUE(loadRegisterReg->getMmioRemapEnableSource());
    EXPECT_EQ(wparidCCSOffset, loadRegisterReg->getDestinationRegisterAddress());
    EXPECT_EQ(generalPurposeRegister4, loadRegisterReg->getSourceRegisterAddress());
    parsedOffset += sizeof(WalkerPartition::LOAD_REGISTER_REG<FamilyType>);

    auto miSetPredicate = genCmdCast<WalkerPartition::MI_SET_PREDICATE<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miSetPredicate);
    EXPECT_EQ(miSetPredicate->getPredicateEnableWparid(), MI_SET_PREDICATE<FamilyType>::PREDICATE_ENABLE_WPARID::PREDICATE_ENABLE_WPARID_NOOP_ON_NON_ZERO_VALUE);
    parsedOffset += sizeof(WalkerPartition::MI_SET_PREDICATE<FamilyType>);

    auto batchBufferStart = genCmdCast<WalkerPartition::BATCH_BUFFER_START<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, batchBufferStart);
    EXPECT_TRUE(batchBufferStart->getPredicationEnable());
    // address routes to WALKER section which is before control section
    auto address = batchBufferStart->getBatchBufferStartAddress();
    EXPECT_EQ(address, gpuVirtualAddress + expectedCommandUsedSize - walkerSectionCommands);
    parsedOffset += sizeof(WalkerPartition::BATCH_BUFFER_START<FamilyType>);

    miSetPredicate = genCmdCast<WalkerPartition::MI_SET_PREDICATE<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miSetPredicate);
    EXPECT_EQ(miSetPredicate->getPredicateEnableWparid(), MI_SET_PREDICATE<FamilyType>::PREDICATE_ENABLE_WPARID::PREDICATE_ENABLE_WPARID_NOOP_NEVER);
    EXPECT_EQ(miSetPredicate->getPredicateEnable(), MI_SET_PREDICATE<FamilyType>::PREDICATE_ENABLE::PREDICATE_ENABLE_PREDICATE_DISABLE);
    parsedOffset += sizeof(WalkerPartition::MI_SET_PREDICATE<FamilyType>);

    uint64_t expectedCleanupGpuVa = gpuVirtualAddress + expectedCommandUsedSize + offsetof(BatchBufferControlData, finalSyncTileCount);
    constexpr uint32_t expectedData = 0u;
    auto finalSyncTileCountFieldStore = genCmdCast<WalkerPartition::MI_STORE_DATA_IMM<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, finalSyncTileCountFieldStore);
    EXPECT_EQ(expectedCleanupGpuVa, finalSyncTileCountFieldStore->getAddress());
    EXPECT_EQ(expectedData, finalSyncTileCountFieldStore->getDataDword0());
    parsedOffset += sizeof(WalkerPartition::MI_STORE_DATA_IMM<FamilyType>);

    auto pipeControl = genCmdCast<WalkerPartition::PIPE_CONTROL<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, pipeControl);
    EXPECT_TRUE(pipeControl->getCommandStreamerStallEnable());
    EXPECT_EQ(MemorySynchronizationCommands<FamilyType>::getDcFlushEnable(true, *mockExecutionEnvironment.rootDeviceEnvironments[0]), pipeControl->getDcFlushEnable());
    parsedOffset += sizeof(WalkerPartition::PIPE_CONTROL<FamilyType>);

    miAtomic = genCmdCast<WalkerPartition::MI_ATOMIC<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miAtomic);
    auto miAtomicTileAddress = gpuVirtualAddress + expectedCommandUsedSize + sizeof(uint32_t);
    auto miAtomicTileProgrammedAddress = UnitTestHelper<FamilyType>::getAtomicMemoryAddress(*miAtomic);
    EXPECT_EQ(miAtomicTileAddress, miAtomicTileProgrammedAddress);
    EXPECT_FALSE(miAtomic->getReturnDataControl());
    EXPECT_EQ(MI_ATOMIC<FamilyType>::ATOMIC_OPCODES::ATOMIC_4B_INCREMENT, miAtomic->getAtomicOpcode());
    parsedOffset += sizeof(WalkerPartition::MI_ATOMIC<FamilyType>);

    auto miSemaphoreWait = genCmdCast<WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miSemaphoreWait);
    EXPECT_EQ(miSemaphoreWait->getSemaphoreGraphicsAddress(), miAtomicTileAddress);
    EXPECT_EQ(miSemaphoreWait->getCompareOperation(), MI_SEMAPHORE_WAIT<FamilyType>::COMPARE_OPERATION::COMPARE_OPERATION_SAD_GREATER_THAN_OR_EQUAL_SDD);
    EXPECT_EQ(miSemaphoreWait->getSemaphoreDataDword(), testArgs.tileCount);

    parsedOffset += sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>);

    // final batch buffer start that routes at the end of the batch buffer
    auto batchBufferStartFinal = genCmdCast<WalkerPartition::BATCH_BUFFER_START<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, batchBufferStartFinal);
    EXPECT_EQ(batchBufferStartFinal->getBatchBufferStartAddress(), gpuVirtualAddress + cleanupSectionOffset);
    parsedOffset += sizeof(WalkerPartition::BATCH_BUFFER_START<FamilyType>);

    auto computeWalker = genCmdCast<WalkerType *>(ptrOffset(cmdBuffer, parsedOffset));
    EXPECT_NE(nullptr, computeWalker);
    parsedOffset += sizeof(WalkerType);

    batchBufferStart = genCmdCast<WalkerPartition::BATCH_BUFFER_START<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, batchBufferStart);
    EXPECT_FALSE(batchBufferStart->getPredicationEnable());
    EXPECT_EQ(gpuVirtualAddress, batchBufferStart->getBatchBufferStartAddress());
    parsedOffset += sizeof(WalkerPartition::BATCH_BUFFER_START<FamilyType>);

    auto controlSection = reinterpret_cast<BatchBufferControlData *>(ptrOffset(cmdBuffer, expectedCommandUsedSize));
    EXPECT_EQ(0u, controlSection->partitionCount);
    EXPECT_EQ(0u, controlSection->tileCount);
    EXPECT_EQ(0u, controlSection->inTileCount);
    EXPECT_EQ(0u, controlSection->finalSyncTileCount);

    parsedOffset += sizeof(BatchBufferControlData);
    EXPECT_EQ(parsedOffset, cleanupSectionOffset);

    miAtomic = genCmdCast<WalkerPartition::MI_ATOMIC<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miAtomic);
    miAtomicTileAddress = gpuVirtualAddress + cleanupSectionOffset - sizeof(BatchBufferControlData) +
                          3 * sizeof(uint32_t);
    miAtomicTileProgrammedAddress = UnitTestHelper<FamilyType>::getAtomicMemoryAddress(*miAtomic);
    EXPECT_EQ(miAtomicTileAddress, miAtomicTileProgrammedAddress);
    EXPECT_FALSE(miAtomic->getReturnDataControl());
    EXPECT_EQ(MI_ATOMIC<FamilyType>::ATOMIC_OPCODES::ATOMIC_4B_INCREMENT, miAtomic->getAtomicOpcode());
    parsedOffset += sizeof(WalkerPartition::MI_ATOMIC<FamilyType>);

    miSemaphoreWait = genCmdCast<WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miSemaphoreWait);
    EXPECT_EQ(miSemaphoreWait->getSemaphoreGraphicsAddress(), miAtomicTileAddress);
    EXPECT_EQ(miSemaphoreWait->getCompareOperation(), MI_SEMAPHORE_WAIT<FamilyType>::COMPARE_OPERATION::COMPARE_OPERATION_SAD_GREATER_THAN_OR_EQUAL_SDD);
    EXPECT_EQ(miSemaphoreWait->getSemaphoreDataDword(), testArgs.tileCount);
    parsedOffset += sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>);

    expectedCleanupGpuVa = gpuVirtualAddress + cleanupSectionOffset - sizeof(BatchBufferControlData);
    auto partitionCountFieldStore = genCmdCast<WalkerPartition::MI_STORE_DATA_IMM<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, partitionCountFieldStore);
    EXPECT_EQ(expectedCleanupGpuVa, partitionCountFieldStore->getAddress());
    EXPECT_EQ(expectedData, partitionCountFieldStore->getDataDword0());
    parsedOffset += sizeof(WalkerPartition::MI_STORE_DATA_IMM<FamilyType>);

    expectedCleanupGpuVa += sizeof(BatchBufferControlData::partitionCount);
    auto tileCountFieldStore = genCmdCast<WalkerPartition::MI_STORE_DATA_IMM<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, tileCountFieldStore);
    EXPECT_EQ(expectedCleanupGpuVa, tileCountFieldStore->getAddress());
    EXPECT_EQ(expectedData, tileCountFieldStore->getDataDword0());
    parsedOffset += sizeof(WalkerPartition::MI_STORE_DATA_IMM<FamilyType>);

    expectedCleanupGpuVa += sizeof(BatchBufferControlData::tileCount);
    auto inTileCountFieldStore = genCmdCast<WalkerPartition::MI_STORE_DATA_IMM<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, inTileCountFieldStore);
    EXPECT_EQ(expectedCleanupGpuVa, inTileCountFieldStore->getAddress());
    EXPECT_EQ(expectedData, inTileCountFieldStore->getDataDword0());
    parsedOffset += sizeof(WalkerPartition::MI_STORE_DATA_IMM<FamilyType>);

    miAtomic = genCmdCast<WalkerPartition::MI_ATOMIC<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miAtomic);
    miAtomicTileProgrammedAddress = UnitTestHelper<FamilyType>::getAtomicMemoryAddress(*miAtomic);
    EXPECT_EQ(miAtomicTileAddress, miAtomicTileProgrammedAddress);
    EXPECT_FALSE(miAtomic->getReturnDataControl());
    EXPECT_EQ(MI_ATOMIC<FamilyType>::ATOMIC_OPCODES::ATOMIC_4B_INCREMENT, miAtomic->getAtomicOpcode());
    parsedOffset += sizeof(WalkerPartition::MI_ATOMIC<FamilyType>);

    miSemaphoreWait = genCmdCast<WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miSemaphoreWait);
    EXPECT_EQ(miSemaphoreWait->getSemaphoreGraphicsAddress(), miAtomicTileAddress);
    EXPECT_EQ(miSemaphoreWait->getCompareOperation(), MI_SEMAPHORE_WAIT<FamilyType>::COMPARE_OPERATION::COMPARE_OPERATION_SAD_GREATER_THAN_OR_EQUAL_SDD);
    EXPECT_EQ(miSemaphoreWait->getSemaphoreDataDword(), 2 * testArgs.tileCount);
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenSelfCleanupAndAtomicsUsedForCleanupWhenDebugForceDisableCrossTileSyncThenSelfCleanupOverridesDebugAndAddsOwnCleanupSection) {
    using WalkerType = typename FamilyType::DefaultWalkerType;
    using PostSyncType = decltype(FamilyType::template getPostSyncType<WalkerType>());

    MockExecutionEnvironment mockExecutionEnvironment{};
    mockExecutionEnvironment.incRefInternal();

    testArgs.crossTileAtomicSynchronization = false;
    testArgs.partitionCount = 16u;
    checkForProperCmdBufferAddressOffset = false;
    testArgs.emitSelfCleanup = true;
    testArgs.useAtomicsForSelfCleanup = true;
    testArgs.dcFlushEnable = MemorySynchronizationCommands<FamilyType>::getDcFlushEnable(true, *mockExecutionEnvironment.rootDeviceEnvironments[0]);

    uint64_t gpuVirtualAddress = 0x8000123000;
    uint64_t postSyncAddress = 0x8000456000;
    WalkerType walker;
    walker = FamilyType::template getInitGpuWalker<WalkerType>();
    walker.setPartitionType(WalkerType::PARTITION_TYPE::PARTITION_TYPE_X);
    auto &postSync = walker.getPostSync();
    postSync.setOperation(PostSyncType::OPERATION::OPERATION_WRITE_TIMESTAMP);
    postSync.setDestinationAddress(postSyncAddress);
    uint32_t totalBytesProgrammed = 0u;

    auto expectedCommandUsedSize = sizeof(WalkerPartition::LOAD_REGISTER_IMM<FamilyType>) +
                                   sizeof(WalkerPartition::MI_ATOMIC<FamilyType>) * 2 +
                                   sizeof(WalkerPartition::LOAD_REGISTER_REG<FamilyType>) +
                                   sizeof(WalkerPartition::MI_SET_PREDICATE<FamilyType>) * 2 +
                                   sizeof(WalkerPartition::BATCH_BUFFER_START<FamilyType>) * 3 +
                                   sizeof(WalkerPartition::PIPE_CONTROL<FamilyType>) +
                                   sizeof(WalkerType) +
                                   sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>) +
                                   sizeof(WalkerPartition::MI_ATOMIC<FamilyType>);

    auto walkerSectionCommands = sizeof(WalkerPartition::BATCH_BUFFER_START<FamilyType>) +
                                 sizeof(WalkerType);

    auto offset = computeControlSectionOffset<FamilyType, WalkerType>(testArgs);
    EXPECT_EQ(expectedCommandUsedSize, offset);

    auto cleanupSectionOffset = expectedCommandUsedSize + sizeof(BatchBufferControlData);

    auto totalProgrammedSize = cleanupSectionOffset + 3 * sizeof(WalkerPartition::MI_ATOMIC<FamilyType>) +
                               2 * sizeof(WalkerPartition::MI_ATOMIC<FamilyType>) +
                               2 * sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>);

    auto device = std::make_unique<MockDevice>(&mockExecutionEnvironment, 0);

    testArgs.tileCount = 4u;
    WalkerPartition::constructDynamicallyPartitionedCommandBuffer<FamilyType>(cmdBuffer,
                                                                              nullptr,
                                                                              gpuVirtualAddress,
                                                                              &walker,
                                                                              totalBytesProgrammed,
                                                                              testArgs,
                                                                              *device);

    EXPECT_EQ(totalProgrammedSize, totalBytesProgrammed);
    auto wparidMaskProgrammingLocation = cmdBufferAddress;

    auto expectedMask = 0xFFF0u;
    auto expectedRegister = 0x21FCu;

    auto loadRegisterImmediate = genCmdCast<WalkerPartition::LOAD_REGISTER_IMM<FamilyType> *>(wparidMaskProgrammingLocation);
    ASSERT_NE(nullptr, loadRegisterImmediate);
    EXPECT_EQ(expectedRegister, loadRegisterImmediate->getRegisterOffset());
    EXPECT_EQ(expectedMask, loadRegisterImmediate->getDataDword());
    auto parsedOffset = sizeof(WalkerPartition::LOAD_REGISTER_IMM<FamilyType>);

    auto miAtomic = genCmdCast<WalkerPartition::MI_ATOMIC<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miAtomic);
    auto miAtomicAddress = gpuVirtualAddress + expectedCommandUsedSize;
    auto miAtomicProgrammedAddress = UnitTestHelper<FamilyType>::getAtomicMemoryAddress(*miAtomic);
    EXPECT_EQ(miAtomicAddress, miAtomicProgrammedAddress);
    EXPECT_TRUE(miAtomic->getReturnDataControl());
    EXPECT_EQ(MI_ATOMIC<FamilyType>::ATOMIC_OPCODES::ATOMIC_4B_INCREMENT, miAtomic->getAtomicOpcode());
    parsedOffset += sizeof(WalkerPartition::MI_ATOMIC<FamilyType>);

    auto loadRegisterReg = genCmdCast<WalkerPartition::LOAD_REGISTER_REG<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, loadRegisterReg);
    EXPECT_TRUE(loadRegisterReg->getMmioRemapEnableDestination());
    EXPECT_TRUE(loadRegisterReg->getMmioRemapEnableSource());
    EXPECT_EQ(wparidCCSOffset, loadRegisterReg->getDestinationRegisterAddress());
    EXPECT_EQ(generalPurposeRegister4, loadRegisterReg->getSourceRegisterAddress());
    parsedOffset += sizeof(WalkerPartition::LOAD_REGISTER_REG<FamilyType>);

    auto miSetPredicate = genCmdCast<WalkerPartition::MI_SET_PREDICATE<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miSetPredicate);
    EXPECT_EQ(miSetPredicate->getPredicateEnableWparid(), MI_SET_PREDICATE<FamilyType>::PREDICATE_ENABLE_WPARID::PREDICATE_ENABLE_WPARID_NOOP_ON_NON_ZERO_VALUE);
    parsedOffset += sizeof(WalkerPartition::MI_SET_PREDICATE<FamilyType>);

    auto batchBufferStart = genCmdCast<WalkerPartition::BATCH_BUFFER_START<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, batchBufferStart);
    EXPECT_TRUE(batchBufferStart->getPredicationEnable());
    // address routes to WALKER section which is before control section
    auto address = batchBufferStart->getBatchBufferStartAddress();
    EXPECT_EQ(address, gpuVirtualAddress + expectedCommandUsedSize - walkerSectionCommands);
    parsedOffset += sizeof(WalkerPartition::BATCH_BUFFER_START<FamilyType>);

    miSetPredicate = genCmdCast<WalkerPartition::MI_SET_PREDICATE<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miSetPredicate);
    EXPECT_EQ(miSetPredicate->getPredicateEnableWparid(), MI_SET_PREDICATE<FamilyType>::PREDICATE_ENABLE_WPARID::PREDICATE_ENABLE_WPARID_NOOP_NEVER);
    EXPECT_EQ(miSetPredicate->getPredicateEnable(), MI_SET_PREDICATE<FamilyType>::PREDICATE_ENABLE::PREDICATE_ENABLE_PREDICATE_DISABLE);
    parsedOffset += sizeof(WalkerPartition::MI_SET_PREDICATE<FamilyType>);

    uint64_t expectedCleanupGpuVa = gpuVirtualAddress + expectedCommandUsedSize + offsetof(BatchBufferControlData, finalSyncTileCount);
    auto finalSyncTileCountFieldStore = genCmdCast<WalkerPartition::MI_ATOMIC<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, finalSyncTileCountFieldStore);
    miAtomicProgrammedAddress = UnitTestHelper<FamilyType>::getAtomicMemoryAddress(*finalSyncTileCountFieldStore);
    EXPECT_EQ(expectedCleanupGpuVa, miAtomicProgrammedAddress);
    EXPECT_FALSE(finalSyncTileCountFieldStore->getReturnDataControl());
    EXPECT_EQ(MI_ATOMIC<FamilyType>::ATOMIC_OPCODES::ATOMIC_4B_MOVE, finalSyncTileCountFieldStore->getAtomicOpcode());
    parsedOffset += sizeof(WalkerPartition::MI_ATOMIC<FamilyType>);

    auto pipeControl = genCmdCast<WalkerPartition::PIPE_CONTROL<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, pipeControl);
    EXPECT_TRUE(pipeControl->getCommandStreamerStallEnable());
    EXPECT_EQ(MemorySynchronizationCommands<FamilyType>::getDcFlushEnable(true, *mockExecutionEnvironment.rootDeviceEnvironments[0]), pipeControl->getDcFlushEnable());
    parsedOffset += sizeof(WalkerPartition::PIPE_CONTROL<FamilyType>);

    miAtomic = genCmdCast<WalkerPartition::MI_ATOMIC<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miAtomic);
    auto miAtomicTileAddress = gpuVirtualAddress + expectedCommandUsedSize + sizeof(uint32_t);
    auto miAtomicTileProgrammedAddress = UnitTestHelper<FamilyType>::getAtomicMemoryAddress(*miAtomic);
    EXPECT_EQ(miAtomicTileAddress, miAtomicTileProgrammedAddress);
    EXPECT_FALSE(miAtomic->getReturnDataControl());
    EXPECT_EQ(MI_ATOMIC<FamilyType>::ATOMIC_OPCODES::ATOMIC_4B_INCREMENT, miAtomic->getAtomicOpcode());
    parsedOffset += sizeof(WalkerPartition::MI_ATOMIC<FamilyType>);

    auto miSemaphoreWait = genCmdCast<WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miSemaphoreWait);
    EXPECT_EQ(miSemaphoreWait->getSemaphoreGraphicsAddress(), miAtomicTileAddress);
    EXPECT_EQ(miSemaphoreWait->getCompareOperation(), MI_SEMAPHORE_WAIT<FamilyType>::COMPARE_OPERATION::COMPARE_OPERATION_SAD_GREATER_THAN_OR_EQUAL_SDD);
    EXPECT_EQ(miSemaphoreWait->getSemaphoreDataDword(), testArgs.tileCount);

    parsedOffset += sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>);

    // final batch buffer start that routes at the end of the batch buffer
    auto batchBufferStartFinal = genCmdCast<WalkerPartition::BATCH_BUFFER_START<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, batchBufferStartFinal);
    EXPECT_EQ(batchBufferStartFinal->getBatchBufferStartAddress(), gpuVirtualAddress + cleanupSectionOffset);
    parsedOffset += sizeof(WalkerPartition::BATCH_BUFFER_START<FamilyType>);

    auto computeWalker = genCmdCast<WalkerType *>(ptrOffset(cmdBuffer, parsedOffset));
    EXPECT_NE(nullptr, computeWalker);
    parsedOffset += sizeof(WalkerType);

    batchBufferStart = genCmdCast<WalkerPartition::BATCH_BUFFER_START<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, batchBufferStart);
    EXPECT_FALSE(batchBufferStart->getPredicationEnable());
    EXPECT_EQ(gpuVirtualAddress, batchBufferStart->getBatchBufferStartAddress());
    parsedOffset += sizeof(WalkerPartition::BATCH_BUFFER_START<FamilyType>);

    auto controlSection = reinterpret_cast<BatchBufferControlData *>(ptrOffset(cmdBuffer, expectedCommandUsedSize));
    EXPECT_EQ(0u, controlSection->partitionCount);
    EXPECT_EQ(0u, controlSection->tileCount);
    EXPECT_EQ(0u, controlSection->inTileCount);
    EXPECT_EQ(0u, controlSection->finalSyncTileCount);

    parsedOffset += sizeof(BatchBufferControlData);
    EXPECT_EQ(parsedOffset, cleanupSectionOffset);

    miAtomic = genCmdCast<WalkerPartition::MI_ATOMIC<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miAtomic);
    miAtomicTileAddress = gpuVirtualAddress + cleanupSectionOffset - sizeof(BatchBufferControlData) +
                          3 * sizeof(uint32_t);
    miAtomicTileProgrammedAddress = UnitTestHelper<FamilyType>::getAtomicMemoryAddress(*miAtomic);
    EXPECT_EQ(miAtomicTileAddress, miAtomicTileProgrammedAddress);
    EXPECT_FALSE(miAtomic->getReturnDataControl());
    EXPECT_EQ(MI_ATOMIC<FamilyType>::ATOMIC_OPCODES::ATOMIC_4B_INCREMENT, miAtomic->getAtomicOpcode());
    parsedOffset += sizeof(WalkerPartition::MI_ATOMIC<FamilyType>);

    miSemaphoreWait = genCmdCast<WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miSemaphoreWait);
    EXPECT_EQ(miSemaphoreWait->getSemaphoreGraphicsAddress(), miAtomicTileAddress);
    EXPECT_EQ(miSemaphoreWait->getCompareOperation(), MI_SEMAPHORE_WAIT<FamilyType>::COMPARE_OPERATION::COMPARE_OPERATION_SAD_GREATER_THAN_OR_EQUAL_SDD);
    EXPECT_EQ(miSemaphoreWait->getSemaphoreDataDword(), testArgs.tileCount);
    parsedOffset += sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>);

    expectedCleanupGpuVa = gpuVirtualAddress + cleanupSectionOffset - sizeof(BatchBufferControlData);
    auto partitionCountFieldStore = genCmdCast<WalkerPartition::MI_ATOMIC<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, partitionCountFieldStore);
    miAtomicProgrammedAddress = UnitTestHelper<FamilyType>::getAtomicMemoryAddress(*partitionCountFieldStore);
    EXPECT_EQ(expectedCleanupGpuVa, miAtomicProgrammedAddress);
    EXPECT_FALSE(partitionCountFieldStore->getReturnDataControl());
    EXPECT_EQ(MI_ATOMIC<FamilyType>::ATOMIC_OPCODES::ATOMIC_4B_MOVE, partitionCountFieldStore->getAtomicOpcode());
    parsedOffset += sizeof(WalkerPartition::MI_ATOMIC<FamilyType>);

    expectedCleanupGpuVa += sizeof(BatchBufferControlData::partitionCount);
    auto tileCountFieldStore = genCmdCast<WalkerPartition::MI_ATOMIC<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, tileCountFieldStore);
    miAtomicProgrammedAddress = UnitTestHelper<FamilyType>::getAtomicMemoryAddress(*tileCountFieldStore);
    EXPECT_EQ(expectedCleanupGpuVa, miAtomicProgrammedAddress);
    EXPECT_FALSE(tileCountFieldStore->getReturnDataControl());
    EXPECT_EQ(MI_ATOMIC<FamilyType>::ATOMIC_OPCODES::ATOMIC_4B_MOVE, tileCountFieldStore->getAtomicOpcode());
    parsedOffset += sizeof(WalkerPartition::MI_ATOMIC<FamilyType>);

    expectedCleanupGpuVa += sizeof(BatchBufferControlData::tileCount);
    auto inTileCountFieldStore = genCmdCast<WalkerPartition::MI_ATOMIC<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, inTileCountFieldStore);
    miAtomicProgrammedAddress = UnitTestHelper<FamilyType>::getAtomicMemoryAddress(*inTileCountFieldStore);
    EXPECT_EQ(expectedCleanupGpuVa, miAtomicProgrammedAddress);
    EXPECT_FALSE(inTileCountFieldStore->getReturnDataControl());
    EXPECT_EQ(MI_ATOMIC<FamilyType>::ATOMIC_OPCODES::ATOMIC_4B_MOVE, inTileCountFieldStore->getAtomicOpcode());
    parsedOffset += sizeof(WalkerPartition::MI_ATOMIC<FamilyType>);

    miAtomic = genCmdCast<WalkerPartition::MI_ATOMIC<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miAtomic);
    miAtomicTileProgrammedAddress = UnitTestHelper<FamilyType>::getAtomicMemoryAddress(*miAtomic);
    EXPECT_EQ(miAtomicTileAddress, miAtomicTileProgrammedAddress);
    EXPECT_FALSE(miAtomic->getReturnDataControl());
    EXPECT_EQ(MI_ATOMIC<FamilyType>::ATOMIC_OPCODES::ATOMIC_4B_INCREMENT, miAtomic->getAtomicOpcode());
    parsedOffset += sizeof(WalkerPartition::MI_ATOMIC<FamilyType>);

    miSemaphoreWait = genCmdCast<WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miSemaphoreWait);
    EXPECT_EQ(miSemaphoreWait->getSemaphoreGraphicsAddress(), miAtomicTileAddress);
    EXPECT_EQ(miSemaphoreWait->getCompareOperation(), MI_SEMAPHORE_WAIT<FamilyType>::COMPARE_OPERATION::COMPARE_OPERATION_SAD_GREATER_THAN_OR_EQUAL_SDD);
    EXPECT_EQ(miSemaphoreWait->getSemaphoreDataDword(), 2 * testArgs.tileCount);
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenDynamicPartitioningWhenPipeControlProgrammingDisabledThenExpectNoPipeControlCommand) {
    using WalkerType = typename FamilyType::DefaultWalkerType;
    using PostSyncType = decltype(FamilyType::template getPostSyncType<WalkerType>());

    testArgs.crossTileAtomicSynchronization = false;
    testArgs.partitionCount = 16u;
    testArgs.tileCount = 4u;
    testArgs.emitSelfCleanup = false;
    testArgs.useAtomicsForSelfCleanup = false;
    testArgs.emitPipeControlStall = false;

    checkForProperCmdBufferAddressOffset = false;
    uint64_t gpuVirtualAddress = 0x8000123000;
    uint64_t postSyncAddress = 0x8000456000;
    WalkerType walker;
    walker = FamilyType::template getInitGpuWalker<WalkerType>();
    walker.setPartitionType(WalkerType::PARTITION_TYPE::PARTITION_TYPE_X);
    auto &postSync = walker.getPostSync();
    postSync.setOperation(PostSyncType::OPERATION::OPERATION_WRITE_TIMESTAMP);
    postSync.setDestinationAddress(postSyncAddress);
    uint32_t totalBytesProgrammed = 0u;

    auto expectedCommandUsedSize = sizeof(WalkerPartition::LOAD_REGISTER_IMM<FamilyType>) +
                                   sizeof(WalkerPartition::MI_ATOMIC<FamilyType>) +
                                   sizeof(WalkerPartition::LOAD_REGISTER_REG<FamilyType>) +
                                   sizeof(WalkerPartition::MI_SET_PREDICATE<FamilyType>) * 2 +
                                   sizeof(WalkerPartition::BATCH_BUFFER_START<FamilyType>) * 3 +
                                   sizeof(WalkerType);

    auto walkerSectionCommands = sizeof(WalkerPartition::BATCH_BUFFER_START<FamilyType>) +
                                 sizeof(WalkerType);

    auto offset = computeControlSectionOffset<FamilyType, WalkerType>(testArgs);
    EXPECT_EQ(expectedCommandUsedSize, offset);

    auto cleanupSectionOffset = expectedCommandUsedSize + sizeof(BatchBufferControlData);

    auto totalProgrammedSize = cleanupSectionOffset;

    auto device = std::unique_ptr<MockDevice>(MockDevice::createWithNewExecutionEnvironment<MockDevice>(defaultHwInfo.get()));

    WalkerPartition::constructDynamicallyPartitionedCommandBuffer<FamilyType>(cmdBuffer,
                                                                              nullptr,
                                                                              gpuVirtualAddress,
                                                                              &walker,
                                                                              totalBytesProgrammed,
                                                                              testArgs,
                                                                              *device);

    EXPECT_EQ(totalProgrammedSize, totalBytesProgrammed);
    auto wparidMaskProgrammingLocation = cmdBufferAddress;

    auto expectedMask = 0xFFF0u;
    auto expectedRegister = 0x21FCu;

    auto loadRegisterImmediate = genCmdCast<WalkerPartition::LOAD_REGISTER_IMM<FamilyType> *>(wparidMaskProgrammingLocation);
    ASSERT_NE(nullptr, loadRegisterImmediate);
    EXPECT_EQ(expectedRegister, loadRegisterImmediate->getRegisterOffset());
    EXPECT_EQ(expectedMask, loadRegisterImmediate->getDataDword());
    auto parsedOffset = sizeof(WalkerPartition::LOAD_REGISTER_IMM<FamilyType>);

    auto miAtomic = genCmdCast<WalkerPartition::MI_ATOMIC<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miAtomic);
    auto miAtomicAddress = gpuVirtualAddress + expectedCommandUsedSize;
    auto miAtomicProgrammedAddress = UnitTestHelper<FamilyType>::getAtomicMemoryAddress(*miAtomic);
    EXPECT_EQ(miAtomicAddress, miAtomicProgrammedAddress);
    EXPECT_TRUE(miAtomic->getReturnDataControl());
    EXPECT_EQ(MI_ATOMIC<FamilyType>::ATOMIC_OPCODES::ATOMIC_4B_INCREMENT, miAtomic->getAtomicOpcode());
    parsedOffset += sizeof(WalkerPartition::MI_ATOMIC<FamilyType>);

    auto loadRegisterReg = genCmdCast<WalkerPartition::LOAD_REGISTER_REG<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, loadRegisterReg);
    EXPECT_TRUE(loadRegisterReg->getMmioRemapEnableDestination());
    EXPECT_TRUE(loadRegisterReg->getMmioRemapEnableSource());
    EXPECT_EQ(wparidCCSOffset, loadRegisterReg->getDestinationRegisterAddress());
    EXPECT_EQ(generalPurposeRegister4, loadRegisterReg->getSourceRegisterAddress());
    parsedOffset += sizeof(WalkerPartition::LOAD_REGISTER_REG<FamilyType>);

    auto miSetPredicate = genCmdCast<WalkerPartition::MI_SET_PREDICATE<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miSetPredicate);
    EXPECT_EQ(miSetPredicate->getPredicateEnableWparid(), MI_SET_PREDICATE<FamilyType>::PREDICATE_ENABLE_WPARID::PREDICATE_ENABLE_WPARID_NOOP_ON_NON_ZERO_VALUE);
    parsedOffset += sizeof(WalkerPartition::MI_SET_PREDICATE<FamilyType>);

    auto batchBufferStart = genCmdCast<WalkerPartition::BATCH_BUFFER_START<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, batchBufferStart);
    EXPECT_TRUE(batchBufferStart->getPredicationEnable());
    // address routes to WALKER section which is before control section
    auto address = batchBufferStart->getBatchBufferStartAddress();
    EXPECT_EQ(address, gpuVirtualAddress + expectedCommandUsedSize - walkerSectionCommands);
    parsedOffset += sizeof(WalkerPartition::BATCH_BUFFER_START<FamilyType>);

    miSetPredicate = genCmdCast<WalkerPartition::MI_SET_PREDICATE<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miSetPredicate);
    EXPECT_EQ(miSetPredicate->getPredicateEnableWparid(), MI_SET_PREDICATE<FamilyType>::PREDICATE_ENABLE_WPARID::PREDICATE_ENABLE_WPARID_NOOP_NEVER);
    EXPECT_EQ(miSetPredicate->getPredicateEnable(), MI_SET_PREDICATE<FamilyType>::PREDICATE_ENABLE::PREDICATE_ENABLE_PREDICATE_DISABLE);
    parsedOffset += sizeof(WalkerPartition::MI_SET_PREDICATE<FamilyType>);

    // final batch buffer start that routes at the end of the batch buffer
    auto batchBufferStartFinal = genCmdCast<WalkerPartition::BATCH_BUFFER_START<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, batchBufferStartFinal);
    EXPECT_EQ(batchBufferStartFinal->getBatchBufferStartAddress(), gpuVirtualAddress + cleanupSectionOffset);
    parsedOffset += sizeof(WalkerPartition::BATCH_BUFFER_START<FamilyType>);

    auto computeWalker = genCmdCast<WalkerType *>(ptrOffset(cmdBuffer, parsedOffset));
    EXPECT_NE(nullptr, computeWalker);
    parsedOffset += sizeof(WalkerType);

    batchBufferStart = genCmdCast<WalkerPartition::BATCH_BUFFER_START<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, batchBufferStart);
    EXPECT_FALSE(batchBufferStart->getPredicationEnable());
    EXPECT_EQ(gpuVirtualAddress, batchBufferStart->getBatchBufferStartAddress());
    parsedOffset += sizeof(WalkerPartition::BATCH_BUFFER_START<FamilyType>);

    auto controlSection = reinterpret_cast<BatchBufferControlData *>(ptrOffset(cmdBuffer, expectedCommandUsedSize));
    EXPECT_EQ(0u, controlSection->partitionCount);
    EXPECT_EQ(0u, controlSection->tileCount);
    EXPECT_EQ(0u, controlSection->inTileCount);
    EXPECT_EQ(0u, controlSection->finalSyncTileCount);

    parsedOffset += sizeof(BatchBufferControlData);
    EXPECT_EQ(parsedOffset, cleanupSectionOffset);
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenBarrierProgrammingWhenDoNotEmitSelfCleanupThenExpectNoCleanupSection) {
    testArgs.tileCount = 4u;
    testArgs.emitSelfCleanup = false;

    MockExecutionEnvironment mockExecutionEnvironment{};
    auto &rootDeviceEnvironment = *mockExecutionEnvironment.rootDeviceEnvironments[0];
    uint32_t totalBytesProgrammed = 0u;
    uint64_t gpuVirtualAddress = 0xFF0000;

    auto expectedOffsetSectionSize = sizeof(WalkerPartition::PIPE_CONTROL<FamilyType>) +
                                     sizeof(WalkerPartition::MI_ATOMIC<FamilyType>) + sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>) +
                                     sizeof(WalkerPartition::BATCH_BUFFER_START<FamilyType>);

    auto expectedCommandUsedSize = expectedOffsetSectionSize +
                                   sizeof(BarrierControlSection);

    EXPECT_EQ(expectedOffsetSectionSize, computeBarrierControlSectionOffset<FamilyType>(testArgs, rootDeviceEnvironment));
    EXPECT_EQ(expectedCommandUsedSize, estimateBarrierSpaceRequiredInCommandBuffer<FamilyType>(testArgs, rootDeviceEnvironment));

    PipeControlArgs flushArgs;
    flushArgs.dcFlushEnable = false;
    WalkerPartition::constructBarrierCommandBuffer<FamilyType>(cmdBuffer,
                                                               gpuVirtualAddress,
                                                               totalBytesProgrammed,
                                                               testArgs,
                                                               flushArgs,
                                                               rootDeviceEnvironment);

    EXPECT_EQ(expectedCommandUsedSize, totalBytesProgrammed);

    auto pipeControl = genCmdCast<WalkerPartition::PIPE_CONTROL<FamilyType> *>(cmdBufferAddress);
    ASSERT_NE(nullptr, pipeControl);
    EXPECT_TRUE(pipeControl->getCommandStreamerStallEnable());
    EXPECT_FALSE(pipeControl->getDcFlushEnable());
    auto parsedOffset = sizeof(WalkerPartition::PIPE_CONTROL<FamilyType>);

    auto miAtomic = genCmdCast<WalkerPartition::MI_ATOMIC<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miAtomic);
    auto crossTileSyncAddress = gpuVirtualAddress + expectedOffsetSectionSize + offsetof(BarrierControlSection, crossTileSyncCount);
    auto miAtomicProgrammedAddress = UnitTestHelper<FamilyType>::getAtomicMemoryAddress(*miAtomic);
    EXPECT_EQ(crossTileSyncAddress, miAtomicProgrammedAddress);
    EXPECT_FALSE(miAtomic->getReturnDataControl());
    EXPECT_EQ(MI_ATOMIC<FamilyType>::ATOMIC_OPCODES::ATOMIC_4B_INCREMENT, miAtomic->getAtomicOpcode());
    parsedOffset += sizeof(WalkerPartition::MI_ATOMIC<FamilyType>);

    auto miSemaphoreWait = genCmdCast<WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miSemaphoreWait);
    EXPECT_EQ(crossTileSyncAddress, miSemaphoreWait->getSemaphoreGraphicsAddress());
    EXPECT_EQ(MI_SEMAPHORE_WAIT<FamilyType>::COMPARE_OPERATION::COMPARE_OPERATION_SAD_GREATER_THAN_OR_EQUAL_SDD, miSemaphoreWait->getCompareOperation());
    EXPECT_EQ(testArgs.tileCount, miSemaphoreWait->getSemaphoreDataDword());
    parsedOffset += sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>);

    auto batchBufferStart = genCmdCast<WalkerPartition::BATCH_BUFFER_START<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, batchBufferStart);
    EXPECT_EQ(gpuVirtualAddress + expectedOffsetSectionSize + sizeof(BarrierControlSection), batchBufferStart->getBatchBufferStartAddress());
    EXPECT_EQ(BATCH_BUFFER_START<FamilyType>::SECOND_LEVEL_BATCH_BUFFER::SECOND_LEVEL_BATCH_BUFFER_FIRST_LEVEL_BATCH, batchBufferStart->getSecondLevelBatchBuffer());
    parsedOffset += sizeof(WalkerPartition::BATCH_BUFFER_START<FamilyType>);

    auto controlSection = reinterpret_cast<BarrierControlSection *>(ptrOffset(cmdBuffer, parsedOffset));
    EXPECT_EQ(0u, controlSection->crossTileSyncCount);
    EXPECT_EQ(0u, controlSection->finalSyncTileCount);
    parsedOffset += sizeof(BarrierControlSection);

    EXPECT_EQ(parsedOffset, expectedCommandUsedSize);
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenBarrierProgrammingWhenEmitsSelfCleanupThenExpectStoreDataImmCommandCleanupSection) {
    testArgs.tileCount = 4u;
    testArgs.emitSelfCleanup = true;
    testArgs.secondaryBatchBuffer = true;

    uint32_t totalBytesProgrammed = 0u;
    uint64_t gpuVirtualAddress = 0xFF0000;
    MockExecutionEnvironment mockExecutionEnvironment{};
    auto &rootDeviceEnvironment = *mockExecutionEnvironment.rootDeviceEnvironments[0];
    auto expectedOffsetSectionSize = sizeof(WalkerPartition::MI_STORE_DATA_IMM<FamilyType>) +
                                     sizeof(WalkerPartition::PIPE_CONTROL<FamilyType>) +
                                     sizeof(WalkerPartition::MI_ATOMIC<FamilyType>) + sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>) +
                                     sizeof(WalkerPartition::BATCH_BUFFER_START<FamilyType>);

    auto expectedCommandUsedSize = expectedOffsetSectionSize +
                                   sizeof(BarrierControlSection) +
                                   sizeof(WalkerPartition::MI_ATOMIC<FamilyType>) + sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>) +
                                   sizeof(WalkerPartition::MI_STORE_DATA_IMM<FamilyType>) +
                                   sizeof(WalkerPartition::MI_ATOMIC<FamilyType>) + sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>);

    EXPECT_EQ(expectedOffsetSectionSize, computeBarrierControlSectionOffset<FamilyType>(testArgs, rootDeviceEnvironment));
    EXPECT_EQ(expectedCommandUsedSize, estimateBarrierSpaceRequiredInCommandBuffer<FamilyType>(testArgs, rootDeviceEnvironment));

    PipeControlArgs flushArgs;
    flushArgs.dcFlushEnable = true;
    WalkerPartition::constructBarrierCommandBuffer<FamilyType>(cmdBuffer,
                                                               gpuVirtualAddress,
                                                               totalBytesProgrammed,
                                                               testArgs,
                                                               flushArgs,
                                                               rootDeviceEnvironment);

    EXPECT_EQ(expectedCommandUsedSize, totalBytesProgrammed);

    size_t parsedOffset = 0;

    uint64_t finalSyncTileCountAddress = gpuVirtualAddress + expectedOffsetSectionSize + offsetof(BarrierControlSection, finalSyncTileCount);
    constexpr uint32_t expectedData = 0u;

    auto finalSyncTileCountFieldStore = genCmdCast<WalkerPartition::MI_STORE_DATA_IMM<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, finalSyncTileCountFieldStore);
    EXPECT_EQ(finalSyncTileCountAddress, finalSyncTileCountFieldStore->getAddress());
    EXPECT_EQ(expectedData, finalSyncTileCountFieldStore->getDataDword0());
    parsedOffset += sizeof(WalkerPartition::MI_STORE_DATA_IMM<FamilyType>);

    auto pipeControl = genCmdCast<WalkerPartition::PIPE_CONTROL<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, pipeControl);
    EXPECT_TRUE(pipeControl->getCommandStreamerStallEnable());
    EXPECT_TRUE(pipeControl->getDcFlushEnable());
    parsedOffset += sizeof(WalkerPartition::PIPE_CONTROL<FamilyType>);

    auto miAtomic = genCmdCast<WalkerPartition::MI_ATOMIC<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miAtomic);
    auto crossTileSyncAddress = gpuVirtualAddress + expectedOffsetSectionSize + offsetof(BarrierControlSection, crossTileSyncCount);
    auto miAtomicProgrammedAddress = UnitTestHelper<FamilyType>::getAtomicMemoryAddress(*miAtomic);
    EXPECT_EQ(crossTileSyncAddress, miAtomicProgrammedAddress);
    EXPECT_FALSE(miAtomic->getReturnDataControl());
    EXPECT_EQ(MI_ATOMIC<FamilyType>::ATOMIC_OPCODES::ATOMIC_4B_INCREMENT, miAtomic->getAtomicOpcode());
    parsedOffset += sizeof(WalkerPartition::MI_ATOMIC<FamilyType>);

    auto miSemaphoreWait = genCmdCast<WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miSemaphoreWait);
    EXPECT_EQ(crossTileSyncAddress, miSemaphoreWait->getSemaphoreGraphicsAddress());
    EXPECT_EQ(MI_SEMAPHORE_WAIT<FamilyType>::COMPARE_OPERATION::COMPARE_OPERATION_SAD_GREATER_THAN_OR_EQUAL_SDD, miSemaphoreWait->getCompareOperation());
    EXPECT_EQ(testArgs.tileCount, miSemaphoreWait->getSemaphoreDataDword());
    parsedOffset += sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>);

    auto batchBufferStart = genCmdCast<WalkerPartition::BATCH_BUFFER_START<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, batchBufferStart);
    EXPECT_EQ(gpuVirtualAddress + expectedOffsetSectionSize + sizeof(BarrierControlSection), batchBufferStart->getBatchBufferStartAddress());
    EXPECT_EQ(BATCH_BUFFER_START<FamilyType>::SECOND_LEVEL_BATCH_BUFFER::SECOND_LEVEL_BATCH_BUFFER_SECOND_LEVEL_BATCH, batchBufferStart->getSecondLevelBatchBuffer());
    parsedOffset += sizeof(WalkerPartition::BATCH_BUFFER_START<FamilyType>);

    auto controlSection = reinterpret_cast<BarrierControlSection *>(ptrOffset(cmdBuffer, parsedOffset));
    EXPECT_EQ(0u, controlSection->crossTileSyncCount);
    EXPECT_EQ(0u, controlSection->finalSyncTileCount);
    parsedOffset += sizeof(BarrierControlSection);

    miAtomic = genCmdCast<WalkerPartition::MI_ATOMIC<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miAtomic);
    miAtomicProgrammedAddress = UnitTestHelper<FamilyType>::getAtomicMemoryAddress(*miAtomic);
    EXPECT_EQ(finalSyncTileCountAddress, miAtomicProgrammedAddress);
    EXPECT_FALSE(miAtomic->getReturnDataControl());
    EXPECT_EQ(MI_ATOMIC<FamilyType>::ATOMIC_OPCODES::ATOMIC_4B_INCREMENT, miAtomic->getAtomicOpcode());
    parsedOffset += sizeof(WalkerPartition::MI_ATOMIC<FamilyType>);

    miSemaphoreWait = genCmdCast<WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miSemaphoreWait);
    EXPECT_EQ(finalSyncTileCountAddress, miSemaphoreWait->getSemaphoreGraphicsAddress());
    EXPECT_EQ(MI_SEMAPHORE_WAIT<FamilyType>::COMPARE_OPERATION::COMPARE_OPERATION_SAD_GREATER_THAN_OR_EQUAL_SDD, miSemaphoreWait->getCompareOperation());
    EXPECT_EQ(testArgs.tileCount, miSemaphoreWait->getSemaphoreDataDword());
    parsedOffset += sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>);

    auto crossTileFieldStore = genCmdCast<WalkerPartition::MI_STORE_DATA_IMM<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, crossTileFieldStore);
    EXPECT_EQ(crossTileSyncAddress, crossTileFieldStore->getAddress());
    EXPECT_EQ(expectedData, crossTileFieldStore->getDataDword0());
    parsedOffset += sizeof(WalkerPartition::MI_STORE_DATA_IMM<FamilyType>);

    miAtomic = genCmdCast<WalkerPartition::MI_ATOMIC<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miAtomic);
    miAtomicProgrammedAddress = UnitTestHelper<FamilyType>::getAtomicMemoryAddress(*miAtomic);
    EXPECT_EQ(finalSyncTileCountAddress, miAtomicProgrammedAddress);
    EXPECT_FALSE(miAtomic->getReturnDataControl());
    EXPECT_EQ(MI_ATOMIC<FamilyType>::ATOMIC_OPCODES::ATOMIC_4B_INCREMENT, miAtomic->getAtomicOpcode());
    parsedOffset += sizeof(WalkerPartition::MI_ATOMIC<FamilyType>);

    miSemaphoreWait = genCmdCast<WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miSemaphoreWait);
    EXPECT_EQ(finalSyncTileCountAddress, miSemaphoreWait->getSemaphoreGraphicsAddress());
    EXPECT_EQ(MI_SEMAPHORE_WAIT<FamilyType>::COMPARE_OPERATION::COMPARE_OPERATION_SAD_GREATER_THAN_OR_EQUAL_SDD, miSemaphoreWait->getCompareOperation());
    EXPECT_EQ(2u * testArgs.tileCount, miSemaphoreWait->getSemaphoreDataDword());
    parsedOffset += sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>);

    EXPECT_EQ(parsedOffset, expectedCommandUsedSize);
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenBarrierProgrammingWhenEmitsSelfCleanupUsingAtomicThenExpectMiAtomicCommandCleanupSection) {
    testArgs.tileCount = 4u;
    testArgs.emitSelfCleanup = true;
    testArgs.secondaryBatchBuffer = true;
    testArgs.useAtomicsForSelfCleanup = true;

    uint32_t totalBytesProgrammed = 0u;
    uint64_t gpuVirtualAddress = 0xFF0000;
    MockExecutionEnvironment mockExecutionEnvironment{};
    auto &rootDeviceEnvironment = *mockExecutionEnvironment.rootDeviceEnvironments[0];
    auto expectedOffsetSectionSize = sizeof(WalkerPartition::MI_ATOMIC<FamilyType>) +
                                     sizeof(WalkerPartition::PIPE_CONTROL<FamilyType>) +
                                     sizeof(WalkerPartition::MI_ATOMIC<FamilyType>) + sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>) +
                                     sizeof(WalkerPartition::BATCH_BUFFER_START<FamilyType>);

    auto expectedCommandUsedSize = expectedOffsetSectionSize +
                                   sizeof(BarrierControlSection) +
                                   sizeof(WalkerPartition::MI_ATOMIC<FamilyType>) + sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>) +
                                   sizeof(WalkerPartition::MI_ATOMIC<FamilyType>) +
                                   sizeof(WalkerPartition::MI_ATOMIC<FamilyType>) + sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>);

    EXPECT_EQ(expectedOffsetSectionSize, computeBarrierControlSectionOffset<FamilyType>(testArgs, rootDeviceEnvironment));
    EXPECT_EQ(expectedCommandUsedSize, estimateBarrierSpaceRequiredInCommandBuffer<FamilyType>(testArgs, rootDeviceEnvironment));

    PipeControlArgs flushArgs;
    flushArgs.dcFlushEnable = true;
    WalkerPartition::constructBarrierCommandBuffer<FamilyType>(cmdBuffer,
                                                               gpuVirtualAddress,
                                                               totalBytesProgrammed,
                                                               testArgs,
                                                               flushArgs,
                                                               rootDeviceEnvironment);

    EXPECT_EQ(expectedCommandUsedSize, totalBytesProgrammed);

    size_t parsedOffset = 0;

    uint64_t finalSyncTileCountAddress = gpuVirtualAddress + expectedOffsetSectionSize + offsetof(BarrierControlSection, finalSyncTileCount);
    constexpr uint32_t expectedData = 0u;

    auto finalSyncTileCountFieldAtomic = genCmdCast<WalkerPartition::MI_ATOMIC<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, finalSyncTileCountFieldAtomic);
    auto miAtomicProgrammedAddress = UnitTestHelper<FamilyType>::getAtomicMemoryAddress(*finalSyncTileCountFieldAtomic);
    EXPECT_EQ(finalSyncTileCountAddress, miAtomicProgrammedAddress);
    EXPECT_FALSE(finalSyncTileCountFieldAtomic->getReturnDataControl());
    EXPECT_EQ(MI_ATOMIC<FamilyType>::ATOMIC_OPCODES::ATOMIC_4B_MOVE, finalSyncTileCountFieldAtomic->getAtomicOpcode());
    EXPECT_EQ(MI_ATOMIC<FamilyType>::DWORD_LENGTH::DWORD_LENGTH_INLINE_DATA_1, finalSyncTileCountFieldAtomic->getDwordLength());
    EXPECT_TRUE(finalSyncTileCountFieldAtomic->getInlineData());
    EXPECT_EQ(expectedData, finalSyncTileCountFieldAtomic->getOperand1DataDword0());
    parsedOffset += sizeof(WalkerPartition::MI_ATOMIC<FamilyType>);

    auto pipeControl = genCmdCast<WalkerPartition::PIPE_CONTROL<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, pipeControl);
    EXPECT_TRUE(pipeControl->getCommandStreamerStallEnable());
    EXPECT_TRUE(pipeControl->getDcFlushEnable());
    parsedOffset += sizeof(WalkerPartition::PIPE_CONTROL<FamilyType>);

    auto miAtomic = genCmdCast<WalkerPartition::MI_ATOMIC<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miAtomic);
    auto crossTileSyncAddress = gpuVirtualAddress + expectedOffsetSectionSize + offsetof(BarrierControlSection, crossTileSyncCount);
    miAtomicProgrammedAddress = UnitTestHelper<FamilyType>::getAtomicMemoryAddress(*miAtomic);
    EXPECT_EQ(crossTileSyncAddress, miAtomicProgrammedAddress);
    EXPECT_FALSE(miAtomic->getReturnDataControl());
    EXPECT_EQ(MI_ATOMIC<FamilyType>::ATOMIC_OPCODES::ATOMIC_4B_INCREMENT, miAtomic->getAtomicOpcode());
    parsedOffset += sizeof(WalkerPartition::MI_ATOMIC<FamilyType>);

    auto miSemaphoreWait = genCmdCast<WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miSemaphoreWait);
    EXPECT_EQ(crossTileSyncAddress, miSemaphoreWait->getSemaphoreGraphicsAddress());
    EXPECT_EQ(MI_SEMAPHORE_WAIT<FamilyType>::COMPARE_OPERATION::COMPARE_OPERATION_SAD_GREATER_THAN_OR_EQUAL_SDD, miSemaphoreWait->getCompareOperation());
    EXPECT_EQ(testArgs.tileCount, miSemaphoreWait->getSemaphoreDataDword());
    parsedOffset += sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>);

    auto batchBufferStart = genCmdCast<WalkerPartition::BATCH_BUFFER_START<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, batchBufferStart);
    EXPECT_EQ(gpuVirtualAddress + expectedOffsetSectionSize + sizeof(BarrierControlSection), batchBufferStart->getBatchBufferStartAddress());
    EXPECT_EQ(BATCH_BUFFER_START<FamilyType>::SECOND_LEVEL_BATCH_BUFFER::SECOND_LEVEL_BATCH_BUFFER_SECOND_LEVEL_BATCH, batchBufferStart->getSecondLevelBatchBuffer());
    parsedOffset += sizeof(WalkerPartition::BATCH_BUFFER_START<FamilyType>);

    auto controlSection = reinterpret_cast<BarrierControlSection *>(ptrOffset(cmdBuffer, parsedOffset));
    EXPECT_EQ(0u, controlSection->crossTileSyncCount);
    EXPECT_EQ(0u, controlSection->finalSyncTileCount);
    parsedOffset += sizeof(BarrierControlSection);

    miAtomic = genCmdCast<WalkerPartition::MI_ATOMIC<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miAtomic);
    miAtomicProgrammedAddress = UnitTestHelper<FamilyType>::getAtomicMemoryAddress(*miAtomic);
    EXPECT_EQ(finalSyncTileCountAddress, miAtomicProgrammedAddress);
    EXPECT_FALSE(miAtomic->getReturnDataControl());
    EXPECT_EQ(MI_ATOMIC<FamilyType>::ATOMIC_OPCODES::ATOMIC_4B_INCREMENT, miAtomic->getAtomicOpcode());
    parsedOffset += sizeof(WalkerPartition::MI_ATOMIC<FamilyType>);

    miSemaphoreWait = genCmdCast<WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miSemaphoreWait);
    EXPECT_EQ(finalSyncTileCountAddress, miSemaphoreWait->getSemaphoreGraphicsAddress());
    EXPECT_EQ(MI_SEMAPHORE_WAIT<FamilyType>::COMPARE_OPERATION::COMPARE_OPERATION_SAD_GREATER_THAN_OR_EQUAL_SDD, miSemaphoreWait->getCompareOperation());
    EXPECT_EQ(testArgs.tileCount, miSemaphoreWait->getSemaphoreDataDword());
    parsedOffset += sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>);

    auto crossTileFieldAtomic = genCmdCast<WalkerPartition::MI_ATOMIC<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, crossTileFieldAtomic);
    miAtomicProgrammedAddress = UnitTestHelper<FamilyType>::getAtomicMemoryAddress(*crossTileFieldAtomic);
    EXPECT_EQ(crossTileSyncAddress, miAtomicProgrammedAddress);
    EXPECT_FALSE(crossTileFieldAtomic->getReturnDataControl());
    EXPECT_EQ(MI_ATOMIC<FamilyType>::ATOMIC_OPCODES::ATOMIC_4B_MOVE, crossTileFieldAtomic->getAtomicOpcode());
    EXPECT_EQ(MI_ATOMIC<FamilyType>::DWORD_LENGTH::DWORD_LENGTH_INLINE_DATA_1, crossTileFieldAtomic->getDwordLength());
    EXPECT_TRUE(crossTileFieldAtomic->getInlineData());
    EXPECT_EQ(expectedData, crossTileFieldAtomic->getOperand1DataDword0());
    parsedOffset += sizeof(WalkerPartition::MI_ATOMIC<FamilyType>);

    miAtomic = genCmdCast<WalkerPartition::MI_ATOMIC<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miAtomic);
    miAtomicProgrammedAddress = UnitTestHelper<FamilyType>::getAtomicMemoryAddress(*miAtomic);
    EXPECT_EQ(finalSyncTileCountAddress, miAtomicProgrammedAddress);
    EXPECT_FALSE(miAtomic->getReturnDataControl());
    EXPECT_EQ(MI_ATOMIC<FamilyType>::ATOMIC_OPCODES::ATOMIC_4B_INCREMENT, miAtomic->getAtomicOpcode());
    parsedOffset += sizeof(WalkerPartition::MI_ATOMIC<FamilyType>);

    miSemaphoreWait = genCmdCast<WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType> *>(ptrOffset(cmdBuffer, parsedOffset));
    ASSERT_NE(nullptr, miSemaphoreWait);
    EXPECT_EQ(finalSyncTileCountAddress, miSemaphoreWait->getSemaphoreGraphicsAddress());
    EXPECT_EQ(MI_SEMAPHORE_WAIT<FamilyType>::COMPARE_OPERATION::COMPARE_OPERATION_SAD_GREATER_THAN_OR_EQUAL_SDD, miSemaphoreWait->getCompareOperation());
    EXPECT_EQ(2u * testArgs.tileCount, miSemaphoreWait->getSemaphoreDataDword());
    parsedOffset += sizeof(WalkerPartition::MI_SEMAPHORE_WAIT<FamilyType>);

    EXPECT_EQ(parsedOffset, expectedCommandUsedSize);
}

HWCMDTEST_F(IGFX_XE_HP_CORE, WalkerPartitionTests, givenForceExecutionOnSingleTileWhenProgramComputeWalkerThenWalkerIsProperlyProgrammed) {
    using WalkerType = typename FamilyType::DefaultWalkerType;
    WalkerType walker;
    walker = FamilyType::template getInitGpuWalker<WalkerType>();
    walker.setThreadGroupIdXDimension(32u);
    walker.setThreadGroupIdYDimension(1u);
    walker.setThreadGroupIdZDimension(1u);

    auto device = std::unique_ptr<NEO::MockDevice>(NEO::MockDevice::createWithNewExecutionEnvironment<NEO::MockDevice>(NEO::defaultHwInfo.get()));

    bool forceExecutionOnSingleTile = false;
    walker.setPartitionType(WalkerType::PARTITION_TYPE::PARTITION_TYPE_X);
    void *walkerCommandAddress = cmdBufferAddress;
    WalkerPartition::WalkerPartitionArgs args = {};
    args.partitionCount = 2;
    args.tileCount = 2;
    args.threadGroupCount = 32;
    args.forceExecutionOnSingleTile = forceExecutionOnSingleTile;
    programPartitionedWalker<FamilyType>(cmdBufferAddress, totalBytesProgrammed, &walker, args, *device);
    auto walkerCommand = genCmdCast<WalkerType *>(walkerCommandAddress);

    ASSERT_NE(nullptr, walkerCommand);
    EXPECT_TRUE(walkerCommand->getWorkloadPartitionEnable());
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_X, walkerCommand->getPartitionType());
    EXPECT_EQ(16u, walkerCommand->getPartitionSize());

    forceExecutionOnSingleTile = true;
    walkerCommandAddress = cmdBufferAddress;
    args = {};
    args.partitionCount = 2;
    args.tileCount = 2;
    args.threadGroupCount = 32;
    args.forceExecutionOnSingleTile = forceExecutionOnSingleTile;
    programPartitionedWalker<FamilyType>(cmdBufferAddress, totalBytesProgrammed, &walker, args, *device);
    walkerCommand = genCmdCast<WalkerType *>(walkerCommandAddress);

    ASSERT_NE(nullptr, walkerCommand);
    EXPECT_TRUE(walkerCommand->getWorkloadPartitionEnable());
    EXPECT_EQ(WalkerType::PARTITION_TYPE::PARTITION_TYPE_X, walkerCommand->getPartitionType());
    EXPECT_EQ(32u, walkerCommand->getPartitionSize());
}
