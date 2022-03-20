# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.
set(PROJECT_VER "1.0")

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)
idf_component_register(SRCS ${app_sources})

set(RMAKER_PATH ${CMAKE_SOURCE_DIR}/../lib/esp-rainmaker)

#if(DEFINED ENV{RMAKER_PATH})
#  set(RMAKER_PATH $ENV{RMAKER_PATH})
#else()
#  set(RMAKER_PATH ${CMAKE_CURRENT_LIST_DIR}/../..)
#endif(DEFINED ENV{RMAKER_PATH})

# Add RainMaker components and other common application components
set(EXTRA_COMPONENT_DIRS ${RMAKER_PATH}/components/esp-insights/components ${RMAKER_PATH}/components ${RMAKER_PATH}/examples/common)