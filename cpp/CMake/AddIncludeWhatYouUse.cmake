include_guard(GLOBAL)

include(LocateProgram)

option(
    ENABLE_INCLUDE_WHAT_YOU_USE
    "Use include-what-you-use to identify missing/superfluous includes?"
    OFF
)
if(${ENABLE_INCLUDE_WHAT_YOU_USE})
    LocateProgram(include-what-you-use INCLUDE_WHAT_YOU_USE_PATH)
    set(INCLUDE_WHAT_YOU_USE_OPTIONS "" CACHE STRING "additional options for include-what-you-use")
    set(INCLUDE_WHAT_YOU_USE_PATH_AND_OPTIONS
        "${INCLUDE_WHAT_YOU_USE_PATH}"
        ${INCLUDE_WHAT_YOU_USE_OPTIONS}
    )
endif()

function(AddIncludeWhatYouUse TARGET)
    if(${ENABLE_INCLUDE_WHAT_YOU_USE})
        set_property(TARGET ${TARGET} PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${INCLUDE_WHAT_YOU_USE_PATH_AND_OPTIONS})
    endif(${ENABLE_INCLUDE_WHAT_YOU_USE})
endfunction()
