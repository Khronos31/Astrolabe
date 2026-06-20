
# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)



# -------------------------------------------------------------------------
# Global define
# -------------------------------------------------------------------------

# Enable factory test
add_definitions(-DENABLE_FACTORY_TEST)

# WiFi test setting (factory test app)
add_definitions(-DFACTORY_TEST_WIFI_SSID="?")
add_definitions(-DFACTORY_TEST_WIFI_PASSWORD="?")

# Astrolabe WiFi / MQTT credentials — loaded from credentials.cmake (gitignored).
# Copy credentials.cmake.example → credentials.cmake and fill in your values.
set(ASTROLABE_WIFI_SSID "")
set(ASTROLABE_WIFI_PASS "")
set(ASTROLABE_MQTT_URI  "mqtt://192.168.1.130:1883")
set(ASTROLABE_MQTT_USER "")
set(ASTROLABE_MQTT_PASS "")
include(${CMAKE_SOURCE_DIR}/credentials.cmake OPTIONAL)
# Write values into a header to safely handle special characters (e.g. #, ', \).
configure_file(
    ${CMAKE_SOURCE_DIR}/main/credentials.h.in
    ${CMAKE_SOURCE_DIR}/main/credentials.h
)


# Font setting
add_definitions(-DGUI_FONT_CN_BIG=&fonts::efontCN_24)
# add_definitions(-DGUI_FONT_CN_SMALL=&fonts::efontCN_16)
add_definitions(-DGUI_FONT_CN_SMALL=&fonts::efontCN_16_b)

# -------------------------------------------------------------------------



include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(stamp_ring_factory_test)
