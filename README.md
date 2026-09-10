Filtering DNS resolver (dns)
===========================

Author: Timur Nurtdinov
Login:  xnurtd00
Date:   17.11.2025


1. Description
--------------
DNS (Domain Name System) is the system that translates human-readable domain
names into IP addresses used by computers to communicate with each other.

The program `dns` implements a simple filtering DNS resolver. It listens for
incoming DNS queries over UDP and handles them as follows:

  * The query is parsed and validated (a well-formed 12-byte header with
    exactly one question is required).
  * Queries of a type other than A are answered with rcode NOTIMP (Not
    Implemented).
  * If the queried name matches a domain from the filter file, or is a
    subdomain of such a domain, the query is answered with rcode REFUSED.
    Matching is case-insensitive and respects label boundaries, so
    `example.com` in the filter blocks `example.com` and `a.b.example.com`
    but not `notexample.com`.
  * Every other query is forwarded over UDP to the upstream DNS resolver
    given on the command line, and its answer is relayed back to the client
    unchanged.
  * If the upstream resolver does not answer within the timeout, the client
    receives rcode SERVFAIL.

The server runs until it is stopped with SIGINT (Ctrl+C), after which it
closes its sockets and exits cleanly.


2. Build
--------
Run make in the project directory:

    make

This produces the executable `dns`. To remove build artifacts:

    make clean

Requirements: a C compiler (gcc) and a POSIX environment. The code is built
with `-Wall -Wextra -O2`.


3. Usage
--------
    ./dns -s server [-p port] -f filter_file [-v]

    -s server        IPv4 address or hostname of the upstream DNS resolver
                     to which non-filtered queries are forwarded (port 53).
                     Required.
    -p port          Port the server listens on. Default: 53.
                     Ports below 1024 require root privileges.
    -f filter_file   Path to the file with the list of filtered domains.
                     Required.
    -v               Verbose mode: print loaded domain count and every
                     incoming query to standard error.

If the arguments are invalid or a required option is missing, the program
prints a short usage message to standard error and exits with code 1.


4. Filter file format
---------------------
One domain per line. The parser:

  * strips trailing CR/LF characters,
  * ignores empty lines and lines beginning with `#`,
  * accepts only the characters a-z, A-Z, 0-9, `.` and `-`, and requires at
    least one letter; lines with invalid characters are reported in verbose
    mode,
  * skips domains that are 256 characters or longer.

Up to 5000 domains are loaded. The filter is read once at start-up.

An example file, `blocked.txt`, is included.


5. Examples
-----------
Start the server on an unprivileged port and forward to Google Public DNS:

    ./dns -s 8.8.8.8 -p 5300 -f blocked.txt -v

Query it with dig (in another terminal):

    dig @127.0.0.1 -p 5300 google.com   A       # forwarded  -> NOERROR
    dig @127.0.0.1 -p 5300 example.bad  A       # filtered   -> REFUSED
    dig @127.0.0.1 -p 5300 sub.example.bad A    # subdomain  -> REFUSED
    dig @127.0.0.1 -p 5300 google.com   AAAA    # non-A      -> NOTIMP

On the default DNS port (requires root):

    sudo ./dns -s 8.8.8.8 -f blocked.txt



1. File list
------------
    README
    Makefile
    blocked.txt
    test.sh
    src/main.c
    src/argument_parse.c
    src/argument_parse.h
    src/parse_dns.c
    src/parse_dns.h
    src/read_filter.c
    src/read_filter.h
    src/response_dns.c
    src/response_dns.h
    src/utils_dns.c
    src/utils_dns.h
