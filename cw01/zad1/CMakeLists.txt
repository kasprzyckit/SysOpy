set(CMAKE_C_STANDARD 99)

add_library(CharBlocksStatic STATIC ${SOURCE_FILES} ${HEADER_FILES})
add_library(CharBlocksShared SHARED ${SOURCE_FILES})