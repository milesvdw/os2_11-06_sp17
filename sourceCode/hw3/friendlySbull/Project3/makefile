WRITEUP=hw3_writeup

all:
	make tex
#	make concurrency	

tex:
	latex $(WRITEUP).tex
	dvips $(WRITEUP).dvi
	ps2pdf $(WRITEUP).ps
	cp -f $(WRITEUP).pdf ~/public_html
	chmod a+r ~/public_html/$(WRITEUP).pdf

#concurrency:
#	gcc -g phil.c -o phil -lpthread -lrt


clean:
	rm -f *.out *.log *.dvi *.aux *.ps *.pdf *.o *.blg *.bbl
