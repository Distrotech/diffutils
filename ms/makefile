

# Additional rules for pc makefiles.

# This rule uses /, not \, so it can also be run from Posix-compliant hosts.
makefile : Makefile.in pc/makefile pc/makefile.sed pc/$(env)/makefile.sed
	sed -f pc/makefile.sed -f pc/$(env)/makefile.sed <Makefile.in >$@

config.h: pc\$(env)\config.h
	copy $? $@

$(diff_o) $(cmp_o) $(sdiff_o) $(diff3_o): config.h pc\config.h
$(PROGRAMS): $(LIBS)

pc$o: pc\pc.c
	$(COMPILE) pc\pc.c -o $@

debug dos-debug: config.h
	$(MAKE) all \
		CFLAGS="-g -O -Wall" \
		LDFLAGS=
dos-release: config.h all

os2-debug : dos-debug
os2-release: config.h
	$(MAKE) all gnuregex.dll \
		CFLAGS="-Zomf -Zmtd -O2 -Wall" \
		LDFLAGS="-s" \
		o=.obj \
		REGEX=regex.lib
gnuregex.dll: regex$o pc\$(env)\gnuregex.def
	$(CC) $(CFLAGS) -Zdll regex$o -o $@ pc\$(env)\gnuregex.def
regex.lib : pc\$(env)\gnuregex.def
	emximp -o $@ pc\$(env)\gnuregex.def

pc-clean:
	$(SUBSHELL) for %%f in (*.o* *.dll *.lib) do del %%f

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
