set datafile separator ","
set key outside bottom center horizontal
set key box
set grid
set style data linespoints
set term pngcairo size 400,350

# Define line styles explicitly:
# 1 = solid line
# 2 = dotted line

set style line 1 lt 1 lw 2 pt 7 lc rgb "black"         # Solid black with points
set style line 2 lt 1 lw 2 pt 7 dt (1,1) lc rgb "red"  # Red dashed with points
set style line 3 lt 1 lw 2 pt 5 lc rgb "#0072B2"       # Solid blue with points
set style line 4 lt 1 lw 2 pt 5 dt (2,2) lc rgb "#D55E00"  # Orange dashed with points

# Plot for rank scaling
set output "all_chunk_rank_scaling.png"
set title "Scaling by Ranks"
set xlabel "Number of Ranks"
set ylabel "Elapsed Time (s)"
set logscale x 2
plot \
    'hdf5_rank_write_all_chunks.csv' using 3:5 title 'hdf5 write' ls 1, \
    'hdf5_rank_read_all_chunks.csv'  using 3:5 title 'hdf5 read'  ls 2, \
    'pdc_rank_write_all_chunks.csv' using 3:5 title 'pdc write' ls 3,   \
    'pdc_rank_read_all_chunks.csv'  using 3:5 title 'pdc read'  ls 4

# Plot for per chunk rank scaling
set output "per_chunk_rank_scaling.png"
set title "Per Chunk Scaling by Rank"
set xlabel "Number of Ranks"
set ylabel "Elapsed Time (s)"
set logscale x 2
plot \
    'hdf5_rank_write_chunk.csv' using 3:5 title 'hdf5 write' ls 1, \
    'hdf5_rank_read_chunk.csv'  using 3:5 title 'hdf5 read'  ls 2, \
    'pdc_rank_write_chunk.csv' using 3:5 title 'pdc write' ls 3,   \
    'pdc_rank_read_chunk.csv'  using 3:5 title 'pdc read'  ls 4

# Plot for chunk scaling
set output "all_chunk_chunk_scaling.png"
set title "Scaling by Chunks"
set xlabel "Chunks Per Rank"
set ylabel "Elapsed Time (s)"
set logscale x 2
plot \
    'hdf5_chunk_write_all_chunks.csv' using 2:5 title 'hdf5 write' ls 1, \
    'hdf5_chunk_read_all_chunks.csv'  using 2:5 title 'hdf5 read'  ls 2, \
    'pdc_chunk_write_all_chunks.csv' using 2:5 title 'pdc write' ls 3,   \
    'pdc_chunk_read_all_chunks.csv'  using 2:5 title 'pdc read'  ls 4

# Plot for per chunk, chunk scaling
set output "per_chunk_chunk_scaling.png"
set title "Per Chunk Scaling by Chunks"
set xlabel "Chunks Per Rank"
set ylabel "Elapsed Time (s)"
set logscale x 2
plot \
    'hdf5_chunk_write_chunk.csv' using 2:5 title 'hdf5 write' ls 1, \
    'hdf5_chunk_read_chunk.csv'  using 2:5 title 'hdf5 read'  ls 2, \
    'pdc_chunk_write_chunk.csv' using 2:5 title 'pdc write' ls 3, \
    'pdc_chunk_read_chunk.csv'  using 2:5 title 'pdc read'  ls 4
