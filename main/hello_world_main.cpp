//file: main.cpp
#include "Arduino.h"
#include "opendriver.h"
extern "C" void app_main()
{
   
  openDriver();
  // WARNING: if program reaches end of function app_main() the MCU will restart.
}