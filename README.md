# Storage
Bachelor's graduate work

Project's aim is to create COM object with unified interface to storing data in different types of storages.
Supported storages and types of accesses: local file storage, FTP (via curl), Smb, MSSQL, PostgreSQL, SQLite3, MongoDB

You need additional libraries for building project: Boost (serialization for MongoDB and Scope Exit) for main project and GTest for testing.

In bin directory, 32-bit binaries are presented. In order to create 64-bit application, relevant libraries should be found.

### Component registration
Component should be registered manually
To register component use (don't forget admin rights): regsvr32 <path_to_build>/bin/<Debug/Release>/storage.dll
If you have 64-bit system and build x32 COM, you should use regsvr32 from %systemroot%/SysWoW64
To unregister this component:  regsvr32 -u <path_to_dll>/storage.dll

### Tests
Tests are not automated and should be manually tuned. Please look at example file [example_gtest.txt] for test args.
Tests create auxiliary files in your database. You can delete them after testing. You can test several storages per use.

###Benchmark
Benchmark uses storage component to produce measurements and log them.