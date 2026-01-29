add_library(cevfm-tcp1 SHARED)
set_target_properties(cevfm-tcp1 PROPERTIES PREFIX "")
set_target_properties(cevfm-tcp1 PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_compile_options(cevfm-tcp1 PRIVATE -rdynamic)
target_sources(cevfm-tcp1
  PRIVATE
    tcp1.c
)
target_link_libraries(cevfm-tcp1
  PUBLIC
    cevf
)
target_include_directories(cevfm-tcp1
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
install(TARGETS cevfm-tcp1
  LIBRARY DESTINATION lib/cevfms/
)
