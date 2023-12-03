
The folder containing this file is for "sql-plugins", and
not for "sql-drivers", is just confiusingly named by original Qt.

Here "sql-plugins" means having smaller libraries which link to
external dependencies, instead QtSql linking to them directly.

The "sql-plugins" variable may be set at: "mkspecs/qmodule.pri".

See also [../../sql/drivers/README.md][1].

 [1]: ./../../sql/drivers/README.md


## License

By default only some of the GPL compatible plugins are enabled.

Please note that the DB2, Oracle and TDS client drivers are not distributed
with the Qt Open Source Editions.

This is because the client libraries are distributed under a license which
is not compatible with the GPL license.
