s|@CC@|gcc|g
s|@INSTALL@|xcopy|g
s|@INSTALL_PROGRAM@|xcopy|g
s|@INSTALL_DATA@|xcopy|g
s|@CFLAGS@|-Wall -O2|g
s|@LDFLAGS@|-s|g
s|@DEFS@|-DHAVE_CONFIG_H|g
s|@LIBOBJS@|pc.o|g
s|@VPATH@|.|g
s|@srcdir@|.|g
s|@prefix@|c:/djgpp/docs|g
s|@exec_prefix@|c:/djgpp|g
s|^.*@program_transform_name@.*$||g
s|@[0-9A-Z_a-z]*@||g
s|^SHELL *=.*|SUBSHELL = command >nul /c|


s|^\(PROGRAMS *=.*\)\(cmp\)|\1\2.exe|g
s|^\(PROGRAMS *=.*\)\(sdiff\)|\1\2.exe|g
s|^\(PROGRAMS *=.*\)\(diff3\)|\1\2.exe|g
s|^\(PROGRAMS *=.*\)diff |\1diff.exe |g


s|^	rm -f *\(.*\)|	$(SUBSHELL) for %%f in (\1) do del %%f|g
s|^	for \([a-z]\) in \([^ ;]*\);|	$(SUBSHELL) for %%\1 in (\2)|g
s|/`echo |\\|g
s/ | \$[(]edit_program_name.*//g
s|\$\$\([a-z]\)|%%\1|g
s|\/%%\([a-z]\);|\\%%\1|g
s|\([^:] *\)diff.info\*\([^:]\)|\1diff.i*\2|g
s|\.info\*|.i*|g
/test  *-f/d
/	done/d
s|	.*\/mkinstalldirs *\(.*\)|	$(SUBSHELL) for %%d in (\1) do md %%d|g
s|/diff.i\*[)] do del|\\diff.i*) do del|g
s|rm -f|del|g

/^check:/i\
#\
# PC users, note:  the filenames here are deliberately given in\
# mixed-case, so the programs actually read and compare the files'\
# contents, not only their (identical) names.  This assumes that\
# filenames are compared case-sensitively, and same_file() macro\
# returns -1 under non-POSIX OS's.
s|	\.\/cmp cmp cmp|	./cmp CMP.EXE cmp.exe|
s|	\.\/diff diff diff|	./diff -s DIFF.EXE diff.exe|
s|	\.\/diff3 diff3 diff3 diff3|	./diff3 DIFF3.EXE ./diff3.EXE diff3.exe|
s|	\.\/sdiff sdiff sdiff|	./sdiff SDIFF.EXE sdiff.exe|

s|^\(DEFAULT_EDITOR_PROGRAM *=\).*|\1 edit|
s|^\(DIFF_PROGRAM *=\).*|\1 diff|
s|^\(NULL_DEVICE *=\).*|\1 /dev/nul|
s|^\(PR_PROGRAM *=\).*|\1 pr|

s|^config.h:|#&|
s|[	 ][	 ]*$||
/^dist:/,/^[	]*$/d
