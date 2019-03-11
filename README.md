# AcDcCompiler
a simple compiler in Crafting a Compiler with C

## context free grammar(CFG)
previous version:
- expr-> value expr | expr + exprtail | expr - exprtail
![Image](doc/images/former_expr_tree.jpg?raw=true)
current version:
- expr -> term + expr | term - expr | term
- term -> factor * term | factor / term | factor
- factor -> digit | identifier
![Image](doc/images/expr_tree.jpg?raw=true)


