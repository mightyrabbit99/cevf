add_executable(tcp1)
target_sources(tcp1
  PRIVATE
    tcp1.c
    mods/mod_pre.c
)
target_link_libraries(tcp1
  PUBLIC
    cevf
)
target_include_directories(tcp1
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)
install(TARGETS tcp1
  DESTINATION bin
)
