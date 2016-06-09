# Storage
Bachelor's graduate work

Project's aim is to create COM object with unified interface to storing data in different types of storages.
Supported storages and types of accesses: local file storage, FTP (via curl), Samba, MSSQL, PostgreSQL, SQLite3, MongoDB

You need additional libraries for building project: Boost (serialization for MongoDB and Scope Exit). You also need GTest for compiling test project.

In bin directory, 32-bit binaries are presented. In order to create 64-bit application, relevant libraries should be found.

### Component registration
Component should be registered manually
To register component use (don't forget admin rights): regsvr32 <path_to_build>/bin/<Debug/Release>/storage.dll
If you have 64-bit system and build x32 COM, you should use regsvr32 from %systemroot%/SysWoW64
To unregister this component:  regsvr32 -u <path_to_dll>/storage.dll

### Tests
Tests should be manually tuned. That is why they are not automated.