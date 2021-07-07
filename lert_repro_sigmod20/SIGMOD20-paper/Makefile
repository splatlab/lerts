DOCA=main
OPTS = -pdf 
LATEX = pdflatex

default:
	@if (command -v latexmk > /dev/null) ; then latexmk $(OPTS) $(DOCA) ; else make brute; fi 

cont: 
	latexmk -pvc $(OPTS) $(DOCA)

clean:
	latexmk -C
