enable_testing()

add_executable(test_core test_core.cpp)
add_executable(test_io test_io.cpp)
add_executable(test_jobrunner test_jobrunner.cpp)
add_executable(test_serialization test_serialization.cpp)

target_link_libraries(test_core PRIVATE Qt6::Test Qt6::Core Qt6::Widgets cx_core)
target_link_libraries(test_io PRIVATE Qt6::Test Qt6::Core Qt6::Widgets cx_io)
target_link_libraries(test_jobrunner PRIVATE Qt6::Test Qt6::Core Qt6::Widgets cx)
target_link_libraries(test_serialization PRIVATE Qt6::Test Qt6::Core Qt6::Widgets cx)

add_test(NAME test_core COMMAND test_core)
add_test(NAME test_io COMMAND test_io)
add_test(NAME test_serialization COMMAND test_serialization)
add_test(NAME test_jobrunner COMMAND test_jobrunner)
