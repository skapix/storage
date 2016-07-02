# Storage
Bachelor's graduate work

Project's aim is to create library with c-compatible and unified interface to storing data in different types of storages.
Supported storages and types of accesses: local file storage, FTP (via curl), Smb, MSSQL, PostgreSQL, SQLite3, MongoDB

You need additional libraries for building project: Boost(all projects) and GTest(testing project).

In bin directory, 32-bit binaries are presented. In order to create 64-bit application, relevant libraries should be found.


### Tests
Tests are not automated and parameters should be set up. Please look at example file [example_gtest.txt] for test args.
Tests create auxiliary files in your database. You can delete them after testing. You can test several storages per use.

###Benchmark
Benchmark uses storage component to produce time measurement and log it.


Project's documentation and refactoring is still in process.