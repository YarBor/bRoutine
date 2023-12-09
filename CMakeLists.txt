project(bRoutine)
cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -g -fno-strict-aliasing -O2 -export-dynamic -Wall -pipe  -D_GNU_SOURCE -D_REENTRANT -fPIC -Wno-deprecated -m64)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall ")

set(BROUTINE_VERSION  0.1)

enable_language(C ASM)

set(SOURCE_FILES
./src/bRoutineEnv.cc
./src/bTask.cc
./src/bContext.cc
./src/bMutex_Cond.cc
./src/bScheduler.cc
./src/bTimer.cc
./src/bEpoll.cc
./src/bRoutine.cc
./src/bStack.cc
./src/bSwap.S
./src/bHook.cc
)

# Add static and shared library target
add_library(bRoutine_static STATIC ${SOURCE_FILES})

# Set library output name
set_target_properties(bRoutine_static PROPERTIES OUTPUT_NAME bRoutine)
set_target_properties(bRoutine_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)

 