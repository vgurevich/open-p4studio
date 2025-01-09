project(bfutils_examples LANGUAGES C CXX)

add_executable(dynhash_xor
  "dynhash_xor.cpp"
)
target_link_libraries(dynhash_xor
  dynhashStatic
)
