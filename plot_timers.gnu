set datafile separator ","
set key outside bottom center horizontal offset 0,-0.2
set key box
set grid
set style data linespoints
set term pngcairo size 600,400

# Define line styles explicitly:
# 1 = solid line
# 2 = dotted line

set style line 1 lt 1 lw 2 pt 7 lc rgb "black"   # solid line with points
set style line 2 lt 0 lw 2 pt 7 dt (1, 1) lc rgb "black"

# Plot for rank scaling
set output "all_chunk_rank_scaling.png"
set title "All Chunks Scaling by Number of Ranks (Fixed 4 Chunks Per Rank)"
set xlabel "Number of Ranks"
set ylabel "Elapsed Time (s)"
set logscale x 2
plot \
    'rank_write_all_chunks.csv' using 3:5 title 'h5 write all chunks' ls 1, \
    'rank_read_all_chunks.csv'  using 3:5 title 'h5 read all chunks'  ls 2

# Plot for rank scaling
set output "per_chunk_rank_scaling.png"
set title "Per Chunk Scaling by Number of Ranks (Fixed 4 Chunks Per Rank)"
set xlabel "Number of Ranks"
set ylabel "Elapsed Time (s)"
set logscale x 2
plot \
    'rank_write_chunk.csv' using 3:5 title 'h5 write chunk' ls 1, \
    'rank_read_chunk.csv'  using 3:5 title 'h5 read chunk'  ls 2

# Plot for chunk scaling
set output "all_chunk_chunk_scaling.png"
set title "All Chunks Scaling by Chunks per Rank (Fixed 8 Ranks)"
set xlabel "Chunks Per Rank"
set ylabel "Elapsed Time (s)"
set logscale x 2
plot \
    'chunk_write_all_chunks.csv' using 2:5 title 'h5 write all chunks' ls 1, \
    'chunk_read_all_chunks.csv'  using 2:5 title 'h5 read all chunks'  ls 2

# Plot for chunk scaling
set output "per_chunk_chunk_scaling.png"
set title "Per Chunk Scaling by Chunks per Rank (Fixed 8 Ranks)"
set xlabel "Chunks Per Rank"
set ylabel "Elapsed Time (s)"
set logscale x 2
plot \
    'chunk_write_chunk.csv' using 2:5 title 'h5 write chunk' ls 1, \
    'chunk_read_chunk.csv'  using 2:5 title 'h5 read chunk'  ls 2
