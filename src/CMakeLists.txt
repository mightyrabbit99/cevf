add_library(cevf STATIC)
add_compile_definitions(CONFIG_ELOOP_EPOLL)
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
      cevf_main.h
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