cmake_minimum_required(VERSION 3.5.0)
project(h261e LANGUAGES C)

set(CMAKE_C_STANDARD 11) 

set(CMAKE_C_FLAGS "-g -O3")

set(CMAKE_VERBOSE_MAKEFILE ON)

add_executable(h261e 
    H261Enc.c
    H261Enc_Block.c
    H261Enc_dct.c
    H261Enc_MacroBlock.c 
    H261Enc_Motion.c
    H261Enc_Put.c
    main.c
)

# link math lib
target_link_libraries(h261e m)

