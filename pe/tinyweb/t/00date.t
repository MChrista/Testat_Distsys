#!/usr/bin/perl

use strict;
use warnings;
use lib 't/lib';

# required to set LC_TIME
use locale;
use POSIX qw(locale_h); # Imports setlocale() and the LC_ constants.

use POSIX qw(tzset);
use LWP::UserAgent;
use Test::More;
use TinyWebTest qw(check_date_header);

my $root_dir    = "web";
my $remote_host = "localhost";
my $remote_port = "8080";
my $remote_path = "";

my $locale_str = "en_US.UTF-8";
setlocale(LC_TIME, $locale_str) or die "Cannot set LC_TIME to '$locale_str'";

#--------------------------------------------------------------------------
# Test Cases
#--------------------------------------------------------------------------
my @tests = (
    # GET Method
    [ { method => 'GET',  url => "/index.html" } ],
    [ { method => 'GET',  url => "/css/default.css" } ],
    # Non-existing file
    [ { method => 'GET',  url => "/blablabla.html" } ],
    [ { method => 'GET',  url => "/css/xyz.html" } ],
    # HEAD Method
    [ { method => 'HEAD',  url => "/index.html" } ],
    [ { method => 'HEAD',  url => "/css/default.css" } ],
    # Non-existing file
    [ { method => 'HEAD',  url => "/blablabla.html" } ],
    [ { method => 'HEAD',  url => "/css/xyz.html" } ],
);

# Set the number of test cases (excluding subtests)
plan tests => scalar @tests;

# Force the time zone to be GMT
$ENV{TZ} = 'GMT';
tzset;

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
    # test: Date and time is correct
    #--------------------------------------------------
    check_date_header($res->headers->{'date'}, "$method $url");
} # end of connect_to_server

