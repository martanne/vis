-- std = "min"
globals = { "vis" }
include_files = { "lua/*.lua", "lua/**/*.lua", "test/lua/*.lua" }
exclude_files = { "lua/lexer.lua", "lua/lexers/**", "test/lua/visrc.lua" }
files["test/lua"] = { std = "+busted" }
