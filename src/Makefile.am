#
# Makefile.am for core/wfm/src
#

AM_CXXFLAGS = -DY2LOG=\"wfm\" -DSUSEVERSION=\"${SUSEVERSION}\"	\
	-DLOCALEDIR=\"${localedir}\"  -fno-inline

# to look for the packagemanager in the prefix first (really first?)
AM_LDFLAGS = -L${libdir}

plugin_LTLIBRARIES = libpy2Pkg.la

libpy2Pkg_la_SOURCES =				\
	Y2PkgComponent.cc Y2PkgComponent.h	\
	Y2CCPkg.cc Y2CCPkg.h			\
	ycpTools.cc ycpTools.h			\
	PkgModule.cc PkgModule.h		\
	PkgModuleFunctions.h			\
	PkgModuleFunctions.cc			\
	PkgModuleFunctionsPackage.cc		\
	PkgModuleFunctionsPatch.cc		\
	PkgModuleFunctionsSelection.cc		\
	PkgModuleFunctionsTarget.cc		\
	PkgModuleFunctionsSource.cc		\
	PkgModuleCallbacks.h			\
	PkgModuleCallbacks.YCP.h		\
	PkgModuleCallbacks.cc


libpy2Pkg_la_LDFLAGS = -version-info 2:0
libpy2Pkg_la_LIBADD = \
	-lycp		\
	-ly2		\
	-ly2util	\
	-lzypp

INCLUDES = -I$(includedir)

BUILT_SOURCES = PkgBuiltinTable.h PkgBuiltinCalls.h

# "foo bar: baz" is wrong
# see "info '(automake)Multiple Outputs'"
%BuiltinCalls.h %BuiltinTable.h : %ModuleFunctions.h
	${ydatadir}/devtools/bin/generateYCPWrappers $< $*BuiltinCalls.h $*BuiltinTable.h

CLEANFILES = $(BUILT_SOURCES)


