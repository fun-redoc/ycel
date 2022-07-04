Compile and Run
---------------

`` 

    # lexical analysis
    lex ycel.l
    # compile everything 
    cc ycel.c lex.yy.c string_buffer_view.c -o ycel -ggdb -arch x86_64
    # execute a test
    ./ycel test.csv

``

Adding new Formulas
-------------------
1. in ``ycel.l`` like ``<fml>sum``
2. new token in ``%tokens``of ``ycel.y``
3. insert new rule in ``ycel.c:stmt``. Currently the best was to copy the rule for ``SUM``.
4. new node type in ``ycel.h``enum ``ENodeType``like ``TypeSum``
5. new case in ``ycel.c:dump_node`` in the big ``switch(nd->type)`` 
6. new case in ``ycel.c:calc_node``in the big switch. here is the reals implementation of the new formula. Currently best was to copy paste ``TypeSum`` implemtentation.