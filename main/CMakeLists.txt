# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

idf_component_register(
    SRCS 				main.c wifi_app.c http_server.c app_nvs.c uart_app.c       # list the source files of this component
    INCLUDE_DIRS        "."			 # optional, add here public include directories
    EMBED_FILES
        "webpage/index.html"
        "webpage/index_sta.html"
        "webpage/app.css"
        "webpage/app.js"
        "webpage/favicon.ico"
        "webpage/jquery-3.3.1.min.js"
        "webpage/logo.png"
        "webpage/model.png"
)
