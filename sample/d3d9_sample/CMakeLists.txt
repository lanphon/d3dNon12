SET(TARGET d3d9_sample)
INCLUDE_DIRECTORIES(.)
AUX_SOURCE_DIRECTORY(. SRC)
ADD_EXECUTABLE(${TARGET} WIN32
    ${SRC}
    )
TARGET_COMPILE_DEFINITIONS(${TARGET} PUBLIC
    UNICODE
    _UNICODE
    _CRT_SECURE_NO_WARNINGS
    )

#set(EXTRA_LIBS WinPixEventRuntime d3d12 dxgi d3d11on12)
TARGET_LINK_LIBRARIES(${TARGET}
    d3d9
    dxgi
    Winmm
#    d3d11
#    d3dcompiler
#    dxguid
#    ${EXTRA_LIBS}
    )
if(MSVC)
    target_compile_options(${TARGET} PUBLIC /wd4189 /wd4101 /wd4100 /wd4456)
    set_target_properties(${TARGET} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/$(Configuration)"
        )
endif()

