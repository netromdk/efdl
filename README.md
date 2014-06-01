efdl
====

efdl, pronounced "Eff-dell", is an efficient downloading command-line
application that currently supports transfers using HTTP(S).

It is also possible to use the functionality of *efdl* in your own
program. Just link the *libefdlcore* static library and use the
headers in the *include* directory.

Requirements
============

A C++11 compliant compiler (GCC 4.8+, Clang 3.3+ etc.), CMake 2.8.12+,
and Qt 5.2+.

Compilation and installation
===========

To compile the source code and link the binaries do the following:

1. Extract source and go into the diretory.
2. `mkdir build`
3. `cd build`
4. `cmake ..` (If you want the shared library then use `-DLIBRARY_TYPE=SHARED`)
5. `make`

This produces the *efdl* binary in the *bin* directory.

Notice the install prefix when running cmake before, which defaults to
*/usr/local*. If you run `sudo make install` it will install into the
*bin* directory there. Uninstalling is accomplished with `sudo make
uninstall`.

Program usage
====
```
Usage: ./bin/efdl [options] URLs..
Efficient downloading application.

If URLs are given through STDIN then the positional argument(s) are optional.
Also note that piping URLs will make confirmations default to 'no', if any.

HTTP basic authorization is supported either via --http-user and --http-pass
or by using URLs on the form 'scheme://username:password@host.tld/path/'.

Options:
  -h, --help            Displays this help.
  -v, --version         Displays version information.
  -o, --output <dir>    Where to save file. (defaults to current directory)
  -c, --conns <num>     Number of simultaneous connections to use. (defaults to
                        1)
  -r, --resume          Resume download if file is present locally and the
                        server supports it.
  --confirm             Will ask to confirm to download on redirections or
                        whether to truncate a completed file when resuming.
  --verbose             Verbose mode.
  --dry-run             Do not download anything just resolve URLs and stop.
  --chunks <num>        Number of chunks to split the downloadup into. Cannot
                        be used with --chunk-size.
  --chunk-size <bytes>  Size of each chunk which dictates how many to use.
                        Cannot be used with --chunks.
  --http-user <user>    Username for HTTP basic authorization.
  --http-pass <pass>    Password for HTTP basic authorization.
  --show-conn-progress  Shows progress information for each connection.
  --show-http-headers   Shows all HTTP headers. Implies --verbose.
  --checksum <fmt>      Generate a checksum of the downloaded file using the
                        given hash function. Options are: md4, md5, sha1,
                        sha2-224, sha2-256, sha2-384, sha2-512, sha3-224,
                        sha3-256, sha3-384, sha3-512

Arguments:
  URLs                  URLs to download.
```
