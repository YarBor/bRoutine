project(bRoutine)
cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -g -fno-strict-aliasing -O0   -export-dynamic -Wall -pipe  -D_GNU_SOURCE -D_REENTRANT -fPIC -Wno-deprecated -m64)
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g )
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")

set(BROUTINE_VERSION  0.1)

enable_language(C ASM)

set(SOURCE_FILES
        bConditionVar.cc
        bHook.cc
        bRoutineEnv.cc
        bTask.cc
        bContext.cc
        bMutex.cc
        bScheduler.cc
        bTimer.cc
        bEpoll.cc
        bRoutine.cc
        bStack.cc
        bSwap.S
)
 

# Add static and shared library target
add_library(bRoutine_static STATIC ${SOURCE_FILES})
add_library(bRoutine_shared SHARED ${SOURCE_FILES})

# Set library output name
set_target_properties(bRoutine_static PROPERTIES OUTPUT_NAME bRoutine)
set_target_properties(bRoutine_shared PROPERTIES OUTPUT_NAME bRoutine)

set_target_properties(bRoutine_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(bRoutine_shared PROPERTIES CLEAN_DIRECT_OUTPUT 1)

set_target_properties(bRoutine_shared PROPERTIES VERSION ${BROUTINE_VERSION} SOVERSION ${BROUTINE_VERSION})

# Macro for add example target
macro(add_example_target EXAMPLE_TARGET)
    add_executable("example_${EXAMPLE_TARGET}" "example_${EXAMPLE_TARGET}.cpp")
    target_link_libraries("example_${EXAMPLE_TARGET}" bRoutine_static pthread dl)
endmacro(add_example_target)

add_executable(test test.cc)
target_link_libraries(test bRoutine_static pthread dl g)
