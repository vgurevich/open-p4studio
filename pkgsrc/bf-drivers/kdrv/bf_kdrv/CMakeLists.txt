
project(bf_kdrv VERSION 0.1.0 LANGUAGES C)

set(DRIVER_FILE bf_kdrv.ko)
set(KBUILD_CMD $(MAKE) -C ${KERNELDIR} modules M=${CMAKE_CURRENT_BINARY_DIR} src=${CMAKE_CURRENT_SOURCE_DIR})
set(KBUILD_CLEAN $(MAKE) -C ${KERNELDIR} clean M=${CMAKE_CURRENT_BINARY_DIR} src=${CMAKE_CURRENT_SOURCE_DIR})

# Generate the Kbuild file through cmake.
FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/Kbuild "obj-m := bf_kdrv.o\n")
FILE(APPEND ${CMAKE_CURRENT_BINARY_DIR}/Kbuild "ccflags-y += -I${CMAKE_CURRENT_SOURCE_DIR}/../../include\n")
FILE(APPEND ${CMAKE_CURRENT_BINARY_DIR}/Kbuild "ccflags-y += -I$(inc) -D__KERNEL__")

# build
add_custom_command(OUTPUT ${DRIVER_FILE}
  COMMAND ${KBUILD_CMD}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/bf_kdrv.c
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_custom_target(bf_kdrv ALL DEPENDS ${DRIVER_FILE})

# clean
add_custom_target(bf_kdrv_clean COMMAND ${KBUILD_CLEAN})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/bf_kdrv.ko DESTINATION lib/modules)
