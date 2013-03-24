env = Environment(
    CPPFLAGS=['-g', '--std=c++0x'],
    LIBS=['boost_filesystem', 'mpg123', 'curl'],
    )

#env['STATIC_AND_SHARED_OBJECTS_ARE_THE_SAME']=1 
andreasmusic = env.SharedLibrary('andreasmusic', Glob('src/*.cpp'))

#env.InstallVersionedLib()

env.Alias('install', env.Install('/usr/local/include/andreasmusic', Glob('src/*.hpp')))
env.Alias('install', env.Install('/usr/local/lib', andreasmusic))
