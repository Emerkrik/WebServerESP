if(IDF_VERSION_MAJOR GREATER_EQUAL 4)
    idf_component_register(SRC_DIRS src
        REQUIRES esp_wifi log nvs_flash mdns wpa_supplicant lwip esp_http_server
        INCLUDE_DIRS src
        EMBED_FILES src/style.css src/code.js src/index.html)
else()
    set(COMPONENT_SRCDIRS src)
    set(COMPONENT_ADD_INCLUDEDIRS src)
    set(COMPONENT_REQUIRES esp_wifi log nvs_flash mdns wpa_supplicant lwip esp_http_server)
    set(COMPONENT_EMBED_FILES src/style.css src/code.js src/index.html)
    register_component()
endif()
