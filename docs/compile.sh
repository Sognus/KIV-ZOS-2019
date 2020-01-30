#!/bin/bash
#run pdflatex a few times
for i in {1..5}
do
		pdflatex zos.tex
	done

	# Clean non-pdf files
	rm zos.aux 2> /dev/null
	rm zos.log 2> /dev/null
	rm zos.lot 2> /dev/null
	rm zos.out 2> /dev/null
	rm zos.toc 2> /dev/null
