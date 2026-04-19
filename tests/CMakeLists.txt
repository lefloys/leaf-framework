FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
)

FetchContent_MakeAvailable(Catch2)

add_executable("leaf-framework-tests"
    "src/iterator_tests.cpp"
)

target_link_libraries("leaf-framework-tests" PRIVATE
    "leaf-framework"
    Catch2::Catch2WithMain
)

add_test(NAME "leaf-framework-tests" COMMAND "leaf-framework-tests")
