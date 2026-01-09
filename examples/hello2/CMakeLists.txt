add_executable(hello2)
target_sources(hello2
  PRIVATE
    hello2.c
    hello2_arg.c
    mods/mod_kern.c
    mods/mod_pre.c
)
target_link_libraries(hello2
  PUBLIC
    cevf
)
target_include_directories(hello2
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
install(TARGETS hello2
  DESTINATION bin
)
