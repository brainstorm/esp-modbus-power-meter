FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)
idf_component_register(SRCS ${app_sources}
                       INCLUDE_DIRS "."
                                    ${CMAKE_SOURCE_DIR}/include
                                    # ${CMAKE_CURRENT_LIST_DIR}/../lib/esp-rainmaker/components/esp_rainmaker/include
                                    # ${CMAKE_CURRENT_LIST_DIR}/../lib/esp-rainmaker/components/rmaker_common/include
                                    # ${CMAKE_CURRENT_LIST_DIR}/../lib/mdns/tests/host_test/components/esp_event_mock/include
                                    # ${CMAKE_CURRENT_LIST_DIR}/../lib/mdns/tests/host_test/components/esp_system_protocols_linux/include
                       PRIV_REQUIRES nvs_flash esp_rainmaker rmaker_common esp_insights)
