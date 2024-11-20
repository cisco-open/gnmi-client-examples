# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information
from cgitb import html
import yaml
import os

project = "mgbl_api"
copyright = "2024, Cisco"
author = "Theo"
release = "0.1.0"

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = ["breathe", "myst_parser", "sphinx.ext.graphviz"]
breathe_default_project = "mgbl_api"

templates_path = ["_templates"]
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]

source_suffix = {
    ".rst": "restructuredtext",
    ".md": "markdown",
}

# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = "sphinx_rtd_theme"
html_static_path = ["_static"]

# Add paths for custom static files (such as style sheets) here, relative to this directory.
html_css_files = [
    "css/custom.css",
]

# -- Build versions and languages menu ---------------------------------------
build_all_docs = os.environ.get("build_all_docs")
pages_root = os.environ.get("pages_root", "")

if build_all_docs is not None:
    current_language = os.environ.get("current_language")
    current_version = os.environ.get("current_version")

    html_context = {
        "current_language": current_language,
        "languages": [],
        "current_version": current_version,
        "versions": [],
    }

    if current_version == "latest":
        html_context["languages"].append(["en", pages_root])

    if current_language == "en":
        html_context["versions"].append(["latest", pages_root])

    with open("versions.yaml", "r") as yaml_file:
        docs = yaml.safe_load(yaml_file)

    if current_version != "latest":
        for language in docs[current_version].get("languages", []):
            html_context["languages"].append(
                [language, pages_root + "/" + current_version + "/" + language]
            )

    for version, details in docs.items():
        html_context["versions"].append(
            [version, pages_root + "/" + version + "/" + current_language]
        )
