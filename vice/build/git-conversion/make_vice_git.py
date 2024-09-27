#!/usr/bin/env python3

# PROTOTYPE SCRIPT for mirroring the SVN repo locally and converting it
# into a git repo.
#
# In ordinary usage, you'll want to select a location outside the VICE
# checkout tree and pass that as the first argument to this script:
#
# ./make_vice_git.py ../../vice-repo-mirror
#
# This will create vice-repo-mirror if necessary, get it up to date with
# all current branches and tags, and then create a Git version of that
# in ../../vice-git
#
# vice-git should be deleted before each run of this, but it's best to
# keep vice-repo-mirror around.
#
# email addresses are synthesized as SF.net addresses and may not be
# real at this point, but it's the best we can do with CVS and SVN.
#
# Breaking the cloned repository into properly organized branches and
# tags is in progress, and the current WIP is in tag_monster.py
#
# TODO: Any kind of safety or usability testing. Merge with pre-existing
#       email management. Further TODOs within tag_monster.

import os
import os.path
import subprocess
import sys


if len(sys.argv) < 2:
    print(f"Usage: {sys.argv[0]} <SVN-MIRROR-DIR>")
    sys.exit(1)

MIRROR_PATH = os.path.realpath(sys.argv[1])
if os.path.exists(MIRROR_PATH) and not os.path.isdir(MIRROR_PATH):
    print(f"{MIRROR_PATH} exists and is not a directory; aborting")
    sys.exit(1)

SVN_REMOTE_PATH = "svn://svn.code.sf.net/p/vice-emu/code"
SVN_REPO_PATH = MIRROR_PATH + "/svn-mirror"
GIT_TARGET_PATH = MIRROR_PATH + "/vice-git"
GIT_TECHDOCS_PATH = MIRROR_PATH + "/techdocs-git"
GIT_TESTPROGS_PATH = MIRROR_PATH + "/testprogs-git"

def check_git(target_path):
    if os.path.exists(target_path):
        if not os.path.isdir(target_path + "/.git"):
            print(f"Error: {target_path} already exists, and it's not a Git repo")
            sys.exit(1)

check_git(GIT_TARGET_PATH)
check_git(GIT_TECHDOCS_PATH)
check_git(GIT_TESTPROGS_PATH)

def find_program(prog):
    proc = subprocess.run(['which', prog], capture_output=True)
    if proc.returncode == 0:
        return proc.stdout.strip()
    return None

svn = find_program("svn")
svnadmin = find_program("svnadmin")
svnsync = find_program("svnsync")
gitsvn_ok = subprocess.run(['git', 'svn', '--version'], capture_output=True).returncode == 0

ok = True
if svn is not None:
    print(f"svn found at {svn.decode('utf-8')}")
else:
    print("svn NOT FOUND")
    ok = False
if svnadmin is not None:
    print(f"svnadmin found at {svnadmin.decode('utf-8')}")
else:
    print("svnadmin NOT FOUND")
    ok = False
if svnsync is not None:
    print(f"svnsync found at {svnsync.decode('utf-8')}")
else:
    print("svnsync NOT FOUND")
    ok = False
if gitsvn_ok:
    print("git-svn found")
else:
    print("git-svn NOT FOUND")
if not ok:
    print("Some prerequisites are not installed. Make sure SVN and git-svn\nare both installed and try again.")
    sys.exit(1)

repo_url = "file://" + SVN_REPO_PATH
if not os.path.isdir(SVN_REPO_PATH + "/hooks"):
    print(f"{SVN_REPO_PATH} not found, creating")
    subprocess.run([svnadmin, 'create', SVN_REPO_PATH], check=True)

    revprop_file = SVN_REPO_PATH + "/hooks/pre-revprop-change"
    if not os.path.isfile(revprop_file):
        print(f"Creating {revprop_file}")
        f = open(revprop_file, "w")
        f.write("#!/bin/sh\n\nexit 0\n")
        f.close()
        os.chmod(revprop_file, 0o755)

    print(f"Pointing {repo_url} at {SVN_REMOTE_PATH}")
    subprocess.run([svnsync, 'initialize', repo_url, SVN_REMOTE_PATH], check=True)

print(f"Syncing {repo_url} to {SVN_REMOTE_PATH}")
subprocess.run([svnsync, 'sync', repo_url], check=True)

print(f"Extracting author list")
changelog = subprocess.run([svn, 'log', '-q', repo_url], check=True, capture_output=True)
authors = {"(no author)": "(no author) <no_email>"}
for line in changelog.stdout.decode('utf-8').split('\n'):
    if '|' in line:
        author = line.split('|')[1].strip()
        if author not in authors:
            authors[author] = f"{author} <{author}@sf.net>"

print("Author transform list:")
AUTHORS_LIST = MIRROR_PATH + "/authors-transform.txt"
f = open(AUTHORS_LIST, "w")
for x in authors:
    print(f"{x} = {authors[x]}")
    f.write(f"{x} = {authors[x]}\n")
f.close()

def git_update(target_path, subrepo=None):
    if os.path.exists(target_path):
        if subrepo == None:
            branchname = "VICE"
            branch = "svn/trunk"
        else:
            branchname = subrepo[1:]
            branch = "svn/git-svn"
        print(f"Updating git repo \"{branchname}\"")
        os.chdir(target_path)
        subprocess.run(['git', 'svn', 'fetch'], check=True)
        subprocess.run(['git', 'merge', branch], check=True)
    else:
        args = ['git', 'svn', 'clone']
        if subrepo == None:
            branchname = "VICE"
            args += [repo_url, f'--rewrite-root={SVN_REMOTE_PATH}', '--prefix=svn/', '--stdlayout', '--no-follow-parent']
        else:
            branchname = subrepo[1:]
            args += [repo_url + subrepo, f'--rewrite-root={SVN_REMOTE_PATH}{subrepo}', '--prefix=svn/']

        print(f"Creating git edition of {branchname} at {target_path}")

        subprocess.run(args + ['--authors-file', AUTHORS_LIST, target_path], check=True)

git_update(GIT_TARGET_PATH)
git_update(GIT_TECHDOCS_PATH, '/techdocs')
git_update(GIT_TESTPROGS_PATH, '/testprogs')
