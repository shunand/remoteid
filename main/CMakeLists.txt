

set(srcs-hello
   

    "mavlink.cpp"
    "CANDriver.cpp"
    "DroneCAN.cpp"
    "WiFi_TX.cpp"
    "BLE_TX.cpp"
 

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
            )
idf_component_register(SRCS "hello_world_main.cpp"
    ${srcs-hello}
    INCLUDE_DIRS "." ${include_hello}
    REQUIRES ${requires}

)

