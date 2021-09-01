# Allow top project to override option() with a variable that is not in cache.
if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

# For set(CACHE) not removing a variable of the same name
if(POLICY CMP0126)
    cmake_policy(SET CMP0126 NEW)
endif()