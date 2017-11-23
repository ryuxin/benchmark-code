#!/bin/bash

name="http://bbs.otianya.cn/post-funinfo-5390415-"
html=".shtml"

for i in $(seq 51 1 79)
do
	full="$name$i$html"
	outn="name_$i"
	wkhtmltopdf $full ./names/$outn.pdf
	sleep 5
#	echo "wkhtmltopdf $full ./names/$outn.pdf"
#	echo $full
done

