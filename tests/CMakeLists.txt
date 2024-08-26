enable_testing()

add_executable(test_core EXCLUDE_FROM_ALL test_core.cpp)
add_executable(test_io EXCLUDE_FROM_ALL test_io.cpp)
add_executable(test_object_tree_model EXCLUDE_FROM_ALL test_object_tree_model.cpp)

target_link_libraries(test_core PRIVATE Qt6::Test Qt6::Core Qt6::Widgets cx_core)
target_link_libraries(test_io PRIVATE Qt6::Test Qt6::Core Qt6::Widgets cx_io)
target_link_libraries(test_object_tree_model PRIVATE Qt6::Test Qt6::Core Qt6::Widgets cx)

add_test(NAME test_core COMMAND test_core)
add_test(NAME test_io COMMAND test_io)
add_test(NAME test_object_tree_model COMMAND test_object_tree_model)
