#!/usr/bin/env perl
# Runs some simple tests

use strict;
use warnings;
use Carp;

sub run_test {
  my %args = @_;
  my $cmd = "echo $args{input} | tr ' ' '\\n' | $args{command}";
  my $output = qx($cmd);
  chomp $args{output};
  chomp $output;
  croak "Test FAILED" unless ($args{output} eq $output);
}

run_test(
  command => "mpirun -n $_ ./prog -X 6 -T 6 -M 6 -K 6 --data",
  input => "x t 0",
  output => "6 5 4 3 2 1 0"
) for 1..3;

run_test(
  command => "mpirun -n $_ ./prog -X 6 -T 6 -M 6 -K 6 --data",
  input => "x t x+2*t",
  output => "6 17 25 35 42 49 53"
) for 1..3;
