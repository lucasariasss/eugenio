# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

idf_component_register(
    SRCS 				main.c wifi_app.c lm35_app.c cooler_app.c msg_app.c console_app.c aux_app.c # list the source files of this component
    INCLUDE_DIRS        "."			 # optional, add here public include directories
)
