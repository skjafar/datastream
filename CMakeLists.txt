
idf_component_register(SRCS "src/datastream.c"
                            "src/dsRegisters.c"
                            "src/dsParameters.c"
                            "src/dsTCP.c"
                            "src/dsUDP.c"
                       INCLUDE_DIRS
                            "include"
                       REQUIRES
                            "driver"
                            "esp_wifi"
                            "esp_system"
                            "esp_netif"
                            "lwip"
                            "freertos"
                            "main")
