cmake_minimum_required(VERSION 3.20)

project(VectorDB)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(app
   main.cpp
   vector_db.cpp
)

add_executable(benchmark
    benchmark.cpp
    vector_db.cpp
)