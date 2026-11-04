
This directory is used in package.path, package has not been overloaded in any way, so the global package namespace is as you see here
Therefore any default external lua packages reside here, only vis and visrc are built by vis, and should be the only thing modified
this is because external modules can be updated at any moment.

Given this, you can overload vis default configuration itself with your own ~/.config/vis/ directory

scintillua uses lexer.lua and expects to reside completely in the global package namespace, so vis overwrites scintilluas load() with its own to work

visrc resides on the users $HOME/.config/vis and must require'vis' to work correctly


Tips:
	Use luacheck to pick up errors
	Use locals so luacheck can pick up typos
