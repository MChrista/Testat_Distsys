#!/usr/bin/perl

use strict;
use warnings;

use Test::More;
use IO::Socket::IP;


my $root_dir    = "web";
my $remote_host = "localhost";
my $remote_port = "8080";


#--------------------------------------------------------------------------
# Test Cases
#--------------------------------------------------------------------------
my @tests = (
    [ "GET",      200 ],
    [ "HEAD",     200 ],
    [ "OPTIONS",  501 ],
    [ "POST",     501 ],
    [ "PUT",      501 ],
    [ "DELETE",   501 ],
    [ "TRACE",    501 ],
    [ "CONNECT",  501 ],
    [ "DUMMY",    501 ]
);

plan tests => scalar @tests;

connect_to_server(@$_) for @tests;

exit 0;


#--------------------------------------------------------------------------
# Establish an HTTP connection to a server and perform tests on the
# returned HTTP response
#
# Parameter(s):
# (IN) Reference to a hash containing test data
#      'method' -> HTTP method be used in HTTP request
#      'status' -> expected HTTP status in the response
#
# Return value: NONE
#
#--------------------------------------------------------------------------
sub connect_to_server {
    my $method = shift;
    my $status = shift;

    my $socket = IO::Socket::IP->new(
                PeerAddr => $remote_host,
                PeerPort => $remote_port,
                Type     => SOCK_STREAM
    ) or die "ERROR: socket() - $@";

    print $socket "$method /index.html HTTP/1.1\r\n\r\n";

    my  $response = <$socket>;
    if (defined $response) {
        my $answer = $response =~ s/\R\z//r;
        if (defined $answer) {
            my @fields = split " ", $answer;
            is($fields[1], $status, "Method $method: Status $status");
        } else {
            fail("Method $method: response format");
        } # end if
    } else {
        fail("Method $method: no response");
    } # end if

    close($socket);
} # end of connect_to_server

