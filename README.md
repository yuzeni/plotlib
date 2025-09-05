This is a plotting library for fast interactive plotting.  
The GUI runs on its own thread so you can inspect your data live as it's being received and plotted.  
The API was designed to be convenient to use from a REPL.  

<div align="center">
<img src="https://github.com/yuzeni/plotlib/demo_screenshot.jpg" alt="demo_screenshot" width="600"/>
</div>

### How to use with Julia

First build the library by running `./build_shared_lib.sh` in your terminal.  
Then start Julia and include `plotlib.jl`. You might need to update the path to the shared library.

```julia
julia> include("path/to/plotlib.jl")
julia> Plotlib.plotlib = "path/to/libplotlib.so" # you don't need this if you started julia in this projects directory
julia> Plotlib.show(69) # shows the Plot at index 69
julia> for θ in 0:0.001:12π Plotlib.append_point(69, cos(θ*sin(θ)), sin(θ*cos(θ))) end # draw a kandinsky
julia> Plotlib.interactive() # Interactive mode lets you navigate the plot with your mouse
```

### This is the entire C-API

```C
void plotlib_show();
void plotlib_hide();
void plotlib_dark_theme();
void plotlib_light_theme();
void plotlib_mode_interactive();
void plotlib_mode_show_n_points_of_tail(uint64_t points_count);
void plotlib_mode_show_x_range_of_tail(double x_range);
void plotlib_mode_fill_window();
bool plotlib_mode_show_specific_plot(uint32_t plot_idx);
void plotlib_clear_all_plots();

bool plot_show(uint32_t plot_idx);
bool plot_hide(uint32_t plot_idx);
void plot_hide_all();
bool plot_clear(uint32_t plot_idx);
bool plot_set_color(uint32_t plot_idx, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
bool plot_set_name(uint32_t plot_idx, const char* name);
bool plot_as_lines(uint32_t plot_idx, double line_width);
bool plot_as_scatter(uint32_t plot_idx, double diameter);
bool plot_as_scatterlines(uint32_t plot_idx, double line_width, double diameter);
bool plot_fill_numbers(uint32_t plot_idx, double* numbers, uint64_t length);
bool plot_fill_points_x_y(uint32_t plot_idx, double* points_x, double* points_y, uint64_t length);
bool plot_fill_points_xy(uint32_t plot_idx, double* points_xy, uint64_t length);
bool plot_append_number(uint32_t plot_idx, double number);
bool plot_append_numbers(uint32_t plot_idx, double* numbers, uint64_t length);
bool plot_append_point(uint32_t plot_idx, double point_x, double point_y);
bool plot_append_points_x_y(uint32_t plot_idx, double* points_x, double* points_y, uint64_t length);
bool plot_append_points_xy(uint32_t plot_idx, double* points_xy, uint64_t length);
    
bool plotgroup_show(uint32_t plotgroup_idx);
bool plotgroup_append(uint32_t plotgroup_idx, uint32_t plot_idx);
bool plotgroup_remove(uint32_t plotgroup_idx, uint32_t plot_idx);
bool plotgroup_clear(uint32_t plotgroup_idx);
bool plotgroup_set_name(uint32_t plotgroup_idx, const char* name);
```
