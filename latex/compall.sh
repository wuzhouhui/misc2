#!/bin/sh
texes=$(ls *.tex)
for tex in $texes
do
	xelatex $tex
done
