

# Additional rules for pc makefiles.

# Check that pc/$(ENV)/Makefile was properly derived from Makefile.in.
# This rule uses /, not \, so that it can also be run from Posix.
pc/$(ENV)/Makefile.chk: Makefile.in \
  pc/Makefile.add pc/Makefile.sed pc/$(ENV)/Makefile.sed
	sed -f pc/Makefile.sed -f pc/$(ENV)/Makefile.sed <Makefile.in >$@
	cat pc/Makefile.add >>$@

check-makefile: pc/$(ENV)/Makefile.chk
	diff pc/$(ENV)/Makefile pc/$(ENV)/Makefile.chk

config.h: pc\$(ENV)\config.h
	copy $? $@

$(diff_o) $(cmp_o) $(sdiff_o) $(diff3_o): config.h pc\config.h
$(PROGRAMS): $(LIBS)

pc$o: pc\pc.c
	$(COMPILE) pc\pc.c -o $@

debug dos-debug: config.h
	$(MAKE) -f pc\Makefile all \
		CFLAGS="-g -O -Wall" \
		LDFLAGS=
dos-release: config.h all

os2-debug : dos-debug
os2-release: config.h
	$(MAKE) -f pc\Makefile all gnuregex.dll \
		CFLAGS="-Zomf -Zmtd -O2 -Wall" \
		LDFLAGS="-s" \
		o=.obj \
		REGEX=regex.lib
gnuregex.dll: regex$o pc\$(ENV)\gnuregex.def
	$(CC) $(CFLAGS) -Zdll regex$o -o $@ pc\$(ENV)\gnuregex.def
regex.lib : pc\$(ENV)\gnuregex.def
	emximp -o $@ pc\$(ENV)\gnuregex.def

pc-clean:
	$(SUBSHELL) for %%f in (*.o* *.dll *.lib pc\*\Makefile.chk) do del %%f

%.exe: %
	coff2exe $<

dist:
	sed -e /version_string/!d -e "s/[^0-9]*\([0-9a-z.]*\).*/diff-\1/" -e "s/\([0-9]\)\./\1/g" -e q version.c > fname
	echo @echo off > dist.bat
	sed -e "s|.*|md &|" fname >> dist.bat
	sed -e "s|.*|del &\\\*.*|" fname >> dist.bat
	sed -e "s|.*|xcopy *.* & /s/e|" fname >> dist.bat
	sed -e "s|.*|del &\\\*.o*|" fname >> dist.bat
	sed -e "s|.*|del &\\\diff*|" fname >> dist.bat
	sed -e "s|.*|del &\\\sdiff|" fname >> dist.bat
	sed -e "s|.*|del &\\\cmp|" fname >> dist.bat
	sed -e "s|.*|del &\\\dist.bat|" fname >> dist.bat
	sed -e "s|.*|del &\\\fname|" fname >> dist.bat
	sed -e "s|.*|del &\\\~*.*|" fname >> dist.bat
	sed -e "s|.*|pkzip -rPmex &.zip &\\\*.*|" fname >> dist.bat
	.\dist.bat
	del dist.bat fname
