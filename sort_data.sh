#!/bin/bash

awk '{
  # Remove spaces in the 3rd column (the time field)
  gsub(/ /, "", $3);
  # Split the 3rd field by comma into seconds and fraction
  split($3, t, ",");
  sec = (t[1] != "") ? t[1] : 0;
  frac = (t[2] != "") ? t[2] : 0;
  # Convert to float seconds: sec + fractional part scaled
  time = sec + frac / 1e9;
  # Print a sortable key, tab, then full line
  printf "%020.9f\t%s\n", time, $0;
}' client_data.txt | sort -n | cut -f2-

