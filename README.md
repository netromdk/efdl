efdl
====

efdl, pronounced "Eff-dell", is an efficient downloading application
over HTTP/HTTPS.

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
  -o, --output <dir>    Where to save file. (defaults to current directory)
  -c, --conns <num>     Number of simultaneous connections to use. (defaults to
                        1)
  --chunks <num>        Number of chunks to split the downloadup into. Cannot
                        be used with --chunk-size.
  --chunk-size <bytes>  Size of each chunk which dictates how many to use.
                        Cannot be used with --chunks.
  --show-conn-progress  Shows progress information for each connection.

Arguments:
  URL                   URL to download.
```
