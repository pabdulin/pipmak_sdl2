Reference.pdf: Reference.tex indexstyle.ist
	mkdir -p last; \
	until ([ Reference.pdf -nt Reference.tex ] && (diff -qs Reference.toc last/Reference.toc) && (diff -qs Reference.aux last/Reference.aux) && (diff -qs Reference.ind last/Reference.ind)); do \
		touch Reference.toc Reference.aux Reference.ind; \
		cp -fp Reference.toc Reference.aux Reference.ind last; \
		pdflatex Reference.tex; \
		makeindex -s indexstyle.ist Reference.idx; \
	done; \
	rm -rf last

clean:
	rm -fv Reference.pdf Reference.aux Reference.idx Reference.log Reference.out Reference.toc Reference.ind Reference.ilg
	rm -rf last
