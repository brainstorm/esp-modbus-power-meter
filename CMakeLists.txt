cmake_minimum_required(VERSION 3.16.0)

list(APPEND EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_LIST_DIR}/lib/esp-rainmaker/components ${CMAKE_CURRENT_LIST_DIR}/lib/esp-rainmaker/components/esp-insights/components)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(yigedianqi-modbus)
