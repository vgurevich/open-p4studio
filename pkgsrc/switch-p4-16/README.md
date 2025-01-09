BF Switch
=========


      +---------------------+---------+  
      |  Thrift  |  CLI     |   SAI   |  
      |          |          |         |  
      +---------------------+---------+  
      |             API               |  
      |                               |  
    +-----------------------+-----------+  
    |          S3           |   Schema  |  
    |  Simple state service |           |  
    +-----------------------------------+         
    |           bf-rt.json              |  
    |          (BF-runtime)             |  
    +-----------------------------------+         
    |     Harlyn / Tofino / Tofino2     |         
    |     (compiled from switch.p4)     |         
    +-----------------------------------+         
                                                

Directory Structure  
===================  
Top level directory structure for bf-switch repo is as follows:  
  
```  
├── api                : This contains the API code for P4-16 stack  
│   ├── common         : Common code for bf-switch api. Primarily CRUD API or  
|   |                    any init, logging code etc  
│   ├── switch_tna     : This is Tofino specific implementation. All PD API  
|   |                    invocation logic is part of this directory.  
│   ├── test           : This is test for bfrt api  
|   |
│   └── templates      : Any auto-generated code templates are part of this  
|                        diretory. Examples of auto-generated code is Python  
|                        Thrif API calls and autogeneration of CRUD APIs for  
|                        user objects for PTF Scripts  
├── docs               : API implementation notes   
├── doxygen            : Describes the documentation system for a project                 
|                       
├── include            : Common include files for bf-switch  
│   ├── bf_switch      : Public header files  
│   ├── s3             : Framework specific header files  
│   └── sal            : Framework for bf_sal_switch  
|
├── p4src              : P4 source root directory  
│   ├── shared         : Shared code between different versions of Tofino and  
|   |                    and P4 Programs  
│   ├── switch-tofino  : Directory for Switch.P4 for Tofino  
│   └── switch-tofino2 : Directory for Switch.P4 for Tofino2  
|
├── ptf                : PTF Tests Scripts  
│   ├── api            : PTF Tests Scripts for testing the stack using BMAI  
|   |                    CRUD API  
│   ├── common         : Any common routines that can be used across BMAI and  
|   |                    SAI PTF Scripts  
│   ├── sal            : SAL PTF Scripts  
│   ├── sai            : SAI PTF Scripts  
│   └── saiv2          : SAI PTF Scripts  
|
├── s3                 : Core logic Schema Store  
│   ├── parser         : JSON Parser code for schema and creating an object  
|   |                    store  
│   ├── test           : test code for S3  
│   ├── third_party    : third party code S3. Right now only fmtlib  
│   └── tools          : Any S3 related scripts/automation scripts. Example  
|       |                CLI generation. Documentation generation etc.  
|       ├── aug_model_  
|       |   json_gen   : JSON schema sanitizer  
|       ├── enum_gen   : Generate model.h from JSON file  
|       ├── doc_gen    : Generate file for switch object model  
|       └── graphgen   : Generate a dot graph from JSON file  
|  
├── sai                : SAI Implementation for P4-16 stack  
|   ├── generated      : SAI thrift apis  
|   ├── perl           : SAI RPC interface generator  
|   |   ├── SAI        : SAI data collected from XML  
|   |   ├── Utils      : SAI submodule classes  
│   |   └── doc        : Documentation for SAI api  
|   ├── templates      : Generate model.h from JSON file  
│   └── maps           : JSON Maps that define SAI to BMAI Attribute mapping  
|                        for autogeneration of SAI code  
├── sal                : SAL Implementation for Intel BXD ASICs.  
├── schema             : Schema for Switch stack. Key for S3  
│   ├── appObj         : User defined objects for switch features  
│   └── asicObj        : ASIC Objects  
├── scripts            : Scripts that can be used on the repo. Right now  
|                        only Lint for CPP Code  
├── submodules         : Standard SAI Submodule  
│   └── SAI  
├── targets            : Various confiugration file required for SONiC  
│   ├── tofino  
│   ├── tofino2  
├── tools              : Any patches that need to be applied in SONiC  
│   └── sonic  
└── version            : Stack Version  
```  
