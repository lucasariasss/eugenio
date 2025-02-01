# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

idf_component_register(
    SRCS 				main.c wifi_app.c http_server.c app_nvs.c uart_Handle.c #LCDI2C.c       # list the source files of this component
    INCLUDE_DIRS        "."			 # optional, add here public include directories
						EMBED_FILES webpage/app.css webpage/app.js webpage/logo.png webpage/favicon.ico webpage/model.png webpage/index.html webpage/jquery-3.3.1.min.js
)
