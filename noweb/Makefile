all: PiPack.dvi LedGrid.dvi

PiPack.dvi: PiPack.tex
	latex PiPack.tex
	noindex PiPack.tex
	latex PiPack.tex

PiPack.tex: PiPack.nw
	noweave -latex -n -delay -autodefs c -index PiPack.nw > PiPack.tex

PiPack.h: PiPack.nw
	notangle -RPiPack.h -L PiPack.nw > PiPack.h

PiPack.c: PiPack.nw
	notangle -RPiPack.c -L PiPack.nw > PiPack.c

LedGrid.dvi: LedGrid.tex
	latex LedGrid.tex
	noindex LedGrid.tex
	latex LedGrid.tex

LedGrid.tex: LedGrid.nw
	noweave -latex -n -delay -autodefs c -index LedGrid.nw > LedGrid.tex

LedGrid.h: LedGrid.nw
	notangle -RLedGrid.h -L LedGrid.nw > LedGrid.h

LedGrid.c: LedGrid.nw
	notangle -RLedGrid.c -L LedGrid.nw > LedGrid.c

