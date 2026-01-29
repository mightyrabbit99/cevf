add_library(cevfm-hello2 SHARED)
set_target_properties(cevfm-hello2 PROPERTIES PREFIX "")
set_target_properties(cevfm-hello2 PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_compile_options(cevfm-hello2 PRIVATE -rdynamic)
target_sources(cevfm-hello2
  PRIVATE
    hello2.c
)
target_link_libraries(cevfm-hello2
  PUBLIC
    cevf
)
target_include_directories(cevfm-hello2
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
install(TARGETS cevfm-hello2
  LIBRARY DESTINATION lib/cevfms/
)
