#
# Makefile.am for core/wfm/src
#

# -Woverloaded-virtual catches mis-overridden callbacks
AM_CXXFLAGS = -DY2LOG=\"Pkg\"			\
	-DSUSEVERSION=\"${SUSEVERSION}\"	\
	-DLOCALEDIR=\"${localedir}\"		\
	-fno-inline				\
	-Woverloaded-virtual			\
	-DZYPP_BASE_LOGGER_LOGGROUP=\"Pkg\"


# to look for the packagemanager in the prefix first (really first?)
AM_LDFLAGS = -L${libdir}

plugin_LTLIBRARIES = libpy2Pkg.la

libpy2Pkg_la_SOURCES =				\
	Arch.cc					\
	PkgError.cc PkgError.h			\
	Y2PkgComponent.cc Y2PkgComponent.h	\
	Y2CCPkg.cc Y2CCPkg.h			\
	ycpTools.cc ycpTools.h			\
	PkgModule.cc PkgModule.h		\
	PkgProgress.cc PkgProgress.h		\
	PkgModuleFunctions.h			\
	PkgModuleFunctions.cc			\
	PkgFunctions.h PkgFunctions.cc		\
	Package.cc				\
	Patch.cc				\
	Resolvable_Install.cc			\
	Resolvable_Patches.cc			\
	Resolvable_Properties.cc		\
	Selection.cc				\
	Target.cc Target_DU.cc Target_Load.cc	\
	Locale.cc Locks.cc			\
	Source_Callbacks.cc			\
	Source_Create.cc			\
	Source_Download.cc			\
	Source_Get.cc				\
	Source_Installation.cc			\
	Source_Load.cc				\
	Source_Misc.cc				\
	Source_Resolvables.cc			\
	Source_Save.cc				\
	Source_Set.cc				\
	Keyring.cc GPGMap.cc GPGMap.h		\
	Callbacks.h				\
	Callbacks.YCP.h Callbacks.YCP.cc	\
	Callbacks.cc Callbacks_Register.cc	\
	Y2PkgFunction.cc Y2PkgFunction.h	\
	YRepo.h YRepo.cc			\
	PkgService.cc PkgService.h		\
	ServiceManager.cc ServiceManager.h	\
	Service.cc				\
	HelpTexts.h i18n.h log.h


libpy2Pkg_la_LDFLAGS = -version-info 2:0
libpy2Pkg_la_LIBADD = \
	-lycp		\
	-ly2		\
	-ly2util	\
	${ZYPP_LIBS}

INCLUDES = -I$(includedir) ${ZYPP_CFLAGS}

BUILT_SOURCES = PkgBuiltinTable.h PkgBuiltinCalls.h

# "foo bar: baz" is wrong
# see "info '(automake)Multiple Outputs'"
%BuiltinCalls.h %BuiltinTable.h : %Functions.h
	${ydatadir}/devtools/bin/generateYCPWrappers $< $*BuiltinCalls.h $*BuiltinTable.h

CLEANFILES = $(BUILT_SOURCES)


