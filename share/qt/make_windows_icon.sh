#!/bin/bash
# create multiresolution windows icon
ICON_DST=../../src/qt/res/icons/Scash.ico

convert ../../src/qt/res/icons/Scash-16.png ../../src/qt/res/icons/Scash-32.png ../../src/qt/res/icons/Scash-48.png ${ICON_DST}
