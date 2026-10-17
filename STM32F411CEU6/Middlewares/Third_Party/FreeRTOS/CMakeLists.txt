add_library(freertos_kernel STATIC
    Source/tasks.c
    Source/queue.c
    Source/list.c
    Source/portable/GCC/ARM_CM4F/port.c
    Source/portable/MemMang/heap_4.c
    Config/freertos_hooks.c
)

target_include_directories(freertos_kernel PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/include
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/portable/GCC/ARM_CM4F
    ${CMAKE_CURRENT_SOURCE_DIR}/Config
)

target_link_libraries(freertos_kernel PRIVATE stm32cubemx)
