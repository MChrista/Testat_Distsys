#!/usr/bin/perl

use strict;
use warnings;

use LWP::UserAgent;
use Test::More;

my $root_dir    = "web";
my $remote_host = "localhost";
my $remote_port = "8080";
my $remote_path = "";


#--------------------------------------------------------------------------
# Test Cases
#--------------------------------------------------------------------------
my @tests = (
    # GET Method
    [ { method => 'GET',  url => "/index.html", status => 200 } ],
    [ { method => 'GET',  url => "/css/default.css", status => 200 } ],
    # Non-existing file
    [ { method => 'GET',  url => "/blablabla.html", status => 404 } ],
    [ { method => 'GET',  url => "/css/xyz.html", status => 404 } ],
    # HEAD Method
    [ { method => 'HEAD',  url => "/index.html", status => 200 } ],
    [ { method => 'HEAD',  url => "/css/default.css", status => 200 } ],
    # Non-existing file
    [ { method => 'HEAD',  url => "/blablabla.html", status => 404 } ],
    [ { method => 'HEAD',  url => "/css/xyz.html", status => 404 } ]
);

# Set the number of test cases (excluding subtests)
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
#      'url'    -> URL
#      'status' -> expected HTTP status in the response
#
# Return value: NONE
#
#--------------------------------------------------------------------------
sub connect_to_server {
    my $ref = shift;

    my $method = $ref->{method};
    my $url = $ref->{url};

    # Create a user agent object
    my $ua = LWP::UserAgent->new(max_redirect => 0, timeout => 30);
    $ua->agent("TinyWeb Test Harness, Test Script $0");

    # Create a request
    my $req = HTTP::Request->new($method => "http://$remote_host:$remote_port$remote_path$url");

    # Pass request to the user agent and get a response back from the server
    my $res = $ua->request($req);

    #--------------------------------------------------
    # Subtest: HTTP Status is as expected
    #--------------------------------------------------
    my $status = $ref->{status};
    like($res->status_line, qr/^$status/, "Status $status $method $url");
} # end of connect_to_server

