add_executable(Filter filter.c)
target_link_libraries(Filter pthread)
target_link_libraries(Filter m)