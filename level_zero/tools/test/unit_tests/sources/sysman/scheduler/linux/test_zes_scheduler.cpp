/*
 * Copyright (C) 2020-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "level_zero/tools/test/unit_tests/sources/sysman/linux/mock_sysman_fixture.h"

#include "mock_sysfs_scheduler.h"

extern bool sysmanUltsEnable;

namespace L0 {
namespace ult {
constexpr uint32_t handleComponentCount = 4u;
constexpr uint64_t convertMilliToMicro = 1000u;
constexpr uint64_t defaultTimeoutMilliSecs = 650u;
constexpr uint64_t defaultTimesliceMilliSecs = 1u;
constexpr uint64_t defaultHeartbeatMilliSecs = 3000u;
constexpr uint64_t timeoutMilliSecs = 640u;
constexpr uint64_t timesliceMilliSecs = 1u;
constexpr uint64_t heartbeatMilliSecs = 2500u;
constexpr uint64_t expectedDefaultHeartbeatTimeoutMicroSecs = defaultHeartbeatMilliSecs * convertMilliToMicro;
constexpr uint64_t expectedDefaultTimeoutMicroSecs = defaultTimeoutMilliSecs * convertMilliToMicro;
constexpr uint64_t expectedDefaultTimesliceMicroSecs = defaultTimesliceMilliSecs * convertMilliToMicro;
constexpr uint64_t expectedHeartbeatTimeoutMicroSecs = heartbeatMilliSecs * convertMilliToMicro;
constexpr uint64_t expectedTimeoutMicroSecs = timeoutMilliSecs * convertMilliToMicro;
constexpr uint64_t expectedTimesliceMicroSecs = timesliceMilliSecs * convertMilliToMicro;

class SysmanDeviceSchedulerFixture : public SysmanDeviceFixture {

  protected:
    std::unique_ptr<MockSchedulerSysfsAccess> pSysfsAccess;
    SysfsAccess *pSysfsAccessOld = nullptr;
    std::vector<ze_device_handle_t> deviceHandles;

    void SetUp() override {
        if (!sysmanUltsEnable) {
            GTEST_SKIP();
        }
        SysmanDeviceFixture::SetUp();
        pSysfsAccessOld = pLinuxSysmanImp->pSysfsAccess;
        pSysfsAccess = std::make_unique<MockSchedulerSysfsAccess>();
        pLinuxSysmanImp->pSysfsAccess = pSysfsAccess.get();
        for (const auto &engineName : listOfMockedEngines) {
            pSysfsAccess->setFileProperties(engineName, defaultPreemptTimeoutMilliSecs, true, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR);
            pSysfsAccess->setFileProperties(engineName, defaultTimesliceDurationMilliSecs, true, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR);
            pSysfsAccess->setFileProperties(engineName, defaultHeartbeatIntervalMilliSecs, true, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR);
            pSysfsAccess->setFileProperties(engineName, preemptTimeoutMilliSecs, true, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR);
            pSysfsAccess->setFileProperties(engineName, timesliceDurationMilliSecs, true, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR);
            pSysfsAccess->setFileProperties(engineName, heartbeatIntervalMilliSecs, true, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR);

            pSysfsAccess->write(engineDir + "/" + engineName + "/" + defaultPreemptTimeoutMilliSecs, defaultTimeoutMilliSecs);
            pSysfsAccess->write(engineDir + "/" + engineName + "/" + defaultTimesliceDurationMilliSecs, defaultTimesliceMilliSecs);
            pSysfsAccess->write(engineDir + "/" + engineName + "/" + defaultHeartbeatIntervalMilliSecs, defaultHeartbeatMilliSecs);
            pSysfsAccess->write(engineDir + "/" + engineName + "/" + preemptTimeoutMilliSecs, timeoutMilliSecs);
            pSysfsAccess->write(engineDir + "/" + engineName + "/" + timesliceDurationMilliSecs, timesliceMilliSecs);
            pSysfsAccess->write(engineDir + "/" + engineName + "/" + heartbeatIntervalMilliSecs, heartbeatMilliSecs);
        }

        pSysmanDeviceImp->pSchedulerHandleContext->handleList.clear();
        uint32_t subDeviceCount = 0;

        // We received a device handle. Check for subdevices in this device
        Device::fromHandle(device->toHandle())->getSubDevices(&subDeviceCount, nullptr);
        if (subDeviceCount == 0) {
            deviceHandles.resize(1, device->toHandle());
        } else {
            deviceHandles.resize(subDeviceCount, nullptr);
            Device::fromHandle(device->toHandle())->getSubDevices(&subDeviceCount, deviceHandles.data());
        }
        getSchedHandles(0);
    }

    void TearDown() override {
        if (!sysmanUltsEnable) {
            GTEST_SKIP();
        }
        pSysfsAccess->cleanUpMap();
        pLinuxSysmanImp->pSysfsAccess = pSysfsAccessOld;
        SysmanDeviceFixture::TearDown();
    }

    std::vector<zes_sched_handle_t> getSchedHandles(uint32_t count) {
        std::vector<zes_sched_handle_t> handles(count, nullptr);
        EXPECT_EQ(zesDeviceEnumSchedulers(device->toHandle(), &count, handles.data()), ZE_RESULT_SUCCESS);
        return handles;
    }

    zes_sched_mode_t fixtureGetCurrentMode(zes_sched_handle_t hScheduler) {
        zes_sched_mode_t mode;
        ze_result_t result = zesSchedulerGetCurrentMode(hScheduler, &mode);
        EXPECT_EQ(ZE_RESULT_SUCCESS, result);
        return mode;
    }

    zes_sched_timeout_properties_t fixtureGetTimeoutModeProperties(zes_sched_handle_t hScheduler, ze_bool_t getDefaults) {
        zes_sched_timeout_properties_t config;
        ze_result_t result = zesSchedulerGetTimeoutModeProperties(hScheduler, getDefaults, &config);
        EXPECT_EQ(ZE_RESULT_SUCCESS, result);
        return config;
    }

    zes_sched_timeslice_properties_t fixtureGetTimesliceModeProperties(zes_sched_handle_t hScheduler, ze_bool_t getDefaults) {
        zes_sched_timeslice_properties_t config;
        ze_result_t result = zesSchedulerGetTimesliceModeProperties(hScheduler, getDefaults, &config);
        EXPECT_EQ(ZE_RESULT_SUCCESS, result);
        return config;
    }
};

TEST_F(SysmanDeviceSchedulerFixture, GivenComponentCountZeroWhenCallingzesDeviceEnumSchedulersAndSysfsCanReadReturnsErrorThenZeroCountIsReturned) {
    pSysfsAccess->mockGetScanDirEntryError = ZE_RESULT_ERROR_NOT_AVAILABLE;

    auto pSchedulerHandleContextTest = std::make_unique<SchedulerHandleContext>(pOsSysman);
    pSchedulerHandleContextTest->init(deviceHandles);
    EXPECT_EQ(0u, static_cast<uint32_t>(pSchedulerHandleContextTest->handleList.size()));
}

TEST_F(SysmanDeviceSchedulerFixture, GivenComponentCountZeroWhenCallingzesDeviceEnumSchedulersAndSysfsCanReadReturnsIncorrectPermissionThenZeroCountIsReturned) {
    pSysfsAccess->setEngineDirectoryPermission(0);
    auto pSchedulerHandleContextTest = std::make_unique<SchedulerHandleContext>(pOsSysman);
    pSchedulerHandleContextTest->init(deviceHandles);
    EXPECT_EQ(0u, static_cast<uint32_t>(pSchedulerHandleContextTest->handleList.size()));
}

TEST_F(SysmanDeviceSchedulerFixture, GivenComponentCountZeroWhenCallingzesDeviceEnumSchedulersThenNonZeroCountIsReturnedAndVerifyCallSucceeds) {
    uint32_t count = 0;
    EXPECT_EQ(ZE_RESULT_SUCCESS, zesDeviceEnumSchedulers(device->toHandle(), &count, NULL));
    EXPECT_EQ(count, handleComponentCount);

    uint32_t testcount = count + 1;
    EXPECT_EQ(ZE_RESULT_SUCCESS, zesDeviceEnumSchedulers(device->toHandle(), &testcount, NULL));
    EXPECT_EQ(testcount, count);

    count = 0;
    std::vector<zes_sched_handle_t> handles(count, nullptr);
    EXPECT_EQ(zesDeviceEnumSchedulers(device->toHandle(), &count, handles.data()), ZE_RESULT_SUCCESS);
    EXPECT_EQ(count, handleComponentCount);
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerGetCurrentModeThenVerifyzesSchedulerGetCurrentModeCallSucceeds) {
    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        auto mode = fixtureGetCurrentMode(handle);
        EXPECT_EQ(mode, ZES_SCHED_MODE_TIMESLICE);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerGetTimeoutModePropertiesThenVerifyzesSchedulerGetTimeoutModePropertiesCallSucceeds) {
    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        auto config = fixtureGetTimeoutModeProperties(handle, false);
        EXPECT_EQ(config.watchdogTimeout, expectedHeartbeatTimeoutMicroSecs);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerGetTimeoutModePropertiesThenVerifyzesSchedulerGetTimeoutModePropertiesForDifferingValues) {
    auto handles = getSchedHandles(handleComponentCount);
    pSysfsAccess->write(engineDir + "/" + "vcs1" + "/" + heartbeatIntervalMilliSecs, (heartbeatMilliSecs + 5));

    for (auto handle : handles) {
        zes_sched_properties_t properties = {};
        zes_sched_timeout_properties_t config;
        ze_result_t result = zesSchedulerGetProperties(handle, &properties);
        EXPECT_EQ(ZE_RESULT_SUCCESS, result);
        if (properties.engines == ZES_ENGINE_TYPE_FLAG_MEDIA) {
            ze_result_t result = zesSchedulerGetTimeoutModeProperties(handle, false, &config);
            EXPECT_NE(ZE_RESULT_SUCCESS, result);
        }
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerGetTimeoutModePropertiesWithDefaultsThenVerifyzesSchedulerGetTimeoutModePropertiesCallSucceeds) {
    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        auto config = fixtureGetTimeoutModeProperties(handle, true);
        EXPECT_EQ(config.watchdogTimeout, expectedDefaultHeartbeatTimeoutMicroSecs);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerGetTimesliceModePropertiesThenVerifyzesSchedulerGetTimesliceModePropertiesCallSucceeds) {
    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        auto config = fixtureGetTimesliceModeProperties(handle, false);
        EXPECT_EQ(config.interval, expectedTimesliceMicroSecs);
        EXPECT_EQ(config.yieldTimeout, expectedTimeoutMicroSecs);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerGetTimescliceModePropertiesThenVerifyzesSchedulerGetTimescliceModePropertiesForDifferingPreemptTimeoutValues) {
    auto handles = getSchedHandles(handleComponentCount);
    pSysfsAccess->write(engineDir + "/" + "vcs1" + "/" + preemptTimeoutMilliSecs, (timeoutMilliSecs + 5));

    for (auto handle : handles) {
        zes_sched_properties_t properties = {};
        zes_sched_timeslice_properties_t config;
        ze_result_t result = zesSchedulerGetProperties(handle, &properties);
        EXPECT_EQ(ZE_RESULT_SUCCESS, result);
        if (properties.engines == ZES_ENGINE_TYPE_FLAG_MEDIA) {
            ze_result_t result = zesSchedulerGetTimesliceModeProperties(handle, false, &config);
            EXPECT_NE(ZE_RESULT_SUCCESS, result);
        }
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerGetTimescliceModePropertiesThenVerifyzesSchedulerGetTimescliceModePropertiesForDifferingTimesliceDurationValues) {
    auto handles = getSchedHandles(handleComponentCount);
    pSysfsAccess->write(engineDir + "/" + "vcs1" + "/" + timesliceDurationMilliSecs, (timesliceMilliSecs + 5));

    for (auto handle : handles) {
        zes_sched_properties_t properties = {};
        zes_sched_timeslice_properties_t config;
        ze_result_t result = zesSchedulerGetProperties(handle, &properties);
        EXPECT_EQ(ZE_RESULT_SUCCESS, result);
        if (properties.engines == ZES_ENGINE_TYPE_FLAG_MEDIA) {
            ze_result_t result = zesSchedulerGetTimesliceModeProperties(handle, false, &config);
            EXPECT_NE(ZE_RESULT_SUCCESS, result);
        }
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerGetTimeoutModePropertiesThenVerifyzesSchedulerGetTimeoutModePropertiesForReadFileFailureFileUnavailable) {
    auto handles = getSchedHandles(handleComponentCount);

    for (auto handle : handles) {
        zes_sched_properties_t properties = {};
        zes_sched_timeout_properties_t config;
        ze_result_t result = zesSchedulerGetProperties(handle, &properties);
        EXPECT_EQ(ZE_RESULT_SUCCESS, result);
        if (properties.engines == ZES_ENGINE_TYPE_FLAG_MEDIA) {

            pSysfsAccess->mockReadFileFailureError = ZE_RESULT_ERROR_NOT_AVAILABLE;

            ze_result_t result = zesSchedulerGetTimeoutModeProperties(handle, false, &config);
            EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, result);
        }
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerGetTimeoutModePropertiesThenVerifyzesSchedulerGetTimeoutModePropertiesForReadFileFailureInsufficientPermissions) {
    auto handles = getSchedHandles(handleComponentCount);

    for (auto handle : handles) {
        zes_sched_properties_t properties = {};
        zes_sched_timeout_properties_t config;
        ze_result_t result = zesSchedulerGetProperties(handle, &properties);
        EXPECT_EQ(ZE_RESULT_SUCCESS, result);
        if (properties.engines == ZES_ENGINE_TYPE_FLAG_MEDIA) {

            pSysfsAccess->mockReadFileFailureError = ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS;

            ze_result_t result = zesSchedulerGetTimeoutModeProperties(handle, false, &config);
            EXPECT_EQ(ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS, result);
        }
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerGetTimescliceModePropertiesThenVerifyzesSchedulerGetTimescliceModePropertiesForReadFileFailureDueToUnavailable) {
    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        zes_sched_timeslice_properties_t config;
        zes_sched_properties_t properties = {};
        ze_result_t result = zesSchedulerGetProperties(handle, &properties);
        EXPECT_EQ(ZE_RESULT_SUCCESS, result);
        if (properties.engines == ZES_ENGINE_TYPE_FLAG_MEDIA) {

            /* The flag mockReadReturnStatus enables the call to read call where
             * the read call returns ZE_RESULT_SUCCESS for the first 3 invocations
             * and the 4th invocation returns the ZE_RESULT_ERROR_NOT_AVAILABLE
             */

            pSysfsAccess->mockReadReturnStatus = true;

            result = zesSchedulerGetTimesliceModeProperties(handle, false, &config);
            EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, result);
        }
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerGetTimescliceModePropertiesThenVerifyzesSchedulerGetTimescliceModePropertiesForReadFileFailureDueToInsufficientPermissions) {
    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        zes_sched_timeslice_properties_t config;
        zes_sched_properties_t properties = {};
        ze_result_t result = zesSchedulerGetProperties(handle, &properties);
        EXPECT_EQ(ZE_RESULT_SUCCESS, result);
        if (properties.engines == ZES_ENGINE_TYPE_FLAG_MEDIA) {

            pSysfsAccess->mockReadFileFailureError = ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS;

            result = zesSchedulerGetTimesliceModeProperties(handle, false, &config);
            EXPECT_EQ(ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS, result);
        }
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerGetTimesliceModePropertiesWithDefaultsThenVerifyzesSchedulerGetTimesliceModePropertiesCallSucceeds) {
    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        auto config = fixtureGetTimesliceModeProperties(handle, true);
        EXPECT_EQ(config.interval, expectedDefaultTimesliceMicroSecs);
        EXPECT_EQ(config.yieldTimeout, expectedDefaultTimeoutMicroSecs);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerSetTimeoutModeThenVerifyzesSchedulerSetTimeoutModeCallSucceeds) {
    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        ze_bool_t needReboot;
        zes_sched_timeout_properties_t setConfig;
        setConfig.watchdogTimeout = 10000u;
        ze_result_t result = zesSchedulerSetTimeoutMode(handle, &setConfig, &needReboot);
        EXPECT_EQ(ZE_RESULT_SUCCESS, result);
        EXPECT_FALSE(needReboot);
        auto getConfig = fixtureGetTimeoutModeProperties(handle, false);
        EXPECT_EQ(getConfig.watchdogTimeout, setConfig.watchdogTimeout);
        auto mode = fixtureGetCurrentMode(handle);
        EXPECT_EQ(mode, ZES_SCHED_MODE_TIMEOUT);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerSetTimeoutModeWhenTimeoutLessThanMinimumThenVerifyzesSchedulerSetTimeoutModeCallFails) {
    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        ze_bool_t needReboot;
        zes_sched_timeout_properties_t setConfig;
        setConfig.watchdogTimeout = minTimeoutModeHeartbeat - 1;
        ze_result_t result = zesSchedulerSetTimeoutMode(handle, &setConfig, &needReboot);
        EXPECT_EQ(ZE_RESULT_ERROR_INVALID_ARGUMENT, result);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerSetTimeoutModeWhenCurrentModeIsTimeoutModeThenVerifyzesSchedulerSetTimeoutModeCallSucceeds) {
    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        ze_bool_t needReboot;
        zes_sched_timeout_properties_t setTimeOutConfig;
        setTimeOutConfig.watchdogTimeout = 10000u;
        ze_result_t result = zesSchedulerSetTimeoutMode(handle, &setTimeOutConfig, &needReboot);
        EXPECT_EQ(ZE_RESULT_SUCCESS, result);

        result = zesSchedulerSetTimeoutMode(handle, &setTimeOutConfig, &needReboot);
        EXPECT_EQ(ZE_RESULT_SUCCESS, result);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerSetTimeoutModeWhenHeartBeatSettingFailsThenVerifyzesSchedulerSetTimeoutModeCallFails) {
    for (const auto &engineName : listOfMockedEngines) {
        pSysfsAccess->setFileProperties(engineName, heartbeatIntervalMilliSecs, false, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR);
    }
    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        ze_bool_t needReboot;
        zes_sched_timeout_properties_t setTimeOutConfig;
        setTimeOutConfig.watchdogTimeout = 10000u;
        ze_result_t result = zesSchedulerSetTimeoutMode(handle, &setTimeOutConfig, &needReboot);
        EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, result);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerSetTimeoutModeWhenGetCurrentModeFailsThenVerifyzesSchedulerSetTimeoutModeCallFails) {
    for (const auto &engineName : listOfMockedEngines) {
        pSysfsAccess->setFileProperties(engineName, preemptTimeoutMilliSecs, false, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR);
    }
    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        ze_bool_t needReboot;
        zes_sched_timeout_properties_t setConfig;
        setConfig.watchdogTimeout = minTimeoutModeHeartbeat;
        ze_result_t result = zesSchedulerSetTimeoutMode(handle, &setConfig, &needReboot);
        EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, result);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerSetTimeoutModeWhenPreEmptTimeoutNoPermissionThenVerifyzesSchedulerSetTimeoutModeCallFails) {
    for (const auto &engineName : listOfMockedEngines) {
        pSysfsAccess->setFileProperties(engineName, preemptTimeoutMilliSecs, true, S_IRUSR | S_IRGRP | S_IROTH);
    }
    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        ze_bool_t needReboot;
        zes_sched_timeout_properties_t setConfig;
        setConfig.watchdogTimeout = minTimeoutModeHeartbeat;
        ze_result_t result = zesSchedulerSetTimeoutMode(handle, &setConfig, &needReboot);
        EXPECT_EQ(ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS, result);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerSetTimeoutModeWhenTimeSliceDurationNoPermissionThenVerifyzesSchedulerSetTimeoutModeCallFails) {
    for (const auto &engineName : listOfMockedEngines) {
        pSysfsAccess->setFileProperties(engineName, timesliceDurationMilliSecs, true, S_IRUSR | S_IRGRP | S_IROTH);
    }
    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        ze_bool_t needReboot;
        zes_sched_timeout_properties_t setConfig;
        setConfig.watchdogTimeout = minTimeoutModeHeartbeat;
        ze_result_t result = zesSchedulerSetTimeoutMode(handle, &setConfig, &needReboot);
        EXPECT_EQ(ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS, result);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerSetTimesliceModeThenVerifyzesSchedulerSetTimesliceModeCallSucceeds) {
    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        ze_bool_t needReboot;
        zes_sched_timeslice_properties_t setConfig;
        setConfig.interval = 1000u;
        setConfig.yieldTimeout = 1000u;
        ze_result_t result = zesSchedulerSetTimesliceMode(handle, &setConfig, &needReboot);
        EXPECT_EQ(ZE_RESULT_SUCCESS, result);
        EXPECT_FALSE(needReboot);
        auto getConfig = fixtureGetTimesliceModeProperties(handle, false);
        EXPECT_EQ(getConfig.interval, setConfig.interval);
        EXPECT_EQ(getConfig.yieldTimeout, setConfig.yieldTimeout);
        auto mode = fixtureGetCurrentMode(handle);
        EXPECT_EQ(mode, ZES_SCHED_MODE_TIMESLICE);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerSetTimesliceModeWhenIntervalIsLessThanMinimumThenVerifyzesSchedulerSetTimesliceModeCallFails) {
    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        ze_bool_t needReboot;
        zes_sched_timeslice_properties_t setConfig;
        setConfig.interval = minTimeoutInMicroSeconds - 1;
        setConfig.yieldTimeout = 1000u;
        ze_result_t result = zesSchedulerSetTimesliceMode(handle, &setConfig, &needReboot);
        EXPECT_EQ(ZE_RESULT_ERROR_INVALID_ARGUMENT, result);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerSetTimesliceModeWhenNoAccessToTimeSliceDurationThenVerifyzesSchedulerSetTimesliceModeCallFails) {
    for (const auto &engineName : listOfMockedEngines) {
        pSysfsAccess->setFileProperties(engineName, timesliceDurationMilliSecs, true, S_IRUSR | S_IRGRP | S_IROTH);
    }
    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        ze_bool_t needReboot;
        zes_sched_timeslice_properties_t setConfig;
        setConfig.interval = minTimeoutInMicroSeconds;
        setConfig.yieldTimeout = 1000u;
        ze_result_t result = zesSchedulerSetTimesliceMode(handle, &setConfig, &needReboot);
        EXPECT_EQ(ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS, result);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerSetTimesliceModeWhenNoAccessToHeartBeatIntervalThenVerifyzesSchedulerSetTimesliceModeCallFails) {
    for (const auto &engineName : listOfMockedEngines) {
        pSysfsAccess->setFileProperties(engineName, heartbeatIntervalMilliSecs, true, S_IRUSR | S_IRGRP | S_IROTH);
    }
    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        ze_bool_t needReboot;
        zes_sched_timeslice_properties_t setConfig;
        setConfig.interval = minTimeoutInMicroSeconds;
        setConfig.yieldTimeout = 1000u;
        ze_result_t result = zesSchedulerSetTimesliceMode(handle, &setConfig, &needReboot);
        EXPECT_EQ(ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS, result);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerSetExclusiveModeThenVerifyzesSchedulerSetExclusiveModeCallSucceeds) {
    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        ze_bool_t needReboot;
        ze_result_t result = zesSchedulerSetExclusiveMode(handle, &needReboot);
        EXPECT_EQ(ZE_RESULT_SUCCESS, result);
        EXPECT_FALSE(needReboot);
        auto mode = fixtureGetCurrentMode(handle);
        EXPECT_EQ(mode, ZES_SCHED_MODE_EXCLUSIVE);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerSetExclusiveModeWhenPreEmptTimeoutNotAvailableThenVerifyzesSchedulerSetExclusiveModeCallFails) {
    for (const auto &engineName : listOfMockedEngines) {
        pSysfsAccess->setFileProperties(engineName, preemptTimeoutMilliSecs, false, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR);
    }
    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        ze_bool_t needReboot;
        ze_result_t result = zesSchedulerSetExclusiveMode(handle, &needReboot);
        EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, result);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerSetExclusiveModeWhenTimeSliceDurationNotAvailableThenVerifyzesSchedulerSetExclusiveModeCallFails) {
    for (const auto &engineName : listOfMockedEngines) {
        pSysfsAccess->setFileProperties(engineName, timesliceDurationMilliSecs, false, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR);
    }
    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        ze_bool_t needReboot;
        ze_result_t result = zesSchedulerSetExclusiveMode(handle, &needReboot);
        EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, result);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerSetExclusiveModeWhenTimeSliceDurationHasNoPermissionsThenVerifyzesSchedulerSetExclusiveModeCallFails) {
    for (const auto &engineName : listOfMockedEngines) {
        pSysfsAccess->setFileProperties(engineName, timesliceDurationMilliSecs, true, S_IRUSR | S_IRGRP | S_IROTH);
    }
    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        ze_bool_t needReboot;
        ze_result_t result = zesSchedulerSetExclusiveMode(handle, &needReboot);
        EXPECT_EQ(ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS, result);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerGetCurrentModeWhenPreEmptTimeOutNotAvailableThenFailureIsReturned) {
    for (const auto &engineName : listOfMockedEngines) {
        pSysfsAccess->setFileProperties(engineName, preemptTimeoutMilliSecs, false, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR);
    }
    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        zes_sched_mode_t mode;
        ze_result_t result = zesSchedulerGetCurrentMode(handle, &mode);
        EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, result);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerGetCurrentModeWhenTimeSliceDurationNotAvailableThenFailureIsReturned) {
    for (const auto &engineName : listOfMockedEngines) {
        pSysfsAccess->setFileProperties(engineName, timesliceDurationMilliSecs, false, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR);
    }
    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        zes_sched_mode_t mode;
        ze_result_t result = zesSchedulerGetCurrentMode(handle, &mode);
        EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, result);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerGetCurrentModeWhenTimeSliceDurationHasNoPermissionThenFailureIsReturned) {
    for (const auto &engineName : listOfMockedEngines) {
        pSysfsAccess->setFileProperties(engineName, timesliceDurationMilliSecs, true, S_IRGRP | S_IROTH | S_IWUSR);
    }
    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        zes_sched_mode_t mode;
        ze_result_t result = zesSchedulerGetCurrentMode(handle, &mode);
        EXPECT_EQ(ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS, result);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerSetComputeUnitDebugModeThenUnsupportedFeatureIsReturned) {
    pSysfsAccess->writeResult = ZE_RESULT_ERROR_UNSUPPORTED_FEATURE;
    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        ze_bool_t needReload;
        ze_result_t result = zesSchedulerSetComputeUnitDebugMode(handle, &needReload);
        EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, result);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerGetTimeoutModePropertiesWithDefaultsWhenSysfsNodeIsAbsentThenFailureIsReturned) {
    pSysfsAccess->mockReadFileFailureError = ZE_RESULT_ERROR_NOT_AVAILABLE;

    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        zes_sched_timeout_properties_t config;
        ze_result_t result = zesSchedulerGetTimeoutModeProperties(handle, true, &config);
        EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, result);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerGetTimesliceModePropertiesWithDefaultsWhenSysfsNodeIsAbsentThenFailureIsReturned) {
    pSysfsAccess->mockReadFileFailureError = ZE_RESULT_ERROR_NOT_AVAILABLE;

    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        zes_sched_timeslice_properties_t config;
        ze_result_t result = zesSchedulerGetTimesliceModeProperties(handle, true, &config);
        EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, result);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerSetTimesliceModeWhenSysfsNodeIsAbsentThenFailureIsReturned) {
    pSysfsAccess->mockWriteFileStatus = ZE_RESULT_ERROR_NOT_AVAILABLE;

    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        ze_bool_t needReboot;
        zes_sched_timeslice_properties_t setConfig;
        setConfig.interval = 1000u;
        setConfig.yieldTimeout = 1000u;
        ze_result_t result = zesSchedulerSetTimesliceMode(handle, &setConfig, &needReboot);
        EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, result);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerSetTimesliceModeWhenSysfsNodeWithoutPermissionsThenFailureIsReturned) {
    pSysfsAccess->mockWriteFileStatus = ZE_RESULT_ERROR_NOT_AVAILABLE;

    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        ze_bool_t needReboot;
        zes_sched_timeslice_properties_t setConfig;
        setConfig.interval = 1000u;
        setConfig.yieldTimeout = 1000u;

        pSysfsAccess->mockWriteFileStatus = ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS;
        ze_result_t result = zesSchedulerSetTimesliceMode(handle, &setConfig, &needReboot);
        EXPECT_EQ(ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS, result);
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidDeviceHandleWhenCallingzesSchedulerGetPropertiesThenVerifyzesSchedulerGetPropertiesCallSucceeds) {
    auto handles = getSchedHandles(handleComponentCount);
    for (auto handle : handles) {
        zes_sched_properties_t properties = {};
        ze_result_t result = zesSchedulerGetProperties(handle, &properties);
        EXPECT_EQ(ZE_RESULT_SUCCESS, result);
        EXPECT_TRUE(properties.canControl);
        EXPECT_LE(properties.engines, ZES_ENGINE_TYPE_FLAG_RENDER);
        EXPECT_EQ(properties.supportedModes, static_cast<uint32_t>((1 << ZES_SCHED_MODE_TIMEOUT) | (1 << ZES_SCHED_MODE_TIMESLICE) | (1 << ZES_SCHED_MODE_EXCLUSIVE)));
    }
}

