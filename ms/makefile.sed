1i\
default: debug\
o = .o\
.SUFFIXES: .c $o\
REGEX = regex$o

: again

s|@CC@|gcc|g
s|@CFLAGS@|-Wall -O2|g
s|@DEFS@|-DHAVE_CONFIG_H|g
s|@INSTALL@|xcopy|g
s|@INSTALL_DATA@|xcopy|g
s|@INSTALL_PROGRAM@|xcopy|g
s|@LDFLAGS@|-s|g
s|@LIBOBJS@|pc.o|g
s|@VPATH@|.|g
s|@exec_prefix@|c:/$(env)|g
s|@prefix@|c:/$(env)/docs|g
s|@srcdir@|.|g
s|@[0-9A-Z_a-z]*@||g

s|quote.o *||g
s|regex.o|$(REGEX)|g
s|\$(REGEX):|regex.o:|g

/^[^#]/{
	s|;||g
	s|\$\$|%%|g

	s|\.o|$o|g
	s|\*\$o|*.o|g
}

s|^\(DEFAULT_DIFF_PROGRAM *=\).*|\1 diff|
s|^\(DEFAULT_EDITOR_PROGRAM *=\).*|\1 edit|
s|^\(NULL_DEVICE *=\).*|\1 nul|
s|^\(PR_PROGRAM *=\).*|\1 pr|
/^SHELL *=/{
	s|^|# |
	a\
SUBSHELL = command >nul /c
}

s|^	rm -f *\(.*\)|	$(SUBSHELL) for %%f in (\1) do del %%f|g
/^	for \([a-z]\) in \(.*\) do /{
	s||	$(SUBSHELL) for %%\1 in (\2) do |
	/\\$/{
		N
		s| *\\\n[	 ]*| |
		b again
	}
}
/^	done$/d
s|`echo ||g
s/ | \$[(]edit_program_name.*//g
/test  *-f/d
s|	.*/mkinstalldirs *\(.*\)|	$(SUBSHELL) for %%d in ($(exec_prefix) $(prefix) \1) do md %%d|g
s|^	\$(INSTALL_DATA)|	$(SUBSHELL) $(INSTALL_DATA)|
s|\(\$(INSTALL[^)]*) %%.\)\( \$([^)]*)\)/%%.|\1.exe\2|g
s|\.info|.inf|g
s|rm -f|del|g
s|\$(bindir).%%.|&.exe|

s|^clean:|& pc-clean|
s|^config.h:|#&|

/^dist:/,/^[	]*$/d
/^Makefile:/,/^[	]*$/d

/^[A-Z_a-z][0-9A-Z_a-z]*[ 	]*=/s|/|\\|g
/^	$(SUBSHELL)/s|/|\\|g

s|[	 ]*$||

$r pc/makefile
