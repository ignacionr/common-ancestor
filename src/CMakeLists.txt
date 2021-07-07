cmake_minimum_required(VERSION 3.14)
project(common-ancestor VERSION 0.4)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
)
# For Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

FetchContent_Declare(mongoose
  GIT_REPOSITORY https://github.com/cesanta/mongoose.git
)

FetchContent_Populate(mongoose)

add_executable(common-ancestor 
  main.cpp data-adapter.h tree.h tree-parser.h
  ${mongoose_SOURCE_DIR}/mongoose.c 
)

target_link_libraries(common-ancestor
  sqlite3
)

configure_file(version.h.in version.h)

target_include_directories(common-ancestor
  PRIVATE ${mongoose_SOURCE_DIR}
  PUBLIC ${PROJECT_BINARY_DIR} )

enable_testing()

add_executable(test-common-ancestor
  test/unit/tree-parser-test.cpp
  test/unit/tree-test.cpp
  test/unit/tree-controller-test.cpp
  test/unit/mini-parser-test.cpp
  test/unit/data-adapter-test.cpp
  ${mongoose_SOURCE_DIR}/mongoose.c 
)

target_include_directories(test-common-ancestor
  PUBLIC ${PROJECT_BINARY_DIR} )

target_link_libraries(
  test-common-ancestor
  gtest_main
  sqlite3
)

include(GoogleTest)
gtest_discover_tests(test-common-ancestor)
