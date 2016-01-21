flac-stream
===========

node.js bindings to libflac

This is burdened by race conditions that are hard to debug (since it's difficult to instrument node C++ addons) and I decided this wasn't really a good use of my time anyway since I could very easily [write a flac player in a different language](https://github.com/cosmicexplorer/player).
