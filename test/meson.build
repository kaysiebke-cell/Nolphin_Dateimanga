
test('Search Engine test',
  executable('test-nolphin-search-engine',
    [ 'test-nolphin-search-engine.c' ],
    include_directories: [ rootInclude, ],
    dependencies: [ gtk, nolphin_private ],
  ),
  args: []
)

test('Directory Async test',
  executable('test-nolphin-directory-async',
    [ 'test-nolphin-directory-async.c' ],
    include_directories: [ rootInclude, ],
    dependencies: [ gtk, nolphin_private ],
  ),
  args: []
)

test('Copy test',
  executable('test-nolphin-copy',
    [ 'test-copy.c', 'test.c' ],
    include_directories: [ rootInclude, ],
    dependencies: [ gtk, nolphin_private ],
    c_args: nolphin_definitions,
  ),
  args: []
)

test('Archive test',
  executable('test-nolphin-archive',
    [ 'test-nolphin-archive.c' ],
    include_directories: [ rootInclude, ],
    dependencies: [ gtk, nolphin_private ],
  ),
  args: []
)

test('CAD test',
  executable('test-nolphin-cad',
    [ 'test-nolphin-cad.c' ],
    include_directories: [ rootInclude, ],
    dependencies: [ gtk, nolphin_private ],
  ),
  args: []
)
