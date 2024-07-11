
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/mqtt_comm.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/user_app.c)

zephyr_include_directories(.)