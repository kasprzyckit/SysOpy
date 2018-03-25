add_executable(BatchLimits batch_processing.c words.c)
add_executable(TestMemory test_memory.c)
add_executable(TestCPU test_cpu.c)

add_custom_target(BatchLimitsTest
		DEPENDS BatchLimits TestMemory TestCPU
		COMMAND ./BatchLimits input.txt 5 15)