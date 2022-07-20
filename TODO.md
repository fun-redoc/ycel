In Work:
--------

TODO:
-----
- command line params, -csv-out-empty
- command line params, -pretty-header-line
- command line params, -csv-col-sep, -csv-line-sep
- command line params, -csv-quote-string
- more formulas (stddev, var, sample some distro, net value, kapital value,..)
- make up a better grammar to get rid of conflicts issued by yacc, maybe reread Aho/Ullman to relearn what to do
- enable UTF, use wide string for UTF

IDEAS:
-----
- make adding new formulas easieer and more generic. wite a generic formula handler, where you can register new formula handlers (calc, prettyprint, csv print, dump ..)...
- terminal online gui or some other gui
- write out resulting table in other formats (json, xml, ...)

READY:
------
- command line params, -csv -pretty (mutually exclusive)
- pretty printer, make string center aligned
- pretty printer, make number right aligned
- pretty printer, print as nice looking readable table
- fix empty cell printout bug
- write out resulting table as csv
- implement infix expressions
- Test usage of prefix + and - (in all contexts)