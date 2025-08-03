#!/usr/bin/env python3
# coding=utf-8

import os
import tempfile
from collections import defaultdict
from pygnuplot import gnuplot

def save_data(x_vals, y_vals):
    """Save x and y values to a temporary .dat file."""
    tmp = tempfile.NamedTemporaryFile(delete=False, mode='w', suffix='.dat')
    for x, y in zip(x_vals, y_vals):
        tmp.write(f'{x} {y}\n')
    tmp.close()
    print(f'tmp file written to {tmp.name}')
    return tmp.name

def main():
    # Data structure:
    # dict_csv[timer_tag][filter][workload_name][io_participation] = list of (num_ranks, elapsed_seconds)
    dict_csv = defaultdict(
        lambda: defaultdict(
            lambda: defaultdict(
                lambda: defaultdict(list)
            )
        )
    )

    # Read CSV file
    try:
        with open('./build/output.csv', 'r') as file:
            header_skipped = False
            for line in file:
                if not header_skipped:
                    print("Skipping header: " + line.strip())
                    header_skipped = True
                    continue
                parts = line.strip().split(',')
                workload_name = parts[0]
                num_ranks = int(parts[2])
                timer_tag = parts[3]
                elapsed_seconds = float(parts[4])
                io_participation = parts[6]
                filter_key = parts[7]

                dict_csv[timer_tag][filter_key][workload_name][io_participation].append((num_ranks, elapsed_seconds))
    except FileNotFoundError:
        print("Error: The file does not exist.")
        return
    except IOError:
        print("Error: Could not read the file due to an IO error.")
        return

    os.makedirs('./res', exist_ok=True)
    g = gnuplot.Gnuplot(log=True)
    num_point_types = 5

    # Iterate over timer_tag and filter for charts
    for timer_tag in dict_csv:
        for filter_key in dict_csv[timer_tag]:
            print(f"Plotting chart for timer_tag={timer_tag}, filter={filter_key}")

            plot_cmds = []
            cur_point_type = 1

            # For each workload_name and io_participation, plot a line
            for workload_name in dict_csv[timer_tag][filter_key]:
                for io_participation in dict_csv[timer_tag][filter_key][workload_name]:
                    points = dict_csv[timer_tag][filter_key][workload_name][io_participation]
                    points.sort(key=lambda p: p[0])
                    x_vals = [p[0] for p in points]
                    y_vals = [p[1] for p in points]

                    label = f"{workload_name} {io_participation.title()}"
                    data_file = save_data(x_vals, y_vals)

                    pt = (cur_point_type % num_point_types) + 1
                    plot_cmds.append(f'"{data_file}" with linespoints title "{label}" pt {pt} ps 1.5 lw 2')
                    cur_point_type += 1

            if plot_cmds:
                safe_timer_tag = timer_tag.replace(' ', '_')
                safe_filter = filter_key.replace(' ', '_')
                output_path = f'./res/{safe_timer_tag}_{safe_filter}.png'

                g.set(
                    terminal='pngcairo size 400,400 font "Consolas,12" crop',
                    output=f'"{output_path}"',
                    title=f'"{timer_tag.replace("_", " ").title()} | {filter_key.replace("_", " ").title()}"',
                    xlabel='"Number of Ranks"',
                    ylabel='"Elapsed Time (s)"',
                    key='outside center bottom box vertical',
                    grid='xtics ytics mxtics mytics',
                    size='ratio 0.6'
                )
                g.cmd('set logscale x 2')
                g.plot(', '.join(plot_cmds))
            else:
                print(f"No data to plot for {timer_tag}, {filter_key}")

if __name__ == "__main__":
    main()
