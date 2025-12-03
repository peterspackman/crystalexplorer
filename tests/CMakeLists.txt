# Add Catch2 dependency
CPMAddPackage(
    NAME catch
    GITHUB_REPOSITORY "catchorg/Catch2"
    GIT_TAG v3.5.2
)

list(APPEND CMAKE_MODULE_PATH "${catch_SOURCE_DIR}/extras")
include(CTest)
include(Catch)

enable_testing()

# Catch2 test executables
add_executable(test_core "${CMAKE_CURRENT_SOURCE_DIR}/test_core.cpp")
target_link_libraries(test_core PRIVATE cx_core Catch2::Catch2WithMain)
catch_discover_tests(test_core)

add_executable(test_io "${CMAKE_CURRENT_SOURCE_DIR}/test_io.cpp")
target_link_libraries(test_io PRIVATE cx_io Catch2::Catch2WithMain)
catch_discover_tests(test_io)

add_executable(test_object_tree_model "${CMAKE_CURRENT_SOURCE_DIR}/test_object_tree_model.cpp")
target_link_libraries(test_object_tree_model PRIVATE cx Catch2::Catch2WithMain)
catch_discover_tests(test_object_tree_model)

add_executable(provider_tests "${CMAKE_CURRENT_SOURCE_DIR}/provider_tests.cpp")
target_link_libraries(provider_tests PRIVATE cx_core Catch2::Catch2WithMain)
catch_discover_tests(provider_tests)

add_executable(interaction_energy_tests "${CMAKE_CURRENT_SOURCE_DIR}/interaction_energy_tests.cpp")
target_link_libraries(interaction_energy_tests PRIVATE cx_core cx_pair_energy Catch2::Catch2WithMain)
catch_discover_tests(interaction_energy_tests)

add_executable(pair_energy_integration_tests "${CMAKE_CURRENT_SOURCE_DIR}/pair_energy_integration_tests.cpp")
target_link_libraries(pair_energy_integration_tests PRIVATE cx_core cx_pair_energy Catch2::Catch2WithMain)
catch_discover_tests(pair_energy_integration_tests)

add_executable(test_chemical_structure "${CMAKE_CURRENT_SOURCE_DIR}/test_chemical_structure.cpp")
target_link_libraries(test_chemical_structure PRIVATE cx_core Catch2::Catch2WithMain)
catch_discover_tests(test_chemical_structure)

add_executable(test_crystal_structure "${CMAKE_CURRENT_SOURCE_DIR}/test_crystal_structure.cpp")
target_link_libraries(test_crystal_structure PRIVATE cx_core cx_crystal Catch2::Catch2WithMain)
catch_discover_tests(test_crystal_structure)

add_executable(elastic_tensor_tests "${CMAKE_CURRENT_SOURCE_DIR}/elastic_tensor_tests.cpp")
target_link_libraries(elastic_tensor_tests PRIVATE cx_core Catch2::Catch2WithMain)
catch_discover_tests(elastic_tensor_tests)

add_executable(mesh_tests "${CMAKE_CURRENT_SOURCE_DIR}/mesh_tests.cpp")
target_link_libraries(mesh_tests PRIVATE cx_core cx_crystal Catch2::Catch2WithMain)
catch_discover_tests(mesh_tests)

add_executable(test_task_system
    "${CMAKE_CURRENT_SOURCE_DIR}/test_task_system.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_main.cpp")
target_link_libraries(test_task_system PRIVATE cx_exe Catch2::Catch2 Qt6::Core Qt6::Test)
catch_discover_tests(test_task_system)
