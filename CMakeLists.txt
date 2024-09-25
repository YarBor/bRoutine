project(bRoutine)
cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -g -fno-strict-aliasing -O2 -export-dynamic -Wall -pipe  -D_GNU_SOURCE -D_REENTRANT -fPIC -Wno-deprecated -m64)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall ")

set(BROUTINE_VERSION  0.1)

enable_language(C ASM)

set(SOURCE_FILES
./src/b_task.cc
./src/b_routine_env.cc
./src/b_context.cc
./src/b_mutex_cond.cc
./src/b_scheduler.cc
./src/b_epoll.cc
./src/b_routine.cc
./src/b_stack.cc
./src/b_swap.S
./src/b_hook.cc
)

# Add static and shared library target
add_library(bRoutine_static STATIC ${SOURCE_FILES})

# Set library output name
set_target_properties(bRoutine_static PROPERTIES OUTPUT_NAME bRoutine)
set_target_properties(bRoutine_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)

 