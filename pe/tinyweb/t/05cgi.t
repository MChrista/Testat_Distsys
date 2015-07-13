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
use IPC::System::Simple qw(capture);

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
    # CGI
    [ { method => 'GET',  url => "/cgi-bin/hello.pl", status => 200 } ],
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
    $req->header('Accept' => '*/*');

    # Pass request to the user agent and get a response back from the server
    my $res = $ua->request($req);

    subtest "$method '$url'" => sub {
        #--------------------------------------------------
        # Subtest: HTTP Status is as expected
        #--------------------------------------------------
        my $status = $ref->{status};
        like($res->status_line, qr/^$status/, "Status $status");

        #--------------------------------------------------
        # Subtest: Date and time is correct
        #--------------------------------------------------
        check_date_header($res->headers->{'date'});

        #--------------------------------------------------
        # Subtest: Header field 'Server' is provided
        #--------------------------------------------------
        isnt($res->headers->{'server'}, undef, "Server");

        if (($ref->{status} == 200)) {
            #------------------------------------------------------------------
            # Status: 200 OK
            #------------------------------------------------------------------
            # Call the CGI script directly and compare its output with the
            # response body sent by the server
            my $cgiresult = capture($^X, "$root_dir/$url", []);
            my $index = index($cgiresult, "\r\n\r\n");
            $cgiresult = substr $cgiresult, $index+4 if $index != -1;
            #------------------------------------------------------------------
            # Subtest: Provided response body matches result from direct call
            #------------------------------------------------------------------
            is($res->content, $cgiresult, "CGI contents body");
        } else {
            #------------------------------------------------------------------
            # Status: any other
            #------------------------------------------------------------------
            # do nothing, there are no additional sub-tests
            ;
        } # end if
    };
} # end of connect_to_server

