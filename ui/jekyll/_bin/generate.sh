#!/bin/bash
for lang in ja en fr de
do
    for page in broadcast index relays tree connections logout settings viewlog
    do
        ln -fs _orig/$page.html $page.$lang.html
    done
done
