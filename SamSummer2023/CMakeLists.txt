
cmake_minimum_required(VERSION 3.0)
project(frontend)
set(CMAKE_CXX_STANDARD 11)

if (NOT EXISTS $ENV{MIDASSYS})
  message(FATAL_ERROR "Environment variable $MIDASSYS not defined, aborting.")
endif()

set(INC_PATH $ENV{MIDASSYS}/include $ENV{MIDASSYS}/mxml)
link_directories($ENV{MIDASSYS}/lib)


add_compile_options(-O2 -g -Wall -Wformat=2 -Wno-format-nonliteral -Wno-strict-aliasing -Wuninitialized -Wno-unused-function)

if (${CMAKE_SYSTEM_NAME} MATCHES Linux) 
   set(LIBS -lpthread -lutil -lrt) 
endif() 

add_executable(arduino_test_station teststand-frontend.cpp)
target_include_directories(arduino_test_station PRIVATE ${INC_PATH})
target_link_directories(arduino_test_station PRIVATE "/home/mu3e/midas-install/lib")
target_link_libraries(arduino_test_station mfe midas ${LIBS})
