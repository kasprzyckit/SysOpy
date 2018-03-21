add_executable(Directory directory.c stack.c)

add_custom_target(DirectoryTest
		DEPENDS Directory
		COMMAND ./Directory . "=" stack.c)