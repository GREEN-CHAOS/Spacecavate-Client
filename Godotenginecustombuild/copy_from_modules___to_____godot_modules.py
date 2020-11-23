from distutils.dir_util import copy_tree

fromdir = "modules/OrbitCalculator"
todir = "godot/modules/OrbitCalculator"

copy_tree(fromdir,todir)