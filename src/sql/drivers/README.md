
Here "sql-drivers" means external non-Qt libraries, on
which QtSql will directly link to, instead of depending on "sql-plugins"
(if configured so by "configure" executable and/or shell-script, ).

If any of said external libraries is missing, QtSql would crash,
hence by default "sql-drivers" variable is empty, and
the "sql-plugins" variable is set instead at: "mkspecs/qmodule.pri".

See also [../../plugins/sqldrivers/README.md][1]

 [1]: ./../../plugins/sqldrivers/README.md
