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

