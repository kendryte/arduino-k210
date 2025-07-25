# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import sys, os

sys.path.append(os.path.abspath('exts'))

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

project = 'K210 Arduino'
copyright = '2025 嘉楠 | ' + '京ICP备2025124317号 | 京公网安备11010802045870号'
# author = 'Canaan'
# release = '0.1'

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
   'sphinx_copybutton',
    'myst_parser',
    'sphinx_multiversion',
    'sphinxcontrib.mermaid'
]
source_suffix = {
    '.rst': 'restructuredtext',
    '.md': 'markdown',
}
html_title = 'K210 Arduino'
templates_path = ['_templates']
exclude_patterns = []

language = 'zh_CN'

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

# myst_heading_anchors = 4
# suppress_warnings = []

html_copy_source = False
html_show_sourcelink = False

html_favicon = 'favicon.ico'

# html_show_sphinx = False

html_theme = "sphinx_book_theme"
html_static_path = ['_static']
# if want to add top nav for canann, enable this.
html_css_files = ['custom-theme.css', 'auto-nums.css']

html_js_files = [
    'transform.js'
]

default_dark_mode = True

smv_tag_whitelist = r'^\d+\.\d+$'

html_theme_options = {
    "repository_url": "https://github.com/kendryte/arduino-k210",
    'collapse_navigation': True,
    'use_repository_button': True,
    'navigation_depth': 7,
    "show_navbar_depth": 2,
    "primary_sidebar_end": ["versionsFlex.html"],
    "footer_start": ["Fleft.html"],
	"footer_center": ["Footer.html"],
	"footer_end" : ["Fright.html"],
    "navbar_start" : ['logo.html'],
    "navbar_center" : ['nav.html'],
    "navbar_end" : ['login.html'],
    "article_footer_items": ["content.html"]
}
if language == 'en':
    html_theme_options["footer_start"] = ["FleftEn.html"]
    html_theme_options["footer_center"] = ["FooterEn.html"]
    html_theme_options["footer_end"] = ["FrightEn.html"]

# Style
pygments_style = "sphinx"