idf_component_register(
    SRCS
        "src/main.c"
        "src/attack.c"
        "src/utils.c"
        "src/webserver.c"
    INCLUDE_DIRS
        "include"
)

target_link_libraries(${COMPONENT_LIB} "-z muldefs")