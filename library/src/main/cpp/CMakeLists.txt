cmake_minimum_required(VERSION 3.5.0)
project(myNpmLib)

set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

if(DEFINED PACKAGE_FIND_FILE)
    include(${PACKAGE_FIND_FILE})
endif()

include_directories(${NATIVERENDER_ROOT_PATH}
                    ${NATIVERENDER_ROOT_PATH}/include)

# 解除所有注释以观察测试用例
add_library(ffi_target SHARED
    third_party/ffi_target.c
)

add_library(library SHARED napi_init.cpp)

if (OHOS_ARCH MATCHES "arm64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    target_sources(library PRIVATE call_generic_arm64.s)
    set_source_files_properties(call_generic_arm64.s PROPERTIES LANGUAGE C)
elseif (OHOS_ARCH MATCHES "x86_64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    target_sources(library PRIVATE call_generic_x86_64.s)
    set_source_files_properties(call_generic_x86_64.s PROPERTIES LANGUAGE C)
elseif (OHOS_ARCH MATCHES "riscv64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "riscv64")
    target_sources(library PRIVATE call_generic_riscv64.s)
    set_source_files_properties(call_generic_riscv64.s PROPERTIES LANGUAGE C)
endif()

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../../libs/${OHOS_ARCH}/librvohostest.so)
    add_library(rvohostest SHARED IMPORTED)
    set_target_properties(rvohostest PROPERTIES
        IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../../../libs/${OHOS_ARCH}/librvohostest.so)
    target_link_libraries(library PUBLIC libace_napi.z.so rvohostest)
else()
    target_link_libraries(library PUBLIC libace_napi.z.so)
endif()
