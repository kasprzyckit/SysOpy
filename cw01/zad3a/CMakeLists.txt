set(CMAKE_C_STANDARD 99)

SET(add_clean_files results3a.txt)
SET_DIRECTORY_PROPERTIES(PROPERTIES  
ADDITIONAL_MAKE_CLEAN_FILES "${add_clean_files}")

add_executable(CharBlocksMainStatic ${MAIN_FILE} )
target_link_libraries (CharBlocksMainStatic CharBlocksStatic)

add_executable(CharBlocksMainShared ${MAIN_FILE} )
target_link_libraries (CharBlocksMainShared CharBlocksShared)

add_executable(CharBlocksMainDynamic ${MAIN_FILE} )
set_target_properties(CharBlocksMainDynamic PROPERTIES COMPILE_FLAGS "-fPIC -ldl -DDYNAMICLOAD")
target_link_libraries(CharBlocksMainDynamic ${CMAKE_DL_LIBS})

add_custom_target(CharBlocksLibraries
	DEPENDS CharBlocksMainDynamic CharBlocksMainStatic CharBlocksMainShared CharBlocksShared
		COMMAND printf "_____Static library_____" > results3a.txt
		COMMAND ./CharBlocksMainStatic ${ARGS} | tee -a results3a.txt
		COMMAND printf "_____Shared library_____" >> results3a.txt
		COMMAND ./CharBlocksMainShared ${ARGS} | tee -a results3a.txt
		COMMAND printf "_____Dynamically loaded library_____" >> results3a.txt
		COMMAND ./CharBlocksMainDynamic ${ARGS} | tee -a results3a.txt)