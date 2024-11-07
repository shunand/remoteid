

set(srcs-openid
    "opendroneid.c"
    "wifi.c"

)
set(include_dirs
    "."

)

idf_component_register(SRCS 
    ${srcs-openid}
    INCLUDE_DIRS "." ${include_dirs})
