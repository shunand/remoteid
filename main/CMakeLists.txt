

set(srcs-hello
   


   

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
   opendriver
   esp_hw_support
   esp_partition
   nvs_flash 
   app_update   
    efuse
   )
idf_component_register(SRCS "hello_world_main.cpp"
    ${srcs-hello}
    INCLUDE_DIRS "." ${include_hello}
    REQUIRES ${requires}

)

