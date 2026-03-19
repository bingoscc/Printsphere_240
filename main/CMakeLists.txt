set(COMPONENT_SRCS
    app_main.cpp
    display/tft_drivers.cpp
    config/config_store.cpp
    wifi/wifi_manager.cpp
    input/button.cpp
    state/printer_state.cpp
    cloud/bambu_cloud_client.cpp
    cloud/printer_client.cpp
    hass/hass_mqtt.cpp
    http/setup_portal.cpp
    ui/ui.cpp
    ui/screens.cpp
    ui/styles.cpp
    ui/progress_ring.cpp
)

set(COMPONENT_ADD_INCLUDEDIRS ".")

idf_component_register(
    SRCS ${COMPONENT_SRCS}
    INCLUDE_DIRS ${COMPONENT_ADD_INCLUDEDIRS}
    REQUIRES esp32 lvgl esp_timer esp_netif mqtt json nvs_flash wifi_provisioning driver
)
