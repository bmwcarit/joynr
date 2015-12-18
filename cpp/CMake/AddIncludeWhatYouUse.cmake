function(AddIncludeWhatYouUse TARGET)
    if(${ENABLE_INCLUDE_WHAT_YOU_USE})
        set_property(TARGET ${TARGET} PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${INCLUDE_WHAT_YOU_USE_PATH_AND_OPTIONS})
    endif(${ENABLE_INCLUDE_WHAT_YOU_USE})
endfunction(AddIncludeWhatYouUse)

