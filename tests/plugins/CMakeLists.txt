include(../../cmake/plugin.cmake)

add_library(plugin1 SHARED plugin1.h plugin1.cpp)
loadso_export_plugin(plugin1 plugin1.h LoadSO::Plugin METADATA_FILE plugin1.txt)

add_library(plugin2 SHARED plugin2.cpp)
loadso_export_plugin(plugin2 plugin2.cpp LoadSO::Plugin METADATA_FILE plugin2.txt)

add_executable(loader loader.cpp)
target_link_libraries(loader PRIVATE loadso)
target_compile_definitions(loader PRIVATE
    PLUGIN1_NAME="$<TARGET_FILE:plugin1>"
    PLUGIN2_NAME="$<TARGET_FILE:plugin2>"
)