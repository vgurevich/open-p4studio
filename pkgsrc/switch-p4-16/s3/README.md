SMI Framework
=============

                  +-------------------------+                   
                  |      switch_store       |              
                  +-------------------------+         
                              |
                              |
    +-----------------------------------------------+         
    |   factory   |   store   |   log   |   event   |
    +-----------------------------------------------+  
                              |       
                              |       
                  +-------------------------+         
                  |     bf_rt_backend       |         
                  +-------------------------+         
                                                
Modules
-------
    switch_store:
        The work horse of the whole framework. Implements the 4 primary APIs
        create/delete/set/get. Pushes the objects to the store. Integrates
        with factory, deps, etc
    store:
        Data storage backend. It's a simple key-value pair currently. Can be
        swapped out or extended to a persistent data store.
    factory:
        Implements a factory pattern. Table classes register with this factory
        along with a type so that they can be created at run-time.
    bf_rt_backend:
        A wrapper for Bf-Rt APIs. Implements helper functions for
          - Match action direct/indirect tables
          - Action profile/selector tables
          - Selector group tables
    log:
        Logging framework. Internally uses the open source "fmt" framework
    event:
        Notification module for various events like port status, learn, etc
