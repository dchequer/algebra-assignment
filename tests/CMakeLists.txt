# determinant tests
set_source_files_properties(determinant_tests.tpp PROPERTIES LANGUAGE CXX)

add_executable(determinant_tests determinant_tests.tpp)
target_compile_features(determinant_tests PRIVATE cxx_std_20)
target_include_directories(determinant_tests PRIVATE "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(determinant_tests PRIVATE
    algebra::finite_fields
    algebra::matrices
)

add_test(NAME determinant_tests COMMAND determinant_tests)

# encode/decode tests
set_source_files_properties(encode_decode_tests.tpp PROPERTIES LANGUAGE CXX)

add_executable(encode_decode_tests encode_decode_tests.tpp)
target_compile_features(encode_decode_tests PRIVATE cxx_std_20)
target_include_directories(encode_decode_tests PRIVATE
    "$<TARGET_PROPERTY:algebra::finite_fields,INTERFACE_INCLUDE_DIRECTORIES>"
    "${PROJECT_SOURCE_DIR}/dependencies/algebra-finite-fields-cached/include"
)
target_link_libraries(encode_decode_tests PRIVATE algebra::finite_fields)

add_test(NAME encode_decode_tests COMMAND encode_decode_tests)

# cache state tests
set_source_files_properties(cache_state_tests.tpp PROPERTIES LANGUAGE CXX)

add_executable(cache_state_tests cache_state_tests.tpp)
target_compile_features(cache_state_tests PRIVATE cxx_std_20)
target_include_directories(cache_state_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/dependencies/algebra-finite-fields-cached/include"
)
target_link_libraries(cache_state_tests PRIVATE algebra::finite_fields)

add_test(NAME cache_state_tests COMMAND cache_state_tests)

# Public cached wrapper tests use its staging header in a separate executable.
set_source_files_properties(cached_element_tests.tpp PROPERTIES LANGUAGE CXX)

add_executable(cached_element_tests cached_element_tests.tpp)
target_compile_features(cached_element_tests PRIVATE cxx_std_20)
target_include_directories(cached_element_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/dependencies/algebra-finite-fields-cached/include"
)
target_link_libraries(cached_element_tests PRIVATE
    algebra::finite_fields
    algebra::matrices
)

add_test(NAME cached_element_tests COMMAND cached_element_tests)

# Reuse the same seven determinant fixtures with the public cached element type.
add_executable(cached_determinant_tests determinant_tests.tpp)
target_compile_features(cached_determinant_tests PRIVATE cxx_std_20)
target_compile_definitions(cached_determinant_tests PRIVATE ALGEBRA_TEST_CACHED_FIELDS)
target_include_directories(cached_determinant_tests PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/dependencies/algebra-finite-fields-cached/include"
)
target_link_libraries(cached_determinant_tests PRIVATE
    algebra::finite_fields
    algebra::matrices
)

add_test(NAME cached_determinant_tests COMMAND cached_determinant_tests)
