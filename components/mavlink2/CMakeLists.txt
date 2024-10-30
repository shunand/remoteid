

set(srcs-mavlink2
    "."
    

)
set(include
    "."
    "generated"
    "generated/all"
   "generated/ardupilotmega"
   "generated/ASLUAV"
   "generated/AVSSUAS"
   "generated/common"
   "generated/cubepilot"
   "generated/development"
   "generated/icarous"
   "generated/loweheiser"
   "generated/minimal"
   "generated/python_array_test"
   "generated/standard"
   "generated/storm32"
   "generated/test"
   "generated/ualberta"
   "generated/uAvionix"

)

idf_component_register(SRCS 
    ${srcs-mavlink2}
    INCLUDE_DIRS "." ${include})
