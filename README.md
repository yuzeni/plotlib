This is a stateful immediate-style plotting library.  
The GUI runs on its own thread so you can inspect your data live as it's being received and plotted.  
The API was designed for creating wrappers which are easy to use from a REPL.  

<div align="center">
<img src="https://github.com/yuzeni/plotlib/blob/main/demo_screenshot.png" alt="demo_screenshot" width="600"/>
</div>

### How to use with Julia

Build plotlib with GCC on Linux using `./build_shared_lib.sh`  
Or with MSVC on Windows by running `build_shared_lib.bat` from the Microsoft Visual Studio _"x64 Native Tools Command Prompt"_ or from any command prompt with the `vcvars64.bat` environment.  

For `plotlib.jl` to find the shared library they need to be in the same directory.  

Now start Julia and include `plotlib.jl`.

```julia
julia> include("path/to/plotlib.jl")
julia> Plotlib.show(4) # shows the Plot at index 4
julia> Plotlib.track_latest_values(1, 1)
julia> for theta in 0:0.001:6pi # draw a Kandinsky
           Plotlib.append_point(4, cos(theta*sin(theta)), sin(theta*cos(theta)))
           sleep(0.001) # 1ms
       end
julia> Plotlib.interactive() # Interactive mode lets you navigate the plot with your mouse
```

### The C-API for a quick overview

```C
PLOTAPI void plotlib_show();
PLOTAPI void plotlib_hide();
PLOTAPI void plotlib_dark_theme();
PLOTAPI void plotlib_light_theme();
PLOTAPI void plotlib_interactive();
PLOTAPI void plotlib_track_n_points_of_tail(uint64_t points_count);
PLOTAPI void plotlib_track_x_range_of_tail(double x_range);
PLOTAPI void plotlib_track_latest_values(double x_padding, double y_padding);
PLOTAPI void plotlib_track_all();
PLOTAPI bool plotlib_track_specific_plot(uint32_t plot_idx);
PLOTAPI void plotlib_clear_all_plots();
PLOTAPI void plotlib_clear_everything();

PLOTAPI bool plot_show(uint32_t plot_idx);
PLOTAPI bool plot_hide(uint32_t plot_idx);
PLOTAPI void plot_hide_all();
PLOTAPI bool plot_clear(uint32_t plot_idx);
PLOTAPI bool plot_set_color(uint32_t plot_idx, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
PLOTAPI bool plot_set_name(uint32_t plot_idx, const char* name);
PLOTAPI bool plot_as_lines(uint32_t plot_idx, double line_width);
PLOTAPI bool plot_as_scatter(uint32_t plot_idx, double point_diameter);
PLOTAPI bool plot_as_scatterlines(uint32_t plot_idx, double line_width, double point_diameter);
PLOTAPI bool plot_fill_numbers(uint32_t plot_idx, double* numbers, uint64_t length);
PLOTAPI bool plot_fill_points_x_y(uint32_t plot_idx, double* points_x, double* points_y, uint64_t length);
PLOTAPI bool plot_fill_points_xy(uint32_t plot_idx, double* points_xy, uint64_t length);
PLOTAPI bool plot_append_number(uint32_t plot_idx, double number);
PLOTAPI bool plot_append_numbers(uint32_t plot_idx, double* numbers, uint64_t length);
PLOTAPI bool plot_append_point(uint32_t plot_idx, double point_x, double point_y);
PLOTAPI bool plot_append_points_x_y(uint32_t plot_idx, double* points_x, double* points_y, uint64_t length);
PLOTAPI bool plot_append_points_xy(uint32_t plot_idx, double* points_xy, uint64_t length);

PLOTAPI bool plotgroup_show(uint32_t plotgroup_idx);
PLOTAPI bool plotgroup_append(uint32_t plotgroup_idx, uint32_t plot_idx);
PLOTAPI bool plotgroup_remove(uint32_t plotgroup_idx, uint32_t plot_idx);
PLOTAPI bool plotgroup_clear(uint32_t plotgroup_idx);
PLOTAPI bool plotgroup_set_name(uint32_t plotgroup_idx, const char* name);
```
