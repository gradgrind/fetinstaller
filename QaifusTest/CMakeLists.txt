set(this_target "QaifusTest")
cmake_minimum_required(VERSION 3.28)
project("QaifusTest" LANGUAGES CXX)

include(QAIFUS/common1.cmake)

add_subdirectory(QaifusTest1)
add_subdirectory(QaifusTest2)
add_subdirectory(QaifusTest3)
