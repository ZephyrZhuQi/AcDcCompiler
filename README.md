# AcDcCompiler
a simple compiler in Crafting a Compiler with C

context free grammar(CFG)
previous version:
expr-> value expr | expr + exprtail | expr - exprtail
current version:
expr -> term + expr | term - expr | term
term -> factor * term | factor / term | factor
factor -> digit | identifier

