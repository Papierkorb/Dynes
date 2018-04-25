TEMPLATE = subdirs
SUBDIRS = lib gui test

gui.depends = lib
test.depends = lib
