cmake_minimum_required(VERSION 3.25)

project(DocFuzzer)

add_subdirectory(generator)
add_subdirectory(mutator)

set(DOC_FUZZER_ROOT "${CMAKE_CURRENT_LIST_DIR}")
set(DUMB_PATH       "${DOC_FUZZER_ROOT}/fuzz/dumb")
option(__ALL__      "DOC Fuzzer"    OFF)
option(__PDF__      "PDF Fuzzer"    OFF)

set(__TARGET__      "Target")
set(__PDF_STR__     "PDF")
set(__PDF_PATH__    "${DUMB_PATH}/pdf.dict")

if(__ALL__)
    set(__PDF__ ON)
    
endif()

# Find ANTLR4
message("Compile with ANTLR4")
find_package(ANTLR4 REQUIRED)


