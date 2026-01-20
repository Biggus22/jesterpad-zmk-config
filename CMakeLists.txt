if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config/dts")
	list(APPEND DTS_ROOTS "${CMAKE_CURRENT_SOURCE_DIR}/config")
endif()

target_sources(app PRIVATE src/bt_pair_hold.c src/rgb_cycle.c)
