from collections import defaultdict
import matplotlib.pyplot as plt
import numpy as np

markers = ['s',  # square
           'p',  # pentagon
           '*',  # star
           'h',  # hexagon1
           '+',  # plus
           'x',  # x
           'D']  # diamond

line_styles = ['-',    # solid line
               '--',   # dashed line
               '-.',   # dash-dot line
               ':']    # dotted line

colors = [
    'b',       # blue
    'g',       # green
    'r',       # red
    'c',       # cyan
    'm',       # magenta
    'y',       # yellow
    'k']       # black

def create_figure_with_pixel_size(width_px, height_px, dpi=100):
    """
    Create a Matplotlib figure with the specified width and height in pixels.

    Parameters:
        width_px (int): Width of the figure in pixels.
        height_px (int): Height of the figure in pixels.
        dpi (int): Dots per inch (resolution). Default is 100.

    Returns:
        fig: The created Matplotlib figure object.
    """
    width_in = width_px / dpi
    height_in = height_px / dpi
    fig = plt.figure(figsize=(width_in, height_in), dpi=dpi)
    return fig

def main():
    dict_csv = defaultdict(lambda: defaultdict(lambda: defaultdict(lambda: defaultdict(list))))
    try:
        with open('./build/output.csv', 'r') as file:
            cur_line = 0
            for line in file:
                if cur_line == 0:
                    print("Skipping header: " + str(line.strip()))
                    cur_line += 1
                    continue
                parts = line.strip().split(',')
                # timer_tag -> workload_name  -> io participation -> filter -> actual plot data
                dict_csv[parts[3]][parts[0]][parts[6]][parts[7]].append(parts)
    except FileNotFoundError:
        print("Error: The file does not exist.")
    except IOError:
        print("Error: Could not read the file due to an IO error.")

    # each tag is a seperate graph
    for timer_tag_key in dict_csv.keys():
        print("Found timer tag: " + timer_tag_key)

        fig = plt.figure()
        ax = fig.add_subplot(1, 1, 1)
        cur_line_style = 0

        # each loop creates a singe set of x y values
        for workload_name_key in dict_csv[timer_tag_key]:
            print("\tFound workload name: " + workload_name_key)
            for io_participation_key in dict_csv[timer_tag_key][workload_name_key]:
                print("\t\tFound io participation: " + io_participation_key)
                for filter_key in dict_csv[timer_tag_key][workload_name_key][io_participation_key]:
                    print("\t\t\tFound filter: " + filter_key)

                    x_vals = []
                    y_vals = []

                    for parts in dict_csv[timer_tag_key][workload_name_key][io_participation_key][filter_key]:
                        print("\t\t\t\t" + parts[2])
                        print("\t\t\t\t" + parts[4])
                        x_vals.append(int(parts[2]))
                        y_vals.append(float(parts[4]))

                    marker = markers[cur_line_style % len(markers)]
                    linestyle = line_styles[cur_line_style % len(line_styles)]
                    color = colors[cur_line_style % len(colors)]

                    ax.plot(x_vals, y_vals,
                            marker=marker,
                            linestyle=linestyle,
                            color=color,
                            label=(workload_name_key + ' ' + io_participation_key + ' ' + filter_key),
                            lw=2)

                    cur_line_style += 1

        title = timer_tag_key
        filename = "./res/" + timer_tag_key + ".png"

        plt.style.use('classic')
        ax.set_title(title)
        ax.set_ylabel("time (s)")
        ax.set_xlabel("ranks")
        ax.legend(
            loc='upper center',
            bbox_to_anchor=(0.5, -0.15),
            ncol=3,
            frameon=True,
            fontsize=10,
            handlelength=0.2,     # shorter legend lines
            handletextpad=0.5,  # less space between marker and text
            borderpad=0.5,       # less padding around the legend content
            labelspacing=0.1
        )
        ax.grid(True)
        plt.tight_layout()
        fig.savefig(filename)
        plt.close(fig) 

if __name__ == "__main__":
    main()
