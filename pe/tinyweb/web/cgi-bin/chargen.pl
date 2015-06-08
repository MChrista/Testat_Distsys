#!/usr/bin/perl

srand( time() ^ ($$ + ($$ << 15)) );
my @v = qw ( a e i o u y );
my @c = qw ( b c d f g h j k l m n p q r s t v w x z );

my $lines = 1000000;
my $columns = 78;      # number of char per line, excluding CRLF

print "Content-Length: ", $lines * ($columns+2) , "\n";
print "Content-Type: text/plain\n\n";

for (1 .. $lines) {
    my ($flip, $str) = (0,'');
    $str .= ($flip++ % 2) ? $v[rand(6)] : $c[rand(20)] for 1 .. $columns;
    $str =~ s/(....)/$1 . int rand(10)/e;
    $str = ucfirst $str if rand() > 0.5;
    print "$str\n";
} # end for

exit 0;

