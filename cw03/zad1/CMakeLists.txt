add_executable(DirectoryMultithreaded opendir.c stack.c)

add_custom_target(DirectoryMultithreadedTest
		DEPENDS DirectoryMultithreaded
		COMMAND ./DirectoryMultithreaded . "=" ./opendir.c)