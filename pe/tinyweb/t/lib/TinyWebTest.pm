package TinyWebTest;

use strict;
use warnings;
use Exporter;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK %EXPORT_TAGS);


use File::stat;
use POSIX qw(strftime);
use POSIX::strptime;
use POSIX qw(tzset);
use Test::More;

$VERSION     = 1.00;
@ISA         = qw(Exporter);
@EXPORT      = ();
@EXPORT_OK   = qw(get_gmtime check_date_header get_url_properties check_file_content);
%EXPORT_TAGS = ( DEFAULT => [qw(&check_date_header get_url_properties check_file_content)] );


sub get_gmtime {
    my $time = shift;

    return strftime "%a, %d %b %Y %H:%M:%S GMT", gmtime ($time);
} # end of get_gmtime


sub check_file_content {
    my $content = shift;
    my $file    = shift;
    my $offset  = shift;

    my $st = stat($file) or die "ERROR: cannot access $file: $!";

    $offset = 0 unless defined $offset;

    # Compare the length of the response body with the size of the requested range 
    is(length($content), $st->size - $offset, "Response body length: " . $st->size);

    # Read the contents of the file and compare it with the contents
    # of the response body. Read the file in one go and not line by
    # line, as it is the default.
    my $resource = do {
        local $/ = undef;
        open my $fh, "<", $file
           or die "ERROR: cannot open $file: $!";
        seek $fh, $offset, 0 if $offset > 0;
        <$fh>;
    };
    ok($content eq $resource, "Response body content: '$file'");
} # end of check_file_content


sub check_date_header {
    my $date_header = shift;
    my $note = shift;

    if (defined $date_header) {
        # Get the current system time in GMT
        $note = gmtime unless defined $note;
        my @tm_now = gmtime;
        my $time_now = POSIX::mktime(@tm_now);

        # Parse the server date header and translate into a time value, ignore the DST field
        # which may or may be not initialised depending on the host system.
        my @date_fields = (POSIX::strptime $date_header, "%a, %d %b %Y %H:%M:%S")[0,1,2,3,4,5,6];
        unless (grep {not defined($_)} @date_fields) {
            my $time_response = POSIX::mktime(@date_fields);
            # Finally, compare the two time values...
            my $delta = abs($time_response - $time_now);
            ok($delta <= 1, "Date $note") or print "Note: Time difference is $delta sec.\n";
        } else {
            fail("Date-Header: not valid '$date_header'");
        } # end if
    } else {
        fail("Date-Header: missing");
    } # end if
} # end of check_date_header


sub get_url_properties {
    my $root_dir = shift;
    my $ref = shift;

    my $file = $root_dir . ((exists $ref->{file}) ? $ref->{file} : $ref->{url});
    my $st = stat($file) or die "ERROR: cannot access $file: $!";
    my $file_time = strftime "%a, %d %b %Y %H:%M:%S GMT", gmtime $st->mtime;

    return ($file, $file_time, $st->size);
} # end of get_url_properties

