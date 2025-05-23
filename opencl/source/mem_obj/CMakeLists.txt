#
# Copyright (C) 2018-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

set(RUNTIME_SRCS_MEM_OBJ
    ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer_base.inl
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer_factory_init.inl
    ${CMAKE_CURRENT_SOURCE_DIR}/image.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/image.h
    ${CMAKE_CURRENT_SOURCE_DIR}/image.inl
    ${CMAKE_CURRENT_SOURCE_DIR}/image_tgllp_and_later.inl
    ${CMAKE_CURRENT_SOURCE_DIR}/image_xe2_and_later.inl
    ${CMAKE_CURRENT_SOURCE_DIR}/image_factory_init.inl
    ${CMAKE_CURRENT_SOURCE_DIR}/map_operations_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/map_operations_handler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/mem_obj.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mem_obj.h
    ${CMAKE_CURRENT_SOURCE_DIR}/mem_obj_helper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mem_obj_helper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/definitions${BRANCH_DIR_SUFFIX}buffer_ext.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/definitions${BRANCH_DIR_SUFFIX}image_ext.inl
)

if(WIN32)
  list(APPEND RUNTIME_SRCS_MEM_OBJ
       ${CMAKE_CURRENT_SOURCE_DIR}/buffer_windows.cpp
       ${CMAKE_CURRENT_SOURCE_DIR}/image_windows.cpp
  )
else()
  list(APPEND RUNTIME_SRCS_MEM_OBJ
       ${CMAKE_CURRENT_SOURCE_DIR}/buffer_linux.cpp
       ${CMAKE_CURRENT_SOURCE_DIR}/image_linux.cpp
  )
endif()

target_sources(${NEO_STATIC_LIB_NAME} PRIVATE ${RUNTIME_SRCS_MEM_OBJ})
set_property(GLOBAL PROPERTY RUNTIME_SRCS_MEM_OBJ ${RUNTIME_SRCS_MEM_OBJ})
add_subdirectories()
