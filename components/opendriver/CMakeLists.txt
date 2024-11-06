

set(srcs-hello
    "mavlink.cpp"
    "CANDriver.cpp"
    "DroneCAN.cpp"
    "WiFi_TX.cpp"
    "BLE_TX.cpp"
    "transport.cpp"
    "transmitter.cpp"
    "parameters.cpp"
    "romfs.cpp"
    "webinterface.cpp"
    "check_firmware.cpp"
    "monocypher.cpp"
    "tinfgzip.cpp"
    "tinflate.cpp"
    "status.cpp"
    "util.cpp"
    "efuse.cpp"
    "led.cpp"
    "Adafruit_NeoPixel.cpp"
    "esp.c"


   
 

)
set(include_hello
    "."
)

set(requires
       arduino-esp32-3.0.7
      DroneCAN_generated
      libcanard
    libopendroneid
   mavlink2
   esp_hw_support
   esp_partition
   nvs_flash 
   app_update   
    efuse
   )
idf_component_register(SRCS "opendriver.cpp"
    ${srcs-hello}
    INCLUDE_DIRS "." ${include_hello}
    REQUIRES ${requires}

)

