# Description: This script is used to build the documentation for
# all the versions and languages mentioned in the versions.yml file.

import subprocess
import yaml
import os


def build_doc(version, language, tag):
    os.environ["current_version"] = version
    os.environ["current_language"] = language
    subprocess.run("git checkout " + tag, shell=True)
    subprocess.run("git checkout main -- conf.py", shell=True)
    subprocess.run("git checkout main -- versions.yml", shell=True)
    subprocess.run("doxygen Doxyfile", shell=True)
    os.environ["SPHINXOPTS"] = "-D language='{}'".format(language)
    subprocess.run("make html", shell=True)


def move_dir(src, dst):
    subprocess.run(["mkdir", "-p", dst])
    subprocess.run("mv " + src + "* " + dst, shell=True)


os.environ["build_all_docs"] = str(True)
os.environ["pages_root"] = "github_pages_url"

build_doc("latest", "en", "main")
move_dir("./_build/html/", "../pages/")

with open("versions.yml", "r") as yaml_file:
    docs = yaml.safe_load(yaml_file)

for version, details in docs.items():
    tag = details.get("tag", "")
    for language in details.get("languages", []):
        build_doc(version, language, version)
        move_dir("./_build/html/", "../pages/" + version + "/" + language + "/")
