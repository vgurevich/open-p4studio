INCLUDE(ExternalProject)

# install tofino-model by arch
if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
  install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/${BF_PKG_DIR}/tofino-model/tofino-model.x86_64.bin DESTINATION bin RENAME tofino-model)
else()
  install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/${BF_PKG_DIR}/tofino-model/tofino-model.i686.bin DESTINATION bin RENAME tofino-model)
endif()

# install bf-p4c
ExternalProject_Add(bf-p4c
  DOWNLOAD_COMMAND ""
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
)
if (STANDALONE)
  set(P4C ${CMAKE_INSTALL_PREFIX}/bin/bf-p4c)
  set(P4C-GEN-BFRT-CONF ${CMAKE_INSTALL_PREFIX}/bin/p4c-gen-bfrt-conf)
  set(P4C-MANIFEST-CONFIG ${CMAKE_INSTALL_PREFIX}/bin/p4c-manifest-config)
else()
  # Use the build-local binaries if we are not building standalone.
  set(P4C ${CMAKE_BINARY_DIR}/${BF_PKG_DIR}/p4-compilers/p4c/bf-p4c)
  set(P4C-GEN-BFRT-CONF ${CMAKE_BINARY_DIR}/${BF_PKG_DIR}/p4-compilers/p4c/p4c-gen-bfrt-conf)
  set(P4C-MANIFEST-CONFIG ${CMAKE_BINARY_DIR}/${BF_PKG_DIR}/p4-compilers/p4c/p4c-manifest-config)
endif()
include(PythonDependencies)
set(PDGEN ${CMAKE_INSTALL_PREFIX}/bin/generate_tofino_pd)
set(PDGEN_COMMAND ${PYTHON_COMMAND} ${PDGEN})
set(PDGENCLI ${CMAKE_INSTALL_PREFIX}/bin/gencli)
set(PDGENCLI_COMMAND ${PYTHON_COMMAND} ${PDGENCLI})
set(PDSPLIT ${CMAKE_INSTALL_PREFIX}/bin/split_pd_thrift.py)
set(PDSPLIT_COMMAND ${PYTHON_COMMAND} ${PDSPLIT})

# install p4o
ExternalProject_Add(bf-p4o
  DOWNLOAD_COMMAND tar -xvf ${CMAKE_CURRENT_SOURCE_DIR}/${BF_PKG_DIR}/p4o/p4o-1.0.x86_64.tar.gz
      -C ${CMAKE_INSTALL_PREFIX} --strip-components=1
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
)
