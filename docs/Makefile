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
%: Makefile $(SOURCEDIR)/_templates/versions.html $(SOURCEDIR)/_static/topbar.css $(SOURCEDIR)/_static/custom.css $(SOURCEDIR)/_static/init_mermaid.js
	@$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

mhtml: Makefile $(SOURCEDIR)/_templates/versions.html $(SOURCEDIR)/_static/topbar.css $(SOURCEDIR)/_static/custom.css $(SOURCEDIR)/_static/init_mermaid.js
	@$(SPHINXMULTIVERSION) "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

_templates:
	mkdir -p $(SOURCEDIR)/_templates

$(SOURCEDIR)/_templates/versions.html: 
	wget $(WEB_DOCS_BUILDER_URL)/_templates/$(@F) -O $@

$(SOURCEDIR)/_static/topbar.css:
	wget $(WEB_DOCS_BUILDER_URL)/_static/$(@F) -O $@

$(SOURCEDIR)/_static/custom.css:
	wget $(WEB_DOCS_BUILDER_URL)/_static/$(@F) -O $@

$(SOURCEDIR)/_static/init_mermaid.js:
	wget $(WEB_DOCS_BUILDER_URL)/_static/$(@F) -O $@
