REM read the README 
REM i used impdef to generate hid.def then edited to have the expected @8/@12s
REM you shouldn't need to run this script again

copy c:\windows\system32\hid.dll .\hid.dll
dlltool -k --export-all-symbols -D hid.dll -d hid.def -l libhid.a
del hid.dll
