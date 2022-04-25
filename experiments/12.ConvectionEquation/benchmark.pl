#!/usr/bin/env perl

use strict;
use warnings;
use Benchmark;

my $prog_path = "./prog";
my $input_file_path = "input.txt";
my $X = 1.0;
my $T = 0.05;

sub run_benchmark {
  my %args = @_;
  my $NProc = $args{NProc} || 1;
  my $time = qx(mpirun -n ${NProc} $prog_path -X $X -T $T -M $args{M} -K $args{K} --time --file '$input_file_path');
  chomp $time;
  printf "| %-5d | %-5d | %-5d | %-10s |\n", $args{M}, $args{K}, $NProc, $time;
}

print "Running benchmarks. X = $X, T = $T\n";
my $header = sprintf "| %-5s | %-5s | %-5s | %-10s |", "M", "K", "NProc", "Time";
my $line = "-" x length $header;
print "$line\n$header\n";

sub horizontal_line { print "$line\n"; }

horizontal_line;
run_benchmark(M => 1 << 10, K => 1 << 10);
run_benchmark(M => 1 << 10, K => 4 << 10);
run_benchmark(M => 1 << 10, K => 16 << 10);
run_benchmark(M => 1 << 10, K => 64 << 10);
horizontal_line;
run_benchmark(M => 1 << 10, K => 1 << 10);
run_benchmark(M => 4 << 10, K => 1 << 10);
run_benchmark(M => 16 << 10, K => 1 << 10);
run_benchmark(M => 64 << 10, K => 1 << 10);
horizontal_line;
run_benchmark(M => 16 << 10, K => 16 << 10, NProc => $_) for (1, 3, 4, 8, 16);
horizontal_line;
