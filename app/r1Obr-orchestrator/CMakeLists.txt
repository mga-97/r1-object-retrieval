set(appname r1Obr-orchestrator)

file(GLOB conf                  ${CMAKE_CURRENT_SOURCE_DIR}/conf/*.ini)
file(GLOB robotInterface        ${CMAKE_CURRENT_SOURCE_DIR}/conf/robotInterface/*.xml)
file(GLOB dances                ${CMAKE_CURRENT_SOURCE_DIR}/conf/dances/*.ini)
file(GLOB templates             ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.template)
file(GLOB apps                  ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.xml)

yarp_install(FILES ${conf}    DESTINATION ${${PROJECT_NAME}_CONTEXTS_INSTALL_DIR}/${appname})
yarp_install(FILES ${apps}    DESTINATION ${${PROJECT_NAME}_APPLICATIONS_INSTALL_DIR})
yarp_install(FILES ${scripts} DESTINATION ${${PROJECT_NAME}_APPLICATIONS_TEMPLATES_INSTALL_DIR})
yarp_install(FILES ${robotInterface}    DESTINATION ${${PROJECT_NAME}_CONTEXTS_INSTALL_DIR}/${appname}/robotInterface)
yarp_install(FILES ${dances}  DESTINATION ${${PROJECT_NAME}_CONTEXTS_INSTALL_DIR}/${appname}/dances)