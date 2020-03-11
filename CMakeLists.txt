cmake_minimum_required(VERSION 3.13)
project(d3dNon12)

set(ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    D3D11On12/external
    D3D11On12/include
    D3D12TranslationLayer/external
    D3D12TranslationLayer/include
    )

set(LAYER d3d12translationlayer)
aux_source_directory(D3D12TranslationLayer/src TRANSLATIONLAYER_SRC)
set(TRANSLATIONLAYER_WDK_SRC
    D3D12TranslationLayer/src/DXBCUtils.cpp
    D3D12TranslationLayer/src/ShaderBinary.cpp
    D3D12TranslationLayer/src/BlobContainer.cpp
    D3D12TranslationLayer/src/ShaderParser.cpp
    D3D12TranslationLayer/src/SharedResourceHelpers.cpp
    )
list(REMOVE_ITEM TRANSLATIONLAYER_SRC ${TRANSLATIONLAYER_WDK_SRC})

add_library(${LAYER} STATIC ${TRANSLATIONLAYER_SRC})
target_link_libraries(${LAYER} d3d12 dxcore dxgi atls)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/D3D12TranslationLayer/packages.config ${CMAKE_CURRENT_BINARY_DIR}/packages.config COPYONLY)
add_library(WinPixEventRuntime INTERFACE IMPORTED GLOBAL)
set_target_properties(WinPixEventRuntime PROPERTIES INTERFACE_LINK_LIBRARIES "${CMAKE_BINARY_DIR}/packages/WinPixEventRuntime.1.0.190604001/build/WinPixEventRuntime.targets")
target_link_libraries(${LAYER} WinPixEventRuntime)

target_compile_definitions(${LAYER} PRIVATE $<$<CONFIG:DEBUG>:DBG>)
target_compile_definitions(${LAYER} PUBLIC $<$<CONFIG:DEBUG>:TRANSLATION_LAYER_DBG=1>)

if(MSVC)
    target_compile_options(${LAYER} PUBLIC /W4 /WX /wd4238 /wd4324)
    target_link_options(${LAYER} INTERFACE "/ignore:4286")
else()
    target_compile_options(${LAYER} PUBLIC -Wall -Wextra -pedantic -Werror)
endif()

if (NOT IMPLEMENT_RECTS)
    target_compile_definitions(${LAYER} PRIVATE NO_IMPLEMENT_RECT_FNS)
endif()

include(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX(d3d12TokenizedProgramFormat.hpp HAS_WDK)

if (HAS_WDK)
    message(NOTICE "Adding WDK-dependent project.")
    target_compile_definitions(${LAYER} PUBLIC SUPPORTS_DXBC_PARSE)

    add_library(${LAYER}_wdk STATIC
        ${TRANSLATIONLAYER_WDK_SRC}
        )
    target_link_libraries(${LAYER}_wdk ${LAYER})
else()
    message(WARNING "Only adding SDK-dependent projects.")
endif()

aux_source_directory(D3D11On12/src D3D11ON12_SRC)
set(D3D11ON12_SRC
    ${D3D11ON12_SRC}
    D3D11On12/src/d3d11on12.rc
    D3D11On12/src/d3d11on12.def
    )
set(D3D11ON12 d3d11on12)
add_library(${D3D11ON12} SHARED ${D3D11ON12_SRC})
target_link_libraries(${D3D11ON12} ${LAYER}_wdk)
target_include_directories(${D3D11ON12}
    PRIVATE D3D11On12/include
    PRIVATE D3D11On12/external
    PUBLIC D3D11On12/interface
    PRIVATE D3D11On12/src)


add_subdirectory(sample)
