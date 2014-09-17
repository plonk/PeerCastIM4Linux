#!/bin/bash
for lang in ja en fr de
do
    mkdir -p $lang
    rm -f $lang/images
    ln -fs ../_orig/images $lang/images
    for page in broadcast index relays tree connections logout settings viewlog bcid chaninfo tree login relayinfo
    do
        ln -fs _orig/$page.html $page.$lang.html
    done
done
