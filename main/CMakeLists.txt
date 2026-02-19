file(GLOB SRC_FILES "*.c")

idf_component_register(
        SRCS ${SRC_FILES}
        PRIV_REQUIRES nvs_flash esp_event esp_netif esp_wifi driver esp_driver_i2c esp_timer esp_http_server mbedtls esp_adc
        INCLUDE_DIRS "."
)


# Note: No special link flags required for normal builds. However, if the
# optional RF test archive exists and exposes get_rx_freq_local, link it to
# enable CFO reading.

set(RFTEST_LIB "$ENV{IDF_PATH}/components/esp_phy/lib/${IDF_TARGET}/librftest.a")
if (EXISTS "${RFTEST_LIB}")
    message(STATUS "Linking optional RF test library: ${RFTEST_LIB}")
    target_link_libraries(${COMPONENT_LIB} PRIVATE "-Wl,--whole-archive" "${RFTEST_LIB}" "-Wl,--no-whole-archive")
endif ()

#target_link_libraries(${COMPONENT_LIB} -Wl,-zmuldefs)
#idf_build_set_property(COMPILE_OPTIONS "-O0" "-ggdb3" APPEND)
#target_link_libraries(${COMPONENT_LIB} -Wl,-zmuldefs -Wl,--wrap=esp_wifi_ftm_initiate_session)
