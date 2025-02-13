/*
 * Copyright (C) 2020-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/test/common/helpers/test_files.h"
#include "shared/test/common/tests_configuration.h"

#include "level_zero/core/source/cmdlist/cmdlist.h"
#include "level_zero/core/test/common/ult_config_listener_l0.h"

namespace L0 {
extern std::vector<_ze_driver_handle_t *> *globalDriverHandles;
}
using namespace NEO;
void cleanTestHelpers() {
    delete L0::globalDriverHandles;
    L0::globalDriverHandles = nullptr;
}

bool sysmanUltsEnable = false;

void applyWorkarounds() {

    auto sysmanUltsEnableEnv = getenv("NEO_L0_SYSMAN_ULTS_ENABLE");
    if (sysmanUltsEnableEnv != nullptr) {
        sysmanUltsEnable = (strcmp(sysmanUltsEnableEnv, "1") == 0);
    }
    L0::globalDriverHandles = new std::vector<_ze_driver_handle_t *>;
    L0::globalDriverHandles->reserve(1);
}

void setupTestFiles(std::string testBinaryFiles, int32_t revId) {
    std::string testBinaryFilesApiSpecific = testBinaryFiles;
    testBinaryFilesApiSpecific.append("/level_zero/");
    testBinaryFiles.append("/" + binaryNameSuffix + "/");
    testBinaryFilesApiSpecific.append(binaryNameSuffix + "/");

    testBinaryFiles.append(std::to_string(revId));
    testBinaryFiles.append("/");
    testBinaryFiles.append(testFiles);
    testBinaryFilesApiSpecific.append(std::to_string(revId));
    testBinaryFilesApiSpecific.append("/");
    testBinaryFilesApiSpecific.append(testFilesApiSpecific);
    testFiles = testBinaryFiles;
    testFilesApiSpecific = testBinaryFilesApiSpecific;
}

std::string getBaseExecutionDir() {
    if (testMode != TestMode::aubTests) {
        return "level_zero/";
    }
    return "";
}

void addUltListener(::testing::TestEventListeners &listeners) {
    listeners.Append(new L0::UltConfigListenerL0);
}
