Compile and Run
---------------

look ``build``script.

```console 
    # lexical analysis
    lex ycel.l
    # compile everything 
    cc ycel.c lex.yy.c string_buffer_view.c -o ycel -ggdb -arch x86_64
    # execute a test write csv to file
    ./ycel -csv < test4.csv > out.csv
    # execute a test pretty print to stdout
    ./ycel -pretty < test4.csv
```

Adding new Formulas
-------------------

1. in ``ycel.l`` like ``<fml>sum``
2. new token in ``%tokens``of ``ycel.y``
3. insert new rule in ``ycel.y:stmt``. Currently the best was to copy the rule for ``SUM``.
4. new node type in ``ycel_parser.h``enum ``ENodeType``like ``TypeSum``
5. new case in ``ycel_parser.c:dump_node`` in the big ``switch(nd->type)`` 
6. new case in ``ycel_table.c:calc_node`` and ``ycel_table.c:tree_to_table`` in the big switch. here is the reals implementation of the new formula. Currently best was to copy paste ``TypeSum`` implemtentation.

I think there may be a better design to manage adding additional formulas than those big switches e.g.


Credits and References:
-----------------------

+ tsoding ["Mini Excel in C"](https://www.youtube.com/watch?v=HCAgvKQDJng) on youtube.
+ [Compiler Theory: Lexical Analysis by Marc Moreno Maza](https://www.csd.uwo.ca/~mmorenom/CS447/Lectures/Lexical.html/Lexical.html)
+ [How to Build a C Compiler Using Lex and Yacc by Anjaneya Tripathi](https://medium.com/codex/building-a-c-compiler-using-lex-and-yacc-446262056aaa)
+ THE BOOK: Compiler Design by Alfred V Aho, Ravi Sethi, Jeffrey D Ullman

Addendum:
---------

You should definitivelly use lex/yacc or similar tools like antlr for Java when it comes to parsing and interpreting data.