project(bRoutine)
cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -g -fno-strict-aliasing -O2 -export-dynamic -Wall -pipe  -D_GNU_SOURCE -D_REENTRANT -fPIC -Wno-deprecated -m64)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall ")

set(BROUTINE_VERSION  0.1)

enable_language(C ASM)

set(SOURCE_FILES
../src/bRoutineEnv.cc
../src/bTask.cc
../src/bContext.cc
../src/bMutex_Cond.cc
../src/bScheduler.cc
../src/bTimer.cc
../src/bEpoll.cc
../src/bRoutine.cc
../src/bStack.cc
../src/bSwap.S
../src/bHook.cc
)
 

# Add static and shared library target
add_library(bRoutine_static STATIC ${SOURCE_FILES})
add_library(bRoutine_static_testSwap STATIC ${SOURCE_FILES})

target_compile_definitions(bRoutine_static_testSwap PUBLIC TEST_SWAP_TIME=1)

# Set library output name
set_target_properties(bRoutine_static PROPERTIES OUTPUT_NAME bRoutine)
set_target_properties(bRoutine_static_testSwap PROPERTIES OUTPUT_NAME bRoutine_testSwap)

set_target_properties(bRoutine_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(bRoutine_static_testSwap PROPERTIES CLEAN_DIRECT_OUTPUT 1)


# Macro for add example target
macro(add_example_target EXAMPLE_TARGET)
    add_executable("${EXAMPLE_TARGET}" "./${EXAMPLE_TARGET}.cc")
    target_link_libraries("${EXAMPLE_TARGET}" bRoutine_static  dl pthread)
endmacro(add_example_target)

add_example_target(test_create_time)
add_example_target(test_producer_consumer)
add_example_target(test_CS_net_Echo)


add_executable(test_swap_time ./test_swap_time.cc)
target_link_libraries(test_swap_time bRoutine_static_testSwap pthread dl)
