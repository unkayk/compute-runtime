/*
 * Copyright (C) 2021-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <dirent.h>
#include <iostream>
#include <poll.h>
#include <sys/stat.h>
#include <vector>

namespace NEO {
namespace SysCalls {

extern int (*sysCallsOpen)(const char *pathname, int flags);
extern int (*sysCallsOpenWithMode)(const char *pathname, int flags, int mode);
extern ssize_t (*sysCallsPread)(int fd, void *buf, size_t count, off_t offset);
extern ssize_t (*sysCallsPwrite)(int fd, const void *buf, size_t count, off_t offset);
extern int (*sysCallsReadlink)(const char *path, char *buf, size_t bufsize);
extern int (*sysCallsIoctl)(int fileDescriptor, unsigned long int request, void *arg);
extern int (*sysCallsPoll)(struct pollfd *pollFd, unsigned long int numberOfFds, int timeout);
extern ssize_t (*sysCallsRead)(int fd, void *buf, size_t count);
extern ssize_t (*sysCallsWrite)(int fd, void *buf, size_t count);
extern int (*sysCallsPipe)(int pipeFd[2]);
extern int (*sysCallsFstat)(int fd, struct stat *buf);
extern char *(*sysCallsRealpath)(const char *path, char *buf);
extern ssize_t (*sysCallsPwrite)(int fd, const void *buf, size_t count, off_t offset);
extern int (*sysCallsRename)(const char *currName, const char *dstName);
extern int (*sysCallsScandir)(const char *dirp,
                              struct dirent ***namelist,
                              int (*filter)(const struct dirent *),
                              int (*compar)(const struct dirent **,
                                            const struct dirent **));
extern int (*sysCallsUnlink)(const std::string &pathname);
extern int (*sysCallsStat)(const std::string &filePath, struct stat *statbuf);
extern int (*sysCallsMkstemp)(char *fileName);

extern int flockRetVal;
extern int closeFuncRetVal;
extern int closeFuncArgPassed;
extern const char *drmVersion;
constexpr int fakeFileDescriptor = 123;
extern int passedFileDescriptorFlagsToSet;
extern int getFileDescriptorFlagsCalled;
extern int setFileDescriptorFlagsCalled;
extern uint32_t closeFuncCalled;
extern bool exitCalled;
extern int latestExitCode;
extern int unlinkCalled;
extern int scandirCalled;
extern int mkstempCalled;
extern int renameCalled;
extern int pathFileExistsCalled;
extern int flockCalled;

extern std::vector<void *> mmapVector;
extern std::vector<void *> mmapCapturedExtendedPointers;
extern bool mmapCaptureExtendedPointers;
extern bool mmapAllowExtendedPointers;
extern uint32_t mmapFuncCalled;
extern uint32_t munmapFuncCalled;
} // namespace SysCalls
} // namespace NEO
