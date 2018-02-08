cmake_minimum_required(VERSION 2.8.11)

# this project includes all test subdirectories
project(joynr-tests)
add_subdirectory(dummyKeychain)
add_subdirectory(inter-language-test)
add_subdirectory(performance-test)
add_subdirectory(robustness-test)
add_subdirectory(robustness-test-env)
add_subdirectory(system-integration-test/sit-cpp-app)
