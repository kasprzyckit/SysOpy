set(CMAKE_C_STANDARD 99)

SET(add_clean_files raport2.txt)
SET_DIRECTORY_PROPERTIES(PROPERTIES  
ADDITIONAL_MAKE_CLEAN_FILES "${add_clean_files}")

add_executable(CharBlocksMainTest ${MAIN_FILE} ${SOURCE_FILES})

add_custom_target(CharBlocksTest
	DEPENDS CharBlocksMainTest
		COMMAND printf "_____Test functions_____" > raport2.txt
		COMMAND ./CharBlocksMainTest ${ARGS} | tee -a raport2.txt)