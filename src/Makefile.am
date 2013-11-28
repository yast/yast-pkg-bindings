#
# Makefile.am for core/wfm/src
#

# -Woverloaded-virtual catches mis-overridden callbacks
AM_CXXFLAGS = -DY2LOG=\"wfm\"			\
	-DSUSEVERSION=\"${SUSEVERSION}\"	\
	-DLOCALEDIR=\"${localedir}\"		\
	-fno-inline				\
	-Woverloaded-virtual			\
	-DZYPP_BASE_LOGGER_LOGGROUP=\"pkgbindings\"


# to look for the packagemanager in the prefix first (really first?)
AM_LDFLAGS = -L${libdir}

plugin_LTLIBRARIES = libpy2Pkg.la

libpy2Pkg_la_SOURCES =				\
	HelpTexts.h				\
	i18n.h 					\
	log.h					\
	GPGMap.h GPGMap.cc			\
	PkgError.cc PkgError.h			\
	Y2PkgComponent.cc Y2PkgComponent.h	\
	Y2CCPkg.cc Y2CCPkg.h			\
	ycpTools.cc ycpTools.h			\
	PkgModule.cc PkgModule.h		\
	YRepo.h YRepo.cc                        \
	PkgProgress.cc PkgProgress.h		\
	Utils.cc			\
	ServiceManager.cc ServiceManager.h	\
	PkgService.cc PkgService.h		\
	Network.cc 				\
	PkgModuleFunctions.h			\
	PkgModuleFunctions.cc			\
	Package.cc				\
	Patch.cc				\
	Resolvable.cc				\
	Selection.cc				\
	Target.cc				\
        Source_Callbacks.cc                     \
        Source_Create.cc                        \
        Source_Download.cc                      \
        Source_Get.cc                           \
        Source_Installation.cc                  \
        Source_Load.cc                          \
        Source_Misc.cc                          \
        Source_Resolvables.cc                   \
        Source_Save.cc                          \
        Source_Set.cc                           \
	Keyring.cc				\
	Callbacks.h				\
	Callbacks.YCP.h				\
	Callbacks.cc


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
%BuiltinCalls.h %BuiltinTable.h : %ModuleFunctions.h
	${ydatadir}/devtools/bin/generateYCPWrappers $< $*BuiltinCalls.h $*BuiltinTable.h

CLEANFILES = $(BUILT_SOURCES)


