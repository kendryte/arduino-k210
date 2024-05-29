# Minimal makefile for Sphinx documentation
#
# You can set these variables from the command line, and also
# from the environment for the first two.
SPHINXOPTS    ?=
SPHINXBUILD   ?= sphinx-build
SPHINXMULTIVERSION ?= sphinx-multiversion
SOURCEDIR     = source
BUILDDIR      = build
WEB_DOCS_BUILDER_URL ?= https://ai.b-bug.org/~zhengshanshan/web-docs-builder

# Put it first so that "make" without argument is like "make help".
help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
	@echo 
	@echo "  \033[44mmhtml\033[0m       to make multiple version html"

.PHONY: help Makefile mhtml

# Catch-all target: route all unknown targets to Sphinx using the new
# "make mode" option.  $(O) is meant as a shortcut for $(SPHINXOPTS).
%:
	@$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

mhtml: 
	@$(SPHINXMULTIVERSION) "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