TEST_F(SysmanDeviceSchedulerFixture, GivenValidObjectsOfClassSchedulerImpAndSchedulerHandleContextThenDuringObjectReleaseCheckDestructorBranches) {
    for (auto &handle : pSysmanDeviceImp->pSchedulerHandleContext->handleList) {
        auto pSchedulerImp = static_cast<SchedulerImp *>(handle.get());
        pSchedulerImp->pOsScheduler = nullptr;
        handle = nullptr;
    }
}

TEST_F(SysmanMultiDeviceFixture, GivenValidDevicePointerWhenGettingSchedPropertiesThenValidSchedPropertiesRetrieved) {
    zes_sched_properties_t properties = {};
    std::vector<std::string> listOfEngines;
    ze_device_properties_t deviceProperties = {ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES};
    Device::fromHandle(device)->getProperties(&deviceProperties);
    LinuxSchedulerImp *pLinuxSchedulerImp = new LinuxSchedulerImp(pOsSysman, ZES_ENGINE_TYPE_FLAG_RENDER, listOfEngines,
                                                                  deviceProperties.flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE, deviceProperties.subdeviceId);
    EXPECT_EQ(ZE_RESULT_SUCCESS, pLinuxSchedulerImp->getProperties(properties));
    EXPECT_EQ(properties.subdeviceId, deviceProperties.subdeviceId);
    EXPECT_EQ(properties.onSubdevice, deviceProperties.flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE);
    delete pLinuxSchedulerImp;
}

} // namespace ult
} // namespace L0
