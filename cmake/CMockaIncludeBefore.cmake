# Allow top project to override option() with a variable that is not in cache.
if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()