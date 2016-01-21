macro(AddSanitizerConfig CONFIG_NAME SANITIZER OPTIMIZER_LEVEL)
    set(BLACKLIST_FLAG "")
    # only clang currently supports blacklists
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND ${ARGC} GREATER 3)
        SET(BLACKLIST_FLAG " -fsanitize-blacklist=${CMAKE_CURRENT_LIST_DIR}/sanitizer-blacklist/${ARGV3}")
    endif()
    set(COMPILER_FLAGS "-O${OPTIMIZER_LEVEL} -g -fsanitize=${SANITIZER} -fno-omit-frame-pointer -fno-optimize-sibling-calls${BLACKLIST_FLAG}")
    set(CMAKE_C_FLAGS_${CONFIG_NAME} ${COMPILER_FLAGS}
        CACHE STRING "C compiler flags when building for ${CONFIG_NAME}"
    )
    set(CMAKE_CXX_FLAGS_${CONFIG_NAME} ${COMPILER_FLAGS}
        CACHE STRING "C++ compiler flags when building for ${CONFIG_NAME}"
    )

    set(LINKER_FLAGS "-fsanitize=${SANITIZER}")
    set(CMAKE_EXE_LINKER_FLAGS_${CONFIG_NAME} ${LINKER_FLAGS}
        CACHE STRING "Linker flags for executables when building for ${CONFIG_NAME}"
    )
    set(CMAKE_SHARED_LINKER_FLAGS_${CONFIG_NAME} ${LINKER_FLAGS}
        CACHE STRING "Linker flags for shared libraries when building for ${CONFIG_NAME}"
    )
    
    # add to list of available build types
    set_property(CACHE
                 CMAKE_BUILD_TYPE
                 APPEND
                 PROPERTY STRINGS ${CONFIG_NAME}
    )
endmacro(AddSanitizerConfig)

# AddressSanitizer (ASan) is a memory error detector for C/C++.
# see https://github.com/google/sanitizers/wiki/AddressSanitizer for more information.
AddSanitizerConfig(ASAN address 1)

# ThreadSanitizer (TSan) is a data race detector for C/C++.
# see https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual for more information.
AddSanitizerConfig(TSAN thread 1 tsan.txt)

# MemorySanitizer (MSan) is a detector of uninitialized memory reads in C/C++ programs.
# see https://github.com/google/sanitizers/wiki/MemorySanitizer for more information.
AddSanitizerConfig(MSAN memory 1)

# UndefinedBehaviorSanitizer (UBSan) is a fast undefined behavior detector.
# see http://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html for more information.
AddSanitizerConfig(UBSAN undefined 0 ubsan.txt)
AddSanitizerConfig(INTEGER_SAN integer 0 ubsan.txt)
