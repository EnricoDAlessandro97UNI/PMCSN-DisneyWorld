cmake_minimum_required(VERSION 3.23)
project(PMCSN_DisneyWorld C)

set(CMAKE_C_STANDARD 11)

include_directories(src)
include_directories(src/block1)
include_directories(src/block2)
include_directories(src/block3)
include_directories(src/block4)
include_directories(src/block5)

add_executable(PMCSN_DisneyWorld
        src/block1/block1_helper.h
        src/block1/msq_block1.c
        src/block2/block2_helper.c
        src/block2/block2_helper.h
        src/block2/ssq1_block2.c
        src/block2/ssq3.c
        src/block3/block3_helper.h
        src/block3/msq_block3.c
        src/block4/block4_helper.c
        src/block4/block4_helper.h
        src/block4/msq_block4.c
        src/block5/block5_helper.h
        src/block5/msq_block5.c
        src/orchestrator.c
        src/orchestrator_helper.c
        src/orchestrator_helper.h
        src/rngs.c
        src/rngs.h)
