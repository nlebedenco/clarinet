# ENet requires a CMake version that is too old so a warning is generated over policy CMP0048.
# We inject here a policy setting and silence the warning without having to modify ENet's CMakeLists.txt. 
# This is safe as long as ENet does not define a VERSION variable of its own.
if(POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif()
