add_executable(BatchProcessing batch_processing.c words.c)

add_custom_target(BatchTest
		DEPENDS BatchProcessing
		COMMAND ./BatchProcessing input.txt)