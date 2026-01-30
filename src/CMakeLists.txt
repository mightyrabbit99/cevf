add_compile_definitions(CONFIG_ELOOP_EPOLL)
add_compile_definitions(__HASHMAP_REMOVABLE)

add_library(cevf SHARED)
set_target_properties(cevf PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_sources(cevf
  PRIVATE
    cevf.c
    cevf_ev.c
    utils/qmsg2.c
    utils/timespec.c
    utils/heap.c
    utils/map.c
    utils/ids.c
    utils/eloop/os.c
    utils/eloop/eloop.c
  PUBLIC
    cevf.h
  PUBLIC FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} FILES
      cevf.h
      cevf_mod.h
)
target_include_directories(cevf
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
)
install(TARGETS cevf
  FILE_SET HEADERS DESTINATION include
  LIBRARY DESTINATION lib
  PUBLIC_HEADER DESTINATION include
)

add_library(cevf-static STATIC)
target_compile_options(cevf-static PRIVATE -DCEVF_STATIC_LIB)
set_target_properties(cevf-static PROPERTIES OUTPUT_NAME "cevf")
set_target_properties(cevf-static PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_sources(cevf-static
  PRIVATE
    cevf.c
    cevf_ev.c
    cevfd.c
    utils/qmsg2.c
    utils/timespec.c
    utils/heap.c
    utils/map.c
    utils/ids.c
    utils/eloop/os.c
    utils/eloop/eloop.c
  PUBLIC
    cevf.h
  PUBLIC FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} FILES
      cevf.h
      cevf_mod.h
)
target_include_directories(cevf-static
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
)
install(TARGETS cevf-static
  FILE_SET HEADERS DESTINATION include
  LIBRARY DESTINATION lib
  PUBLIC_HEADER DESTINATION include
)

add_executable(cevfd)
target_link_options(cevfd PRIVATE -rdynamic)
set_target_properties(cevfd PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_sources(cevfd
  PRIVATE
    cevfd.c
)
target_link_libraries(cevfd
  PUBLIC
    cevf
)
target_include_directories(cevfd
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
)
install(TARGETS cevfd
  DESTINATION bin
)

if(BUILD_CEVF_LUA)
add_library(cevf-lualib SHARED)
set_target_properties(cevf-lualib PROPERTIES LIBRARY_OUTPUT_NAME "cevflua")
set_target_properties(cevf-lualib PROPERTIES PREFIX "")
set_target_properties(cevf-lualib PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_include_directories(cevf-lualib
  PRIVATE
    ${LUA_INCLUDE_DIR}
)
target_sources(cevf-lualib
  PRIVATE
    cevf_lualib.c
)
target_link_libraries(cevf-lualib
  PUBLIC
    cevf
)
install(TARGETS cevf-lualib
  FILE_SET HEADERS DESTINATION include
  LIBRARY DESTINATION lib/lua
  PUBLIC_HEADER DESTINATION include
)
endif()
