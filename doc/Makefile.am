#
# Makefile.am for pkg-bindings/doc
#

SUBDIRS = autodocs

htmldir = $(docdir)/html

builtin = $(wildcard $(srcdir)/../src/*.cc)
builtinxml := $(builtin:%.cc=%.xml)
xmlfiles := $(foreach xml,$(builtinxml),$(notdir $(xml)))
htmlfiles = $(filter-out html/index.html, $(wildcard html/*.html))


html_DATA =			\
	html/yast2docs.css	\
	html/index.html		\
	$(htmlfiles)

html/index.html:  $(xmlfiles) builtins.xml
	XML_CATALOG_FILES=@XML_CATALOG@ \
	@XSLTPROC@ --xinclude \
	@STYLESHEET_HTML@ builtins.xml

$(xmlfiles) : %.xml: ../src/%.cc
	`pkg-config --variable=ybindir yast2-devtools`/ydoc2 -o $@.tmp $<
	@XSLTPROC@ @STYLESHEET_YDOC@ $@.tmp > $@
	`pkg-config --variable=ybindir yast2-devtools`/replace_entities $@
	rm $@.tmp

html/yast2docs.css: html/index.html
	cp @STYLESHEET_CSS@ html
	cp -a `pkg-config --variable=yast2dir yast2-devtools`/data/docbook/images html

EXTRA_DIST =  builtins.xml pkg-bindings-section.xml

CLEANFILES = $(xmlfiles)

clean-local:
	rm -rf html

install-data-local:
	mkdir -p $(DESTDIR)$(htmldir)
	cp -a $(srcdir)/html/images $(DESTDIR)$(htmldir)
