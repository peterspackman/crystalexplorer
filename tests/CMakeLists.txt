enable_testing()

add_executable(test_core test_core.cpp)
add_executable(test_io test_io.cpp)
add_executable(test_serialization test_serialization.cpp)
add_executable(test_object_tree_model test_object_tree_model.cpp)

target_link_libraries(test_core PRIVATE Qt6::Test Qt6::Core Qt6::Widgets cx_core)
target_link_libraries(test_io PRIVATE Qt6::Test Qt6::Core Qt6::Widgets cx_io)
target_link_libraries(test_serialization PRIVATE Qt6::Test Qt6::Core Qt6::Widgets cx)
target_link_libraries(test_object_tree_model PRIVATE Qt6::Test Qt6::Core Qt6::Widgets cx)

add_test(NAME test_core COMMAND test_core)
add_test(NAME test_io COMMAND test_io)
add_test(NAME test_serialization COMMAND test_serialization)
add_test(NAME test_object_tree_model COMMAND test_object_tree_model)
