if(CONFIG_APP_ESPNOW_RESPONDER)
    list(APPEND srcs "responder.c")
endif()

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "include"
                    REQUIRES "esp-now")
