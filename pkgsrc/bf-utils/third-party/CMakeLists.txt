INCLUDE(ExternalProject)
INCLUDE(ProcessorCount)
ProcessorCount(N)

set(C_FLAGS " -O2 -fPIC")
if (FORCE32)
  set(C_FLAGS "${C_FLAGS} -m32")
endif()

if(ASAN)
  set(C_FLAGS "${C_FLAGS} -fsanitize=address -fsanitize-recover=address -fsanitize=leak")
endif()

EXTERNALPROJECT_ADD(
    build_edit
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libedit-3.1
    CONFIGURE_COMMAND autoreconf -fi ${CMAKE_CURRENT_SOURCE_DIR}/libedit-3.1 && ${CMAKE_CURRENT_SOURCE_DIR}/libedit-3.1/configure --prefix=${CMAKE_INSTALL_PREFIX} --disable-shared CFLAGS=${C_FLAGS} > /dev/null
    BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/libedit-3.1
    BUILD_COMMAND ${CMAKE_MAKE_PROGRAM} -j${N} > /dev/null
    INSTALL_COMMAND ${CMAKE_MAKE_PROGRAM} install > /dev/null
)
