SET(add_clean_files records records1 records2 wyniki_copy.txt wyniki_sort.txt)
SET_DIRECTORY_PROPERTIES(PROPERTIES  
ADDITIONAL_MAKE_CLEAN_FILES "${add_clean_files}")

add_executable(InOutCompare inout_compare.c)

add_custom_target(MeasureTime
		DEPENDS InOutCompare
		COMMAND printf "1000 records of 4 bytes"
		COMMAND ./InOutCompare generate records 1000 4
		COMMAND ./InOutCompare copy records records1 1000 4 sys > wyniki_copy.txt
		COMMAND ./InOutCompare copy records records2 1000 4 lib >> wyniki_copy.txt
		COMMAND ./InOutCompare sort records1 1000 4 sys > wyniki_sort.txt
		COMMAND ./InOutCompare sort records2 1000 4 lib >> wyniki_sort.txt
		COMMAND printf "5000 records of 4 bytes"
		COMMAND ./InOutCompare generate records 5000 4
		COMMAND ./InOutCompare copy records records1 5000 4 sys >> wyniki_copy.txt
		COMMAND ./InOutCompare copy records records2 5000 4 lib >> wyniki_copy.txt
		COMMAND ./InOutCompare sort records1 5000 4 sys >> wyniki_sort.txt
		COMMAND ./InOutCompare sort records2 5000 4 lib >> wyniki_sort.txt

		COMMAND printf "1000 records of 512 bytes"
		COMMAND ./InOutCompare generate records 1000 512
		COMMAND ./InOutCompare copy records records1 1000 512 sys >> wyniki_copy.txt
		COMMAND ./InOutCompare copy records records2 1000 512 lib >> wyniki_copy.txt
		COMMAND ./InOutCompare sort records1 1000 512 sys >> wyniki_sort.txt
		COMMAND ./InOutCompare sort records2 1000 512 lib >> wyniki_sort.txt
		COMMAND printf "5000 records of 512 bytes"
		COMMAND ./InOutCompare generate records 5000 512
		COMMAND ./InOutCompare copy records records1 5000 512 sys >> wyniki_copy.txt
		COMMAND ./InOutCompare copy records records2 5000 512 lib >> wyniki_copy.txt
		COMMAND ./InOutCompare sort records1 5000 512 sys >> wyniki_sort.txt
		COMMAND ./InOutCompare sort records2 5000 512 lib >> wyniki_sort.txt

		COMMAND printf "1000 records of 4096 bytes"
		COMMAND ./InOutCompare generate records 1000 4096
		COMMAND ./InOutCompare copy records records1 1000 4096 sys >> wyniki_copy.txt
		COMMAND ./InOutCompare copy records records2 1000 4096 lib >> wyniki_copy.txt
		COMMAND ./InOutCompare sort records1 1000 4096 sys >> wyniki_sort.txt
		COMMAND ./InOutCompare sort records2 1000 4096 lib >> wyniki_sort.txt
		COMMAND printf "5000 records of 4096 bytes"
		COMMAND ./InOutCompare generate records 5000 4096
		COMMAND ./InOutCompare copy records records1 5000 4096 sys >> wyniki_copy.txt
		COMMAND ./InOutCompare copy records records2 5000 4096 lib >> wyniki_copy.txt
		COMMAND ./InOutCompare sort records1 5000 4096 sys >> wyniki_sort.txt
		COMMAND ./InOutCompare sort records2 5000 4096 lib >> wyniki_sort.txt

		COMMAND printf "1000 records of 8192 bytes"
		COMMAND ./InOutCompare generate records 1000 8192
		COMMAND ./InOutCompare copy records records1 1000 8192 sys >> wyniki_copy.txt
		COMMAND ./InOutCompare copy records records2 1000 8192 lib >> wyniki_copy.txt
		COMMAND ./InOutCompare sort records1 1000 8192 sys >> wyniki_sort.txt
		COMMAND ./InOutCompare sort records2 1000 8192 lib >> wyniki_sort.txt
		COMMAND printf "5000 records of 8192 bytes"
		COMMAND ./InOutCompare generate records 5000 8192
		COMMAND ./InOutCompare copy records records1 5000 8192 sys >> wyniki_copy.txt
		COMMAND ./InOutCompare copy records records2 5000 8192 lib >> wyniki_copy.txt
		COMMAND ./InOutCompare sort records1 5000 8192 sys >> wyniki_sort.txt
		COMMAND ./InOutCompare sort records2 5000 8192 lib >> wyniki_sort.txt

		COMMAND cat wyniki_copy.txt > wyniki.txt
		COMMAND cat wyniki_sort.txt >> wyniki.txt
		)