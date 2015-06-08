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
use TinyWebTest qw(get_url_properties);
use TinyWebTest qw(check_file_content);

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
    # Redirection in case of a directory
    [ { method => 'HEAD', url => "/css", status => 301, location => "/css/" } ],
    [ { method => 'GET',  url => "/css", status => 301, location => "/css/" } ],
    [ { method => 'HEAD', url => "/source", status => 301, location => "/source/" } ],
    [ { method => 'GET',  url => "/source", status => 301, location => "/source/" } ],
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
#      'status' -> expected HTTP status in the response
#
# Return value: NONE
#
#--------------------------------------------------------------------------
sub connect_to_server {
    my $ref = shift;

    my $method = $ref->{method};
    my $url = $ref->{url};
    my $offset = undef;

    # Create a user agent object
    my $ua = LWP::UserAgent->new(max_redirect => 0, timeout => 30);
    $ua->agent("TinyWeb Test Harness, Test Script $0");

    # Create a request
    my $req = HTTP::Request->new($method => "http://$remote_host:$remote_port$remote_path$url");

    # Pass request to the user agent and get a response back from the server
    my $res = $ua->request($req);

    subtest "$method '$url'" => sub {
        #--------------------------------------------------
        # Subtest: HTTP Status is as expected
        #--------------------------------------------------
        like($res->status_line, qr/^$ref->{status}/, "Status");

        #--------------------------------------------------
        # Subtest: Date and time is correct
        #--------------------------------------------------
        check_date_header($res->headers->{'date'});

        #--------------------------------------------------
        # Subtest: Header field 'Server' is provided
        #--------------------------------------------------
        isnt($res->headers->{'server'}, undef, "Server");

        if ($ref->{status} == 301) {
            #------------------------------------------------------------------
            # Status: 301 Moved Permanently
            #------------------------------------------------------------------
            my $regex = qr/$ref->{location}$/;
            #------------------------------------------------------------------
            # Subtest: Header field 'Location' contains redirection link
            #------------------------------------------------------------------
            like($res->headers->{'location'}, $regex, "Location");
        } # end if
    };
} # end of connect_to_server

