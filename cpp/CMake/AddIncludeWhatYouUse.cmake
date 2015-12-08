function(add_include_what_you_use TARGET)
    if(${ENABLE_INCLUDE_WHAT_YOU_USE})
        set_property(TARGET ${TARGET} PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${INCLUDE_WHAT_YOU_USE_PATH_AND_OPTIONS})
    endif(${ENABLE_INCLUDE_WHAT_YOU_USE})
endfunction(add_include_what_you_use)

