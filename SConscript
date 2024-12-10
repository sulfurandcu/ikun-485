from building import *

cwd = GetCurrentDir()
inc = [cwd]
src  = Glob('*.c')

group = DefineGroup('ik485', src, depend = ['PKG_USING_IK485'], CPPPATH = inc)

Return('group')
