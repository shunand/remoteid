

set(srcs-libcanard
    "."
    "drivers/stm32"
    "drivers/socketcan"
    "drivers/nuttx"
    "drivers/avr"
    "drivers/avr/avr-can-lib/src"
    "drivers/avr/avr-can-lib/demo"
 

)
set(include_lib
    "."
    "drivers/stm32"
    "drivers/socketcan"
    "drivers/nuttx"
    "drivers/avr"
    "drivers/avr/avr-can-lib"
    "drivers/avr/avr-can-lib/src"
    "drivers/avr/avr-can-lib/demo"
    "canard"
  
)

idf_component_register(SRCS 
    ${srcs-libcanard}
    INCLUDE_DIRS "." ${include_lib})
