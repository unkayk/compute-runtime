#
# Copyright (C) 2018-2025 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

if(NOT DISABLED_GTPIN_SUPPORT)
  set(RUNTIME_SRCS_GTPIN
      ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
      ${CMAKE_CURRENT_SOURCE_DIR}/gtpin_callbacks.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/gtpin_helpers.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/gtpin_helpers.h
      ${CMAKE_CURRENT_SOURCE_DIR}/gtpin_gfx_core_helper.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/gtpin_gfx_core_helper.h
      ${CMAKE_CURRENT_SOURCE_DIR}/gtpin_gfx_core_helper.inl
      ${CMAKE_CURRENT_SOURCE_DIR}/gtpin_init.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/gtpin_init.h
      ${CMAKE_CURRENT_SOURCE_DIR}/gtpin_notify.h
      ${CMAKE_CURRENT_SOURCE_DIR}/gtpin_defs.h
  )

  if(SUPPORT_XEHP_AND_LATER)
    list(APPEND RUNTIME_SRCS_GTPIN
         ${CMAKE_CURRENT_SOURCE_DIR}/gtpin_gfx_core_helper_xehp_and_later.inl
    )
  endif()

  if(WIN32)
    set(MSVC_DEF_ADDITIONAL_EXPORTS "${MSVC_DEF_ADDITIONAL_EXPORTS}\nGTPin_Init")
  endif()

else()
  set(RUNTIME_SRCS_GTPIN
      ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt
      ${CMAKE_CURRENT_SOURCE_DIR}/gtpin_callback_stubs.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/gtpin_notify.h
  )
endif()

target_sources(${NEO_STATIC_LIB_NAME} PRIVATE ${RUNTIME_SRCS_GTPIN})
set_property(GLOBAL PROPERTY RUNTIME_SRCS_GTPIN ${RUNTIME_SRCS_GTPIN})
set(MSVC_DEF_ADDITIONAL_EXPORTS "${MSVC_DEF_ADDITIONAL_EXPORTS}" PARENT_SCOPE)

add_subdirectories()
