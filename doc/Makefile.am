# Makefile.am for pkg-bindings/doc

SUBDIRS =

htmldir = $(docdir)/pkg

html_DATA = YCP-builtins-pkg.html packager.html

CLEANFILES = YCP-builtins-pkg.html

builtin_pkg = $(srcdir)/../src/PkgModule*.cc

YCP-builtins-pkg.html: $(builtin_pkg)
	`pkg-config --variable=ybindir yast2-devtools`/makebuiltindocs -t "Pkg:: built-in functions" $^ > $@.new
	mv $@.new $@

EXTRA_DIST = $(html_DATA)
