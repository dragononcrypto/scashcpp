install_name_tool -change /usr/local/opt/openssl/lib/libssl.1.0.0.dylib @executable_path/../Frameworks/libssl.1.0.0.dylib ./scash-qt

install_name_tool -change /usr/local/opt/berkeley-db@4/lib/libdb_cxx-4.8.dylib @executable_path/../Frameworks/libdb_cxx-4.8.dylib ./scash-qt

install_name_tool -change /usr/local/opt/boost@1.60/lib/libboost_system-mt.dylib @executable_path/../Frameworks/libboost_system-mt.dylib ./scash-qt

install_name_tool -change /usr/local/opt/boost@1.60/lib/libboost_filesystem-mt.dylib @executable_path/../Frameworks/libboost_filesystem-mt.dylib ./scash-qt

install_name_tool -change /usr/local/opt/boost@1.60/lib/libboost_program_options-mt.dylib @executable_path/../Frameworks/libboost_program_options-mt.dylib ./scash-qt

install_name_tool -change /usr/local/opt/boost@1.60/lib/libboost_thread-mt.dylib @executable_path/../Frameworks/libboost_thread-mt.dylib ./scash-qt

install_name_tool -change /Users/admin/Qt5.4.2/5.4/clang_64/lib/QtWidgets.framework/Versions/5/QtWidgets @executable_path/../Frameworks/QtWidgets.framework/Versions/5/QtWidgets ./scash-qt

install_name_tool -change /Users/admin/Qt5.4.2/5.4/clang_64/lib/QtGui.framework/Versions/5/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/5/QtGui ./scash-qt

install_name_tool -change /Users/admin/Qt5.4.2/5.4/clang_64/lib/QtCore.framework/Versions/5/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/5/QtCore ./scash-qt


#	/usr/local/opt/openssl/lib/libcrypto.1.0.0.dylib (compatibility version 1.0.0, current version 1.0.0)
#	/usr/local/opt/berkeley-db@4/lib/libdb_cxx-4.8.dylib (compatibility version 0.0.0, current version 0.0.0)
#	/usr/local/opt/boost@1.60/lib/libboost_system-mt.dylib (compatibility version 0.0.0, current version 0.0.0)
#	/usr/local/opt/boost@1.60/lib/libboost_filesystem-mt.dylib (compatibility version 0.0.0, current version 0.0.0)
#	/usr/local/opt/boost@1.60/lib/libboost_program_options-mt.dylib (compatibility version 0.0.0, current version 0.0.0)
#	/usr/local/opt/boost@1.60/lib/libboost_thread-mt.dylib (compatibility version 0.0.0, current version 0.0.0)
#	/Users/admin/Qt5.4.2/5.4/clang_64/lib/QtWidgets.framework/Versions/5/QtWidgets (compatibility version 5.4.0, current version 5.4.2)
#	/Users/admin/Qt5.4.2/5.4/clang_64/lib/QtGui.framework/Versions/5/QtGui (compatibility version 5.4.0, current version 5.4.2)
#	/Users/admin/Qt5.4.2/5.4/clang_64/lib/QtCore.framework/Versions/5/QtCore (compatibility version 5.4.0, current version 5.4.2)
