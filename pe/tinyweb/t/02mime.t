#!/usr/bin/perl

use strict;
use warnings;
use lib 't/lib';

use File::MimeInfo;
use LWP::UserAgent;
use Test::More;
use TinyWebTest qw(get_url_properties);

my $root_dir    = "web";
my $remote_host = "localhost";
my $remote_port = "8080";
my $remote_path = "";

#--------------------------------------------------------------------------
# Test Cases
#--------------------------------------------------------------------------
my @tests = (
    [ { method => 'GET',  url => "/index.html", status => 200 } ],
    [ { method => 'HEAD', url => "/index.html", status => 200 } ],
    [ { method => 'HEAD', url => "/images/computerhead1.gif", status => 200 } ],
    [ { method => 'GET',  url => "/images/computerhead1.gif", status => 200 } ],
    [ { method => 'HEAD', url => "/example.pdf", status => 200 } ],
    [ { method => 'GET',  url => "/example.pdf", status => 200 } ],
    [ { method => 'HEAD', url => "/css/default.css", status => 200 } ],
    [ { method => 'GET',  url => "/css/default.css", status => 200 } ],
    [ { method => 'GET',  url => "/zeros1.jpg", status => 200 } ],
    [ { method => 'GET',  url => "/zeros2.jpg", status => 200 } ],
    [ { method => 'GET',  url => "/zeros3.jpg", status => 200 } ]
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

    # Determine file properties
    (my $file, my $file_time, my $file_size) = get_url_properties($root_dir, $ref);

    #------------------------------------------------------------------
    # test: Header field 'Content-Type' matches file type
    #------------------------------------------------------------------
    my $mime = mimetype($file);
    is($res->headers->{'content-type'}, $mime, "$mime <- $method $url");
} # end of connect_to_server

