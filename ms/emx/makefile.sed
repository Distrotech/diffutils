s|@CC@|gcc|g
s|@CFLAGS@|-g -O|g
s|@DEFS@|-DHAVE_CONFIG_H|g
s|@LIBS@|pc/diff.def|g
s|@LIBOBJS@|pc.o|g
s|@VPATH@|.|g
s|@srcdir@|.|g
s|@[0-9A-Z_a-z]*@||g

s|cmp|&.exe|g
s|s*diff3*|&.exe|g
s|\.exe\([.0-9A-Z_a-z]\)|\1|g

s|\.exe:|& pc/diff.def|

s|regex.o|$(REGEX)|g
s|\$(REGEX):|regex.o:|g

s|\.o|$o|g
s|\*\$o|*.o|g

s|^	rm -f|	-del|g
s|^\(DEFAULT_EDITOR_PROGRAM *=\).*|\1 edit|
s|^\(DIFF_PROGRAM *=\).*|\1 diff.exe|
s|^\(NULL_DEVICE *=\).*|\1 /dev/nul|
s|^\(PR_PROGRAM *=\).*|\1 pr|
s|^clean:|& pc-clean|
s|^config.h:|#&|

s|[	 ][	 ]*$||
