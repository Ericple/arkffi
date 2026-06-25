cmake_minimum_required(VERSION 3.5.0)
project(myNpmLib)

set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

if(DEFINED PACKAGE_FIND_FILE)
    include(${PACKAGE_FIND_FILE})
endif()

include_directories(${NATIVERENDER_ROOT_PATH}
                    ${NATIVERENDER_ROOT_PATH}/include)

# 解除所有注释以观察测试用例
#add_library(ffi_target SHARED
#    third_party/ffi_target.c
#)

# 解除以下注释以观察调用外部so的案例
#add_library(rvohostest SHARED IMPORTED)
#set_target_properties(rvohostest
#PROPERTIES
#IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../../../libs/${OHOS_ARCH}/librvohostest.so)

add_library(library SHARED napi_init.cpp)
target_link_libraries(library PUBLIC
libace_napi.z.so
#rvohostest
)
