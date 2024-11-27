
set(srcs-app
)

# 基础程序
set(srcs-common
  

)

# 自定义框架抽象层
set(srcs-components
    "source/k_kit/k_kit.c"
    "source/shell/sh_vt100.c"
    "source/shell/sh_vset.c"
    "source/shell/sh.c"

)
set(incs
    "include"
    "source"
    "source/k_kit"
    "source/shell"
  
  
)

set(requires

    driver
    nvs_flash 
    app_update
)

idf_component_register(SRCS 
    ${srcs-components}
    ${srcs-common}
    INCLUDE_DIRS "." ${incs}
    REQUIRES ${requires}
  )
