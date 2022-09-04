#!/bin/bash
set -e

# Updates the install name for each of the Mac build's libraries to use @rpath.

cd "$BUILT_PRODUCTS_DIR/$FRAMEWORKS_FOLDER_PATH/"
chmod u+w *.dylib # required for xcodebuild
for libname in *.dylib
do
    [ -e "$libname" ] || continue
    install_name_tool -id "@rpath/$libname" $libname
    codesign -f -s - $libname
done
