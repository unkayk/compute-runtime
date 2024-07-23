/*
 * Copyright (C) 2018-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <string>

namespace NEO {

namespace SysCalls {

unsigned int getProcessId();
unsigned int getCurrentProcessId();

unsigned long getNumThreads();

void exit(int code);

bool pathExists(const std::string &path);

} // namespace SysCalls

} // namespace NEO
