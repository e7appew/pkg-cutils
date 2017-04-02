all: wc.c wc.tex wc

wc.c: wc.web
	../../src/ctangle/ctangle wc.web >wc.c

wc.tex: wc.web
	../../src/cweave/cweave -h wc.web >wc.tex

wc: wc.c
	cc -o wc wc.c

wc.dvi:	wc.tex
	latex wc.tex
	latex wc.tex
	latex wc.tex

clean:
	rm -f wc wc.c wc.tex wc.dvi wc.aux wc.toc wc.log
