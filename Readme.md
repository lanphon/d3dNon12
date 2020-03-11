# Introduction

Here is a d3d11on12's repo, to build the two reposities opened by microsoft, named
D3D11On12, and D3D12TranslationLayer. Another d3d9on12 may be opened in future, and
it shall also be collected, as a submodule here.

Compiling d3d11on12 has lots of tricks, so I'd like to provide a one-solution cmake
file, to set the path correctly on a bare system which has the insider preview SDK/WDK
installed already. The CMake system has lots reference to the official one, but provide
more details.
