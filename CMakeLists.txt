cmake_minimum_required(VERSION 3.16.0)

list(APPEND EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_LIST_DIR}/src
                                 ${CMAKE_CURRENT_LIST_DIR}/lib/esp-rainmaker/components 
                                 ${CMAKE_CURRENT_LIST_DIR}/lib/esp-rainmaker/components/esp-insights/components
                                 ${CMAKE_CURRENT_LIST_DIR}/lib/esp-rainmaker/examples/common)
                                #  ${CMAKE_CURRENT_LIST_DIR}/lib/esp-rainmaker/examples/common/app_wifi
                                #  ${CMAKE_CURRENT_LIST_DIR}/lib/esp-rainmaker/examples/common/app_reset
                                # ${CMAKE_CURRENT_LIST_DIR}/lib/mdns)

# Maybe required for ESP-IDF 4.4 vs 5.x migration
#set(EXCLUDE_COMPONENTS freemodbus)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp-modbus-power-meter)
