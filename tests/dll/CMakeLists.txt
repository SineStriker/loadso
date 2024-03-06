project(testdll)

add_library(${PROJECT_NAME} SHARED dll.h dll.cpp)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_11)