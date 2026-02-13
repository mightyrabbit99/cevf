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
      cevf_extras.h
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
# set_target_properties(cevf-static PROPERTIES OUTPUT_NAME "cevf")
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

add_library(cevfm-tcpsrv SHARED)
set_target_properties(cevfm-tcpsrv PROPERTIES PREFIX "")
set_target_properties(cevfm-tcpsrv PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_sources(cevfm-tcpsrv
  PRIVATE
    extras/cevfm_tcpsrv.c
)
target_link_libraries(cevfm-tcpsrv
  PUBLIC
    cevf
)
target_include_directories(cevfm-tcpsrv
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/extras
)
install(TARGETS cevfm-tcpsrv
  LIBRARY DESTINATION lib/cevfms/
)

add_library(cevfm-tcpsrv-object OBJECT)
set_target_properties(cevfm-tcpsrv-object PROPERTIES EXCLUDE_FROM_ALL TRUE)
set_target_properties(cevfm-tcpsrv-object PROPERTIES PREFIX "")
set_target_properties(cevfm-tcpsrv-object PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_sources(cevfm-tcpsrv-object
  PRIVATE
    extras/cevfm_tcpsrv.c
)
target_include_directories(cevfm-tcpsrv-object
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/extras
)

add_library(cevfm-resumew SHARED)
set_target_properties(cevfm-resumew PROPERTIES PREFIX "")
set_target_properties(cevfm-resumew PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_sources(cevfm-resumew
  PRIVATE
    extras/cevfm_resumew.c
)
target_link_libraries(cevfm-resumew
  PUBLIC
    cevf
)
target_include_directories(cevfm-resumew
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/extras
)
install(TARGETS cevfm-resumew
  LIBRARY DESTINATION lib/cevfms/
)

add_library(cevfm-resumew-object OBJECT)
set_target_properties(cevfm-resumew-object PROPERTIES EXCLUDE_FROM_ALL TRUE)
set_target_properties(cevfm-resumew-object PROPERTIES PREFIX "")
set_target_properties(cevfm-resumew-object PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_sources(cevfm-resumew-object
  PRIVATE
    extras/cevfm_resumew.c
)
target_include_directories(cevfm-resumew-object
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/extras
)

add_library(cevfm-usock SHARED)
set_target_properties(cevfm-usock PROPERTIES PREFIX "")
set_target_properties(cevfm-usock PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_sources(cevfm-usock
  PRIVATE
    extras/cevfm_usock.c
)
target_link_libraries(cevfm-usock
  PUBLIC
    cevf
)
target_include_directories(cevfm-usock
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/extras
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
)
install(TARGETS cevfm-usock
  LIBRARY DESTINATION lib/cevfms/
)

add_library(cevfm-usock-object OBJECT)
set_target_properties(cevfm-usock-object PROPERTIES EXCLUDE_FROM_ALL TRUE)
set_target_properties(cevfm-usock-object PROPERTIES PREFIX "")
set_target_properties(cevfm-usock-object PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_sources(cevfm-usock-object
  PRIVATE
    extras/cevfm_usock.c
)
target_include_directories(cevfm-usock-object
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/extras
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
)

add_library(cevfm-assdisass SHARED)
set_target_properties(cevfm-assdisass PROPERTIES PREFIX "")
set_target_properties(cevfm-assdisass PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_sources(cevfm-assdisass
  PRIVATE
    extras/cevfm_assdisass.c
    utils/map.c
)
target_link_libraries(cevfm-assdisass
  PUBLIC
    cevf
)
target_include_directories(cevfm-assdisass
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/extras
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
)
install(TARGETS cevfm-assdisass
  LIBRARY DESTINATION lib/cevfms/
)

add_library(cevfm-assdisass-object OBJECT)
set_target_properties(cevfm-assdisass-object PROPERTIES EXCLUDE_FROM_ALL TRUE)
set_target_properties(cevfm-assdisass-object PROPERTIES PREFIX "")
set_target_properties(cevfm-assdisass-object PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_sources(cevfm-assdisass-object
  PRIVATE
    extras/cevfm_assdisass.c
)
target_include_directories(cevfm-assdisass-object
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/extras
)

if(BUILD_CEVF_LUA)
add_library(lua STATIC IMPORTED)
set_target_properties(lua PROPERTIES IMPORTED_LOCATION "${LUA_LIBRARY}")

add_library(cevfm-luamac SHARED)
set_target_properties(cevfm-luamac PROPERTIES PREFIX "")
set_target_properties(cevfm-luamac PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_sources(cevfm-luamac
  PRIVATE
    extras/cevfm_luamac.c
)
target_link_libraries(cevfm-luamac
  PUBLIC
    cevf
    lua
)
target_include_directories(cevfm-luamac
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${LUA_INCLUDE_DIR}
)
install(TARGETS cevfm-luamac
  LIBRARY DESTINATION lib/cevfms/
)

add_library(cevfm-luamac-object OBJECT)
set_target_properties(cevfm-luamac-object PROPERTIES EXCLUDE_FROM_ALL TRUE)
set_target_properties(cevfm-luamac-object PROPERTIES PREFIX "")
set_target_properties(cevfm-luamac-object PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_sources(cevfm-luamac-object
  PRIVATE
    extras/cevfm_luamac.c
)
target_link_libraries(cevfm-luamac-object
  PUBLIC
    lua
)
target_include_directories(cevfm-luamac-object
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${LUA_INCLUDE_DIR}
)
endif()
