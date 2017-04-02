FILES = source.c unloop1.c obfusc1.c obfusc2.c obfusc3.c

all: $(FILES)

source.c: ../cweb/wc.web
	../../src/ctangle/ctangle ../cweb/wc.web | sed '/^#line/d' >source.c

unloop1.c: source.c
	../../src/cunloop/cunloop <source.c >unloop1.c

obfusc1.c: source.c
	../../src/cobfusc/cobfusc -iword <source.c >obfusc1.c

obfusc2.c: source.c
	../../src/cobfusc/cobfusc -e <source.c >obfusc2.c

obfusc3.c: unloop1.c
	../../src/cobfusc/cobfusc -adem -inumeric <unloop1.c >obfusc3.c

clean:
	rm -f $(FILES)
