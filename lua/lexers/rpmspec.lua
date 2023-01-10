--- lexers/rpmspec.lua
+++ lexers/rpmspec.lua
@@ -28,4 +28,6 @@ lex:add_rule('keyword', token(lexer.KEYWORD, word_match{
 -- Macros
 lex:add_rule('command', token(lexer.FUNCTION, '%' * lexer.word))
 
+lexer.property['scintillua.comment'] = '#'
+
 return lex
