FceuX-SDL Style Guidelines
=========================
Background
----------
While you may think having style guidelines specific to a directory in a sourcetree is outrageous, fceuX has a unique elaborate history of the code.  I (prg318) have ressurected the majority of the SDL port code and tried to keep some aspects of the style consistant (although not many aspects were that consistent to begin with due to the amount of different people who have contributed to this codebase).  The SDL port alone consists of over 8,000 lines of C++ code, much of which is compliant to these guidelines so please use this guidelines when going forth and writing code in src/drivers/sdl. *Following these simple rules will make it a lot easier for the developers to accept your patch.*

Tabs
----
Hard tabs only!  Please!  src/drivers/sdl/* entirely consists of hard tabbed code.  Please continue to use this convention.  Consult the documentation of your text editor documentation for instructions on how to this with your editor.

Return Statements
-----------------
Please use the "return 0;" style return statements instead of the "return(0);" style return statements.

