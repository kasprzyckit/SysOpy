set(CMAKE_C_STANDARD 99)

SET(add_clean_files results3b.txt)
SET_DIRECTORY_PROPERTIES(PROPERTIES  
ADDITIONAL_MAKE_CLEAN_FILES "${add_clean_files}")

add_library(CharBlocksOptimizationNone ${SOURCE_FILES})
set_target_properties(CharBlocksOptimizationNone PROPERTIES COMPILE_FLAGS "-O0")

add_library(CharBlocksOptimization1 ${SOURCE_FILES})
set_target_properties(CharBlocksOptimization1 PROPERTIES COMPILE_FLAGS "-O1")

add_library(CharBlocksOptimization2 ${SOURCE_FILES})
set_target_properties(CharBlocksOptimization2 PROPERTIES COMPILE_FLAGS "-O2")

add_library(CharBlocksOptimization3 ${SOURCE_FILES})
set_target_properties(CharBlocksOptimization3 PROPERTIES COMPILE_FLAGS "-O3")

add_library(CharBlocksOptimizationFast ${SOURCE_FILES})
set_target_properties(CharBlocksOptimizationFast PROPERTIES COMPILE_FLAGS "-Ofast")

add_library(CharBlocksOptimizationSize ${SOURCE_FILES})
set_target_properties(CharBlocksOptimizationSize PROPERTIES COMPILE_FLAGS "-Os")

add_executable(MainOptimizationNone ${MAIN_FILE})
target_link_libraries (MainOptimizationNone CharBlocksOptimizationNone)
set_target_properties(MainOptimizationNone PROPERTIES COMPILE_FLAGS "-O0")

add_executable(MainOptimization1 ${MAIN_FILE})
target_link_libraries (MainOptimization1 CharBlocksOptimization1)
set_target_properties(MainOptimization1 PROPERTIES COMPILE_FLAGS "-O1")

add_executable(MainOptimization2 ${MAIN_FILE})
target_link_libraries (MainOptimization2 CharBlocksOptimization2)
set_target_properties(MainOptimization2 PROPERTIES COMPILE_FLAGS "-O2")

add_executable(MainOptimization3 ${MAIN_FILE})
target_link_libraries (MainOptimization3 CharBlocksOptimization3)
set_target_properties(MainOptimization3 PROPERTIES COMPILE_FLAGS "-O3")

add_executable(MainOptimizationFast ${MAIN_FILE})
target_link_libraries (MainOptimizationFast CharBlocksOptimizationFast)
set_target_properties(MainOptimizationFast PROPERTIES COMPILE_FLAGS "-Ofast")

add_executable(MainOptimizationSize ${MAIN_FILE})
target_link_libraries (MainOptimizationSize CharBlocksOptimizationSize)
set_target_properties(MainOptimizationSize PROPERTIES COMPILE_FLAGS "-Os")


add_custom_target(CharBlocksOptimization
	DEPENDS MainOptimizationNone MainOptimization1 MainOptimization2 MainOptimization3 MainOptimizationFast MainOptimizationSize
		COMMAND printf "_____No optimization_____" > results3b.txt
		COMMAND ./MainOptimizationNone ${ARGS} | tee -a results3b.txt
		COMMAND printf "_____Optimization 1_____" >> results3b.txt
		COMMAND ./MainOptimization1 ${ARGS}| tee -a results3b.txt
		COMMAND printf "_____Optimization 2_____" >> results3b.txt
		COMMAND ./MainOptimization2 ${ARGS} | tee -a results3b.txt
		COMMAND printf "_____Optimization 3_____" >> results3b.txt
		COMMAND ./MainOptimization3 ${ARGS} | tee -a results3b.txt
		COMMAND printf "_____Optimization fast_____" >> results3b.txt
		COMMAND ./MainOptimizationFast ${ARGS} | tee -a results3b.txt
		COMMAND printf "_____Optimization size_____" >> results3b.txt
		COMMAND ./MainOptimizationSize ${ARGS} | tee -a results3b.txt)