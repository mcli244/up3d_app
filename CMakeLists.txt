cmake_minimum_required (VERSION 2.8)

project (demo)

set (TAGET up3d_test)

set (EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

add_definitions(-g)
add_definitions(-DLINUX)
# Includes
include_directories (${PROJECT_SOURCE_DIR}
					 ${PROJECT_SOURCE_DIR}/inc
					 )
# Sources
set(SRCS

	${PROJECT_SOURCE_DIR}/src/crc-itu-t.c
	${PROJECT_SOURCE_DIR}/src/up3d_dat_pro.c	
	${PROJECT_SOURCE_DIR}/src/lwrb.c	
	${PROJECT_SOURCE_DIR}/src/server.c
	${PROJECT_SOURCE_DIR}/main.c
)
add_executable(${TAGET}
			${SRCS}
			)

target_link_libraries(${TAGET}
		pthread
		)
