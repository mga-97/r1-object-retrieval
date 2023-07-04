set(appname GoAndPoint)

file(GLOB conf      ${CMAKE_CURRENT_SOURCE_DIR}/conf/*.ini)
file(GLOB templates ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.template)
file(GLOB apps      ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.xml)

yarp_install(FILES ${conf}    DESTINATION ${R1_OBJECT_RETRIEVAL_CONTEXTS_INSTALL_DIR}/${appname})
yarp_install(FILES ${apps}    DESTINATION ${R1_OBJECT_RETRIEVAL_APPLICATIONS_INSTALL_DIR})
yarp_install(FILES ${scripts} DESTINATION ${R1_OBJECT_RETRIEVAL_APPLICATIONS_TEMPLATES_INSTALL_DIR})

