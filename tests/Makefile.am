AUTOMAKE_OPTIONS = color-tests
check_DATA = table.txt final_report.txt
CLEANFILES = $(pkg_DATA) table* final_report* *.int
dist_check_SCRIPTS = maketable.awk makereport.awk
TESTS_ENVIRONMENT = PATH=$(top_srcdir):$$PATH
TESTS = test_int2bin.sh test_illm2bin-1.sh test_illm2bin-2.sh test_bin2int-1.sh test_bin2int-2.sh test_bin2int-3.sh test_bin2int-4.sh
EXTRA_DIST = $(TESTS)

table.txt: maketable.awk
	(echo Barcode; for ((i=1; i<=100; i+=1)); do echo id$$i; done) | $(AWK) -f $< > $@
final_report.txt: makereport.awk table.txt
	$(AWK) -f $^ > $@
