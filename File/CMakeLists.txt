cmake_minimum_required(VERSION 3.25)

project(DocFuzzer)

add_subdirectory(generator)
add_subdirectory(mutator)

# find java
find_package(Java REQUIRED)

#
file(GLOB PDF_G4 "generator/")
