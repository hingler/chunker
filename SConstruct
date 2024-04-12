Import('env')

# can we put a flag here or something, to prevent x2 includes???

env.Append(CPPPATH=[
  env.Dir("include")
])

# possible double include??
sources = [
  "src/lod/lod_node.cpp",
  "src/lod/LodTreeGenerator.cpp",
  "src/thread/impl/ThreadHandleImpl.cpp",
  "src/thread/impl/ThreadQueueImpl.cpp"
]

library = env.Library("build/chunker", source=sources)

Default(library)
Return("library")

# don't need to do anything else - header only!
