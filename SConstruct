env = Environment(
    CPPFLAGS=['-g', '--std=c++0x'],
    LIBS=['boost_filesystem', 'mpg123', 'curl'],
    )

#env['STATIC_AND_SHARED_OBJECTS_ARE_THE_SAME']=1 
andreasmusic = env.SharedLibrary('andreasmusic', Glob('src/*.cpp'), SHLIBVERSION='0.1.0')

env.InstallVersionedLib()
