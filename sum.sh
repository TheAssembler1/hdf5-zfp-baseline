#!/bin/bash

awk 
{
  gsub(/ /, "", $3);              # Remove any spaces
  if ($3 ~ /^[0-9]+$/) {          # Ensure it's a number
    total += $3;                  # Sum the nanoseconds
  }
}
END {
  printf "Total time:\n";
  printf "  %d ns\n", total;
  printf "  %.3f ms\n", total / 1e6;
  printf "  %.6f s\n", total / 1e9;
}' data.txt

