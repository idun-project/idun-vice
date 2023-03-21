# Getting Started with Git SVN

- [Getting Started with Git SVN](#getting-started-with-git-svn)
- [Introduction](#introduction)
  - [Cloning the SVN repository](#cloning-the-svn-repository)
  - [Getting the latest changes from SVN](#getting-the-latest-changes-from-svn)
  - [Local work](#local-work)
  - [Pushing local changes to SVN](#pushing-local-changes-to-svn)
- [Caveats](#caveats)
      - [Local merges](#local-merges)
      - [Handling empty folders properly](#handling-empty-folders-properly)
      - [Cloning really big SVN repositories](#cloning-really-big-svn-repositories)
      - [About commits and SHA1](#about-commits-and-sha1)
- [Troubleshooting](#troubleshooting)
  - [MacOS & Perl - Can't locate SVN/Core.pm in @INC](#macos--perl---cant-locate-svncorepm-in-inc)
  - [Linux - git: 'svn' is not a git command](#linux---git-svn-is-not-a-git-command)
  - [git svn rebase command issues a checksum mismatch error](#git-svn-rebase-command-issues-a-checksum-mismatch-error)
  - [File was not found in commit](#file-was-not-found-in-commit)
- [References](#references)

# Introduction

`git svn` is a git command that allows using git to interact with Subversion repositories. `git svn` is part of git, meaning that is NOT a plugin but actually bundled with your git installation. 
SourceTree also happens to support this command so you can use it with your usual workflow.

**Important** 
You need to use the command `git svn` without the hyphen '-'.

**Disclaimer** 
This guide was written circa 2015, when working on a project where it was required to use SVN. As back in the day I was  more proficient and confortable using git workflows (specifically fast and multiple local branches), in order to replicate some of those workflow with SVN I started using this command.
Of course I faced some issues, so this document is just a dump of my knowledge and hopefully a helping guide in case you face the same problems as I did.

However, it is not an exhaustive guide, nor it is updated anymore, so please, if you find something wrong, please leave a messge.

I hope this guide may be helpful to you. Enjoy!

## Cloning the SVN repository

You need to create a new local copy of the repository with the command

`git svn clone SVN_REPO_ROOT_URL [DEST_FOLDER_PATH] -T TRUNK_REPO_PATH -t TAGS_REPO_PATH -b BRANCHES_REPO_PATH`

If your SVN repository follows the standard layout (trunk, branches, tags folders) you can save some typing:

`git svn clone -s SVN_REPO_ROOT_URL [DEST_FOLDER_PATH]`

`git svn clone` checks out each SVN revision, one by one, and makes a git commit in your local repository in order to recreate the history. If the SVN repository has a lot of commits this will take a while, so you may want to grab a coffee.

When the command is finished you will have a full fledged git repository with a local branch called master that trackes the trunk branch in the SVN repository.

If the SVN repository has a long history, the `git svn clone` operation can crash or hang (you'll notice the hang because the progress will stall, just kill the process with CTRL-C).
If this happens, do not worry: the git repository has been created, but there is some SVN history yet to be retrieved from the server. To resume the operation, just change to the git repository's folder and issue the command `git svn fetch`.

## Getting the latest changes from SVN
The equivalent to `git pull` is the command `git svn rebase`. 

This retrieves all the changes from the SVN repository and applies them *on top* of your local commits in your current branch. This works like, you know, a rebase between two branches :)

You can also use `git svn fetch` to retrieve the changes from the SVN repository but without applying them to your local branch.

## Local work
Just use your local git repository as a normal git repo, with the normal git commands

*  `git add FILE` and `git checkout -- FILE` To stage/unstage a file
*  `git commit` To save your changes. Those commits will be local and will not be "pushed" to the SVN repo
*  `git stash` and `git stash pop` Hell yeah! Stashes are back!
*  `git reset HEAD --hard` Revert all your local changes
*  `git log` Access all the history in the repository
*  `git rebase -i` Yep, you can squash all the commits! (as usual be SUPER CAREFULL with this one)
*  `git branch` Yes! you can create local branches! But remember to keep the *history linear*!

## Pushing local changes to SVN
`git svn dcommit --rmdir` will create a SVN commit for each of your local git commits. As with SVN, your local git history must be in sync with the latest changes in the SVN repository, so if the command fails, try performing a `git svn rebase` first.

**Side note**

Your local git commits will be *rewritten* when using the command `git svn dcommit`. This command will add a text to the git commit's message referencing the SVN revision created in the SVN server, which is VERY useful. However, adding a new text  requires modifying an existing commit's message which can't actually be done: git commits are inmutable. The solution is create a new commit with the same contents and the new message, but it is technically a new commit anyway (i.e. the git commit's SHA1 will change)

# Caveats 
"Subversion is a system that is far less sophisticated than Git" so you can't use all the full power of git without messing up the history in the Subversion server. Fortunately the rules are very simple:

**Keep the history linear**

That's it. 

This means you can make all kind of crazy local operations: branches, removing/reordering/squashing commits, move the history around, delete commits, etc anything *but merges*.

#### Local merges

**Do not** merge your local branches, if you need to reintegrate the history of local branches use `git rebase` instead.

When you perform a merge, a merge commit is created. The particular thing about merge commits is that they have two parents, and that makes the history non-linear. Non-linear history will confuse SVN in the case you "push" a merge commit to the repository.

However do not worry: **you won't break anything if you "push" a git merge commit to SVN**. 

If you do so, when the git merge commit is sent to the svn server it will contain all the changes of all commits for that merge, so you will lose the history of those commits, but not the changes in your code.

#### Handling empty folders properly
git does not recognice the concept of folders, it just works with files and their filepaths. This means git does not track empty folders. SVN, however, does. Using git svn means that, by default,  *any change you do involving empty folders with git will not be propagated to SVN*.  
Fortunately the `--rmdir` flag corrects this issue, and makes git remove an empty folder in SVN if you remove the last file inside of it. Unfortunatelly it does not removes existing empty folders, you need to do it manually

To avoid needing to issue the flag each time you do a dcommit, or to play it safe if you are using a git GUI tool (like SourceTree) you need to set this behaviour as default, just issue the command:

`git config --global svn.rmdir true`

This changes your .gitconfig file and adds these lines:
```
[svn]
rmdir = true
```

Be careful if you issue the command `git clean -d`. That will remove all untracked files including folders that should be kept empty for SVN. 
If you need to generate againg the empty folders tracked by SVN use the command `git svn mkdirs`. 

In practices this means that if you want to cleanup your workspace from untracked files and folders you should always use both commands:

`git clean -fd && git svn mkdirs`

#### Cloning really big SVN repositories
If you SVN repo history is really really big this operation could take hours, as git svn needs to rebuild the complete history of the SVN repo.
Fortunately you only need to clone the SVN  repo once; as with any other git repository you can just copy the repo folder to other collaborators. Copying the folder to multiple computers will be quicker that just cloning big SVN repos from scratch.

#### About commits and SHA1
As git commits created for git svn are local, the SHA1 ids for git commits is only work locally. This means that you can't use a SHA1 to reference a commit for another person because the same commit will have a diferent SHA1 in each machine.
You need to rely in svn revision number. The number is appended to the commit message when you push to the SVN server

*You can* use the SHA1 for local operations though (show/diff an specific commit, cherry-picks and resets, etc)

# Troubleshooting

## MacOS & Perl - Can't locate SVN/Core.pm in @INC
`git svn` command is implemented as a Perl script. In newer versions of MacOS, system installed Perl do not include the `SVN::Core` module, so you have to either:
  * Install the `SVN::Core` module manually at the system level with sudo. [Here you have instructions to install a Perl module in MacOS](https://bioinformaticsonline.com/blog/view/29479/how-to-install-perl-modules-on-mac-os-x-in-easy-steps)
  * Install a homebrew version of Perl (which includes that module) and modify the git-svn script (located at `/usr/local/Cellar/git/VERSION/libexec/git-core/git-svn`) so the scripts uses this version of Perl instead of the included with the OS. [Here are instructions on how to do it](https://github.com/Homebrew/homebrew-core/issues/52490#issuecomment-658239604) 

Also, remember that Subversion must also be installed on the system.

## Linux - git: 'svn' is not a git command

[@graue70 pointed out](https://gist.github.com/rickyah/7bc2de953ce42ba07116#gistcomment-3721860) that  on linux you need to install the git-svn command with `sudo apt install git-svn`
The command git svn rebase throws an error similar to this:
```
  Checksum mismatch: <path_to_file> <some_kind_of_sha1>
  expected: <checksum_number_1>
    got: <checksum_number_2>
```

## git svn rebase command issues a checksum mismatch error
The solution to this problem is reset svn to the revision when the troubled file got modified for the last time, and do a git svn fetch so the SVN history is restored. The commands to perform the SVN reset are:

 * git log -1 -- `<path_to_file>` (copy the SVN revision number that appear in the commit message)
 * git svn reset `<revision_number>`
 * git svn fetch
 
You should be able to push/pull data from SVN again


## File was not found in commit 
When you try to fetch or pull from SVN you get an error similar to this
```
<file_path> was not found in commit <hash>
```
This means that a revision in SVN is trying to modify a file that for some reason doesn't exists in your local copy.  The best way to get rid of this error is force a fetch ignoring the path of that file and it will updated to its status in the latest SVN revision:

 * `git svn fetch --ignore-paths <regex>`

(thanks [@rykus0](https://gist.github.com/rickyah/7bc2de953ce42ba07116#gistcomment-2140387) for pointing out that --ignore-paths actually accepts a regex, not a file-path)


# References
- http://git-scm.com/book/en/v1/Git-and-Other-Systems-Git-and-Subversion
- https://git-scm.com/docs/git-svn
