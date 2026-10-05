cmake_minimum_required(VERSION 3.24)

add_library(_Modbus_STM32_HAL_FreeRTOS_internal STATIC
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/Modbus-STM32-HAL-FreeRTOS/MODBUS-LIB//Src/Modbus.c
    ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/Modbus-STM32-HAL-FreeRTOS/MODBUS-LIB//Src/UARTCallback.c
)

target_include_directories(_Modbus_STM32_HAL_FreeRTOS_internal
    PUBLIC
        ${CMAKE_SOURCE_DIR}/Middlewares/Third_Party/Modbus-STM32-HAL-FreeRTOS/MODBUS-LIB//Inc
)

target_link_libraries(_Modbus_STM32_HAL_FreeRTOS_internal
    PUBLIC
        stm32cubemx
)

add_library(Modbus_STM32_HAL_FreeRTOS INTERFACE)

target_link_libraries(Modbus_STM32_HAL_FreeRTOS
    INTERFACE
        "$<LINK_LIBRARY:WHOLE_ARCHIVE,_Modbus_STM32_HAL_FreeRTOS_internal>"
)