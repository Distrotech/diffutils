1i\
env = emx

/^LIBS *=/s|=.*|= pc/$(env)/diff.def|

s|cmp|&.exe|g
s|s*diff3*|&.exe|g
s|\.exe\([$.0-9A-Z_a-z]\)|\1|g
