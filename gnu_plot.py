#!/usr/bin/env python3
# coding=utf-8

import os
import tempfile
from collections import defaultdict
from pygnuplot import gnuplot

def save_data(x_vals, y_vals):
    """
    Save x and y values into a temporary .dat file for gnuplot input.
    Each line contains one data point with x and y separated by a space.
    
    Returns:
        The filename of the created temporary data file.
    """
    tmp = tempfile.NamedTemporaryFile(delete=False, mode='w', suffix='.dat')
    for x, y in zip(x_vals, y_vals):
        tmp.write(f'{x} {y}\n')
    tmp.close()
    print(f'tmp file written to {tmp.name}')  # Debug: show where data was saved
    return tmp.name

def main():
    # Nested defaultdict structure to organize CSV data as:
    # dict_csv[timer_tag][filter_key][workload_name][io_participation] = list of (num_ranks, elapsed_seconds)
    dict_csv = defaultdict(
        lambda: defaultdict(
            lambda: defaultdict(
                lambda: defaultdict(list)
            )
        )
    )

    # Attempt to read and parse the CSV input file line by line
    try:
        with open('./build/output.csv', 'r') as file:
            header_skipped = False
            for line in file:
                # Skip header line once
                if not header_skipped:
                    print("Skipping header: " + line.strip())
                    header_skipped = True
                    continue
                
                # Split CSV line into components
                parts = line.strip().split(',')

                # Extract relevant fields
                workload_name = parts[0]
                num_ranks = int(parts[2])
                timer_tag = parts[3]
                elapsed_seconds = float(parts[4])
                io_participation = parts[6]
                filter_key = parts[7]

                # Store data point into nested dictionary structure
                dict_csv[timer_tag][filter_key][workload_name][io_participation].append((num_ranks, elapsed_seconds))
    except FileNotFoundError:
        print("Error: The file does not exist.")
        return
    except IOError:
        print("Error: Could not read the file due to an IO error.")
        return

    # Ensure output directory exists for saving plots
    os.makedirs('./res', exist_ok=True)
    g = gnuplot.Gnuplot(log=True)

    # Iterate over each timer_tag and filter key combination to generate separate charts
    for timer_tag in dict_csv:
        for filter_key in dict_csv[timer_tag]:
            print(f"Plotting chart for timer_tag={timer_tag}, filter={filter_key}")

            plot_cmds = []  # Holds all plot command fragments for this chart

            # Variables to cycle through point shapes, line styles, and colors
            cur_icon_and_line_style = 0
            cur_line_color = 0

            # Predefined line styles to visually differentiate lines:
            # 'lt' = linetype (solid/dashed/dotted)
            # 'dashtype' specifies custom dash patterns: (on_length, off_length, ...)
            line_styles = [
                'lt 1',                    # solid line
                'dashtype (10,5)',         # long dash pattern
                'dashtype (2,6)',          # dotted pattern
                'dashtype (15,5,2,5)',     # dash-dot pattern
                'lt 3'                     # short dash (example)
            ]

            # List of colors (hex RGB strings) to cycle through for lines
            colors = [
                '#1f77b4',  # blue
                '#ff7f0e',  # orange
                '#2ca02c',  # green
                '#d62728',  # red
                '#9467bd',  # purple
                '#8c564b',  # brown
            ]

            # Point types to cycle through for markers:
            # 6 = filled circle, 4 = filled square, 
            # 2 = filled triangle, 5 = hollow square
            point_types = [6, 4, 2, 5]

            # For each unique workload and io participation 
            # within current timer_tag/filter_key
            for workload_name in dict_csv[timer_tag][filter_key]:

                for io_participation in dict_csv[timer_tag][filter_key][workload_name]:
                    points = dict_csv[timer_tag][filter_key][workload_name][io_participation]

                    # Sort points by number of ranks (x-value) to make lines progress properly
                    points.sort(key=lambda p: p[0])
                    x_vals = [p[0] for p in points]
                    y_vals = [p[1] for p in points]

                    # Compose the legend label combining workload 
                    # and IO participation (capitalized)
                    label = f"{workload_name} {io_participation.title()}"

                    # Save data points to a temporary file for gnuplot consumption
                    data_file = save_data(x_vals, y_vals)

                    # Select point type (marker shape) cycling through point_types list
                    pt = point_types[cur_icon_and_line_style % len(point_types)]

                    # Select line style cycling through line_styles list
                    style = line_styles[cur_icon_and_line_style % len(line_styles)]

                    # Select color cycling through colors list (changes per line)
                    color = colors[cur_line_color % len(colors)]

                    # Compose the gnuplot plot command fragment for this data series
                    plot_cmds.append(
                        f'"{data_file}" with linespoints title "{label} " pt {pt} ps 3 lw 6 {style} lc rgb "{color}"'
                    )
                    
                    # Increment color index after each line to ensure 
                    # different colors for lines within same workload
                    cur_line_color += 1

                # Increment icon and line style index once per workload 
                # to group styles consistently by workload
                cur_icon_and_line_style += 1

            # If we have any plot commands, configure gnuplot and generate the plot image
            if plot_cmds:
                # Sanitize timer_tag and filter_key to safe filenames 
                # by replacing spaces with underscores
                safe_timer_tag = timer_tag.replace(' ', '_')
                safe_filter = filter_key.replace(' ', '_')

                # Define output image filepath
                output_path = f'./res/{safe_timer_tag}_{safe_filter}.png'

                # Define a custom line style for grid lines: gray color, dashed pattern, width 3
                g.cmd('set style line 101 lt rgb "#444444" dashtype (30, 30) lw 3')

                # Define a custom thicker black line style for the plot border
                g.cmd('set style line 200 lw 4 lc rgb "black"')
                g.cmd('set border ls 200')  # Apply the thicker border style

                # Set terminal type, output file, font, plot title, axis labels, 
                # legend key configuration, grid style, and plot size ratio
                g.set(
                    terminal='pngcairo size 1600,1600 font "Consolas,28" crop',
                    output=f'"{output_path}"',
                    title=f'"{timer_tag.replace("_", " ").title()} | {filter_key.replace("_", " ").title()}"',
                    xlabel='"Number of Ranks"',
                    ylabel='"Elapsed Time (s)"',
                    key='Left top left reverse box vertical font ",28"',  # Legend position and style
                    grid='xtics ytics mxtics mytics ls 101',              # Grid lines style (using line style 101)
                    size='ratio 0.6'                                      # Aspect ratio of plot
                )

                # Set x-axis to log scale base 2 for better visualization of rank counts
                g.cmd('set logscale x 2')
                g.plot(', '.join(plot_cmds))

            else:
                print(f"No data to plot for {timer_tag}, {filter_key}")

if __name__ == "__main__":
    main()
