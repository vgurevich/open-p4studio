SAI
===

This folder contains the SAI implementation for Barefoot ASICs.

The SAI OSS header files can be found at <root>/submodules/SAI. We utilize some generated meta header files from SAI and build them into our SAI library.

Steps to upgrade SAI
====================

Update the above version string to the new version.
Update the refpoint in submodules/SAI to new version.
Build SAI meta
> **Note**
> Commands should be run in sde-tolkit or any other env that allow you to build the SDE

```
mkdir build && cd build
cmake .. -DSWITCH=on
make saimetadata
```
> **Note**
> Ensure clang-format is executed on these newly copied files.
For more details see [gensairpc.pl documentation](perl/README.md).



Install dependencies
====================

In case you have some missing dependencies
```
sudo -E apt-get install -y  doxygen \
    graphviz \
    aspell \
    libtemplate-perl \
    libconst-fast-perl \
    libmoosex-aliases-perl \
    libnamespace-autoclean-perl \
    libgetopt-long-descriptive-perl
```
