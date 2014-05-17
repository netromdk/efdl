efdl
====

efdl, pronounced "Eff-dell", is an efficient downloading application
over HTTP/HTTPS.

Requirements
============

A C++11 compliant compiler (GCC 4.8+, Clang 3.3+ etc.), CMake 2.8.8+,
and Qt 5.2+.

Compilation
===========

To compile the source code and link the binaries do the following:

1. Extract source and go into the diretory.
2. mkdir build
3. cd build
4. cmake ..
5. make

This produces the *efdl* binary in the *bin* folder.

Program usage
====
```
Usage: efdl [options] URL
Efficient downloading application.

Options:
  -h, --help            Displays this help.
  -v, --version         Displays version information.
  --verbose             Verbose mode.
  --confirm             Confirm to download on redirections.
  -r, --resume          Resume download if file is present locally and the
                        server supports it.
  -o, --output <dir>    Where to save file. (defaults to current directory)
  -c, --conns <num>     Number of simultaneous connections to use. (defaults to
                        1)
  --chunks <num>        Number of chunks to split the downloadup into. Cannot
                        be used with --chunk-size.
  --chunk-size <bytes>  Size of each chunk which dictates how many to use.
                        Cannot be used with --chunks.
  --show-conn-progress  Shows progress information for each connection.
  --checksum <fmt>      Generate a checksum of the downloaded file using the
                        given hash function. Options are: md4, md5, sha1,
                        sha2-224, sha2-256, sha2-384, sha2-512, sha3-224,
                        sha3-256, sha3-384, sha3-512

Arguments:
  URL                   URL to download.
```
