qgrit (Qt Git rebase --interactive tool)
======================================

This helper allows you to graphically reorder commits during "git rebase -i"
It avoids the potential problem when editing text files.
Typos during edit, lines lost by cutting and forgetting to paste.
And other shortcomings depending on the editor used.

Features
========

* Commits can be moved/reordered by Drag & Drop or by up/down buttons.
* Multiple commits can be moved up/down in a single step
* Actions for modifying commits can be selected from a combo box.
* Easy aborting of the rebase(no need to delete all lines in a text file)
* Easy way of undoing all changes made to the list and start over again
* Commits are sorted in the same order as in gitk, no more confusion about order
* Tool tips explaining different actions (pick, edit, reword, squash, fixup)
* Tool tips contain full commit message, not only subject line


Integration into git
====================

* git v1.7.8 and above:
easy install/uninstall:
Start qgrit
Press the "Install qgrit" or "Uninstall qgrit" button.

manual install:
git config --global sequence.editor "/path/to/qgrit --rebasei"

manual uninstall:
git config --global --unset sequence.editor

If you want to use qgrit only in a certain repository
cd into the repository before invoking git config without "--global"

* git v1.7.* and below:
Older versions of git have no configuration for only invoking qgrit
as editor for interactive rebase list but not for editing commit messages.

start interactive rebase by specifying GIT_EDITOR environment var
GIT_EDITOR="/path/to/qgrit --rebasei" git rebase -i <commit>

You will not be able to use the "reword" action as git tries
to invoke qgrit as commit message editor.

LICENSE
=======

qgrit is released under the BSD 2-Clause License
