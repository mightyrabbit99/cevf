add_library(cevfm-hello1 SHARED)
set_target_properties(cevfm-hello1 PROPERTIES PREFIX "")
set_target_properties(cevfm-hello1 PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_compile_options(cevfm-hello1 PRIVATE -rdynamic)
target_sources(cevfm-hello1
  PRIVATE
    hello1.c
)
target_link_libraries(cevfm-hello1
  PUBLIC
    cevf
)
target_include_directories(cevfm-hello1
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
install(TARGETS cevfm-hello1
  LIBRARY DESTINATION lib/cevfms/
)
