AUTOMAKE_OPTIONS = color-tests
check_DATA = table.txt final_report.txt
CLEANFILES = $(pkg_DATA) table* final_report* *.int
dist_check_SCRIPTS = maketable.awk makereport.awk
TESTS_ENVIRONMENT = PATH=$(top_srcdir):$$PATH
TESTS = test_int2bin-1.sh test_int2bin-2.sh test_int2bin-3.sh test_int2bin-4.sh test_illm2bin-1.sh test_illm2bin-2.sh test_bin2int-1.sh test_bin2int-2.sh test_bin2int-3.sh test_bin2int-4.sh test_bin2int-5.sh
EXTRA_DIST = $(TESTS)

table.txt: maketable.awk
	(echo Barcode; i=1; while [ $$i -lt 101 ]; do echo id$$i; i=$$((i + 1)); done) | $(AWK) -f maketable.awk > $@
final_report.txt: makereport.awk table.txt
	$(AWK) -f makereport.awk table.txt > $@
