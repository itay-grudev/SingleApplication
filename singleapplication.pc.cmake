prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=@CMAKE_INSTALL_PREFIX@
libdir=@LIB_INSTALL_DIR@
includedir=@INCLUDE_INSTALL_DIR@

Name: @SINGLEAPPLICATION_NAME@
Description: Library to detect and communicate with running instances of an application
Version: @SINGLEAPPLICATION_VERSION_PACKAGE@
Libs: -L${libdir} -l@SINGLEAPPLICATION_NAME@
Cflags: -I${includedir}
