

set(srcs-Drone
    "."
    "src"

)
set(include_dro
    "."
    "include"

)

idf_component_register(SRCS 
    ${srcs-Drone}
    INCLUDE_DIRS "." ${include_dro})
