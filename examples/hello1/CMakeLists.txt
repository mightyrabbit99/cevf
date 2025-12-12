add_executable(hello1)
target_sources(hello1
  PRIVATE
    hello1.c
    mods/mod_kern.c
    mods/mod_pre.c
    parts/argum.c
)
target_link_libraries(hello1
  PUBLIC
    cevf
)
target_include_directories(hello1
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/parts
)
install(TARGETS hello1
  DESTINATION bin
)
