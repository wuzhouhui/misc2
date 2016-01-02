#!/usr/bin/awk  -f
#Note: on solaris, use nawk instead
BEGIN {
    for (i = 0; i< ARGC; i++)
        printf "ARGV[%d] = %s\n", i, ARGV[i]
    exit
}
