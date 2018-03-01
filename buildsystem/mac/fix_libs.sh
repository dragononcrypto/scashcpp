install_name_tool -change /usr/local/opt/openssl/lib/libssl.1.0.0.dylib @rpath/libssl.1.0.0.dylib ./scash-qt

install_name_tool -change /usr/local/opt/openssl/lib/libcrypto.1.0.0.dylib @rpath/libcrypto.1.0.0.dylib ./scash-qt

install_name_tool -change /usr/local/opt/berkeley-db@4/lib/libdb_cxx-4.8.dylib @rpath/libdb_cxx-4.8.dylib ./scash-qt

install_name_tool -change /usr/local/opt/boost@1.60/lib/libboost_system-mt.dylib @rpath/libboost_system-mt.dylib ./scash-qt

install_name_tool -change /usr/local/opt/boost@1.60/lib/libboost_filesystem-mt.dylib @rpath/libboost_filesystem-mt.dylib ./scash-qt

install_name_tool -change /usr/local/opt/boost@1.60/lib/libboost_program_options-mt.dylib @rpath/libboost_program_options-mt.dylib ./scash-qt

install_name_tool -change /usr/local/opt/boost@1.60/lib/libboost_thread-mt.dylib @rpath/libboost_thread-mt.dylib ./scash-qt

install_name_tool -change /usr/local/Cellar/openssl/1.0.2n/lib/libcrypto.1.0.0.dylib @rpath/libcrypto.1.0.0.dylib ./libssl.1.0.0.dylib
