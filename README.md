This is a stateful plotting library.  
The GUI runs on its own thread so you can inspect your data live as it's being received and plotted.  
The API was designed for creating wrappers which are easy to use from a REPL.  

<div align="center">
<img src="https://github.com/yuzeni/plotlib/blob/main/demo_screenshot.png" alt="demo_screenshot" width="600"/>
</div>

### Building on linux with GCC

Run `./build_shared_lib.sh` in your terminal. If it doesn't work have a look at the dependencies below.

##### Dependencies

1. GCC (c++ compiler)
2. OpenGL (libGL) (for example libgl-dev package on Ubuntu/Debian)
3. X11 (libX11) (for example libx11-dev package on Ubuntu/Debian)

### Building on Windows with MSVC

Run `build_shared_lib.bat` from the Microsoft Visual Studio _"x64 Native Tools Command Prompt"_ or from any command prompt with the `vcvars64.bat` environment.

##### Dependencies

1. Microsoft Visual Studio

### How to use with Julia

Make sure you have a `libplotlib.so`/`libplotlib.dll` in the same directory as `plotlib.jl`.  
Start Julia and include `plotlib.jl` (including works from any directory).

```julia
julia> include("path/to/plotlib.jl")
julia> Plotlib.plot(4).show()
julia> Plotlib.plotter().track_latest_range_xy(1, 1)
julia> for theta in 0:0.001:6pi # draw a Kandinsky
           Plotlib.plot(4).append_point(cos(theta*sin(theta)), sin(theta*cos(theta)))
           sleep(0.001) # 1ms
       end
julia> Plotlib.interactive() # Interactive mode lets you navigate the plot with your mouse
```

### The C-API

```C

PLOTAPI void plotlib_show();
PLOTAPI void plotlib_hide();
PLOTAPI void plotlib_dark_theme();
PLOTAPI void plotlib_light_theme();
PLOTAPI void plotlib_clear_all_plots();
PLOTAPI void plotlib_interactive();

PLOTAPI bool plot_show(uint32_t plot_idx);
PLOTAPI bool plot_hide(uint32_t plot_idx);
PLOTAPI bool plot_clear(uint32_t plot_idx);
PLOTAPI bool plot_set_color(uint32_t plot_idx, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
PLOTAPI bool plot_set_name(uint32_t plot_idx, const char* name);
PLOTAPI bool plot_as_lines(uint32_t plot_idx, double line_width);
PLOTAPI bool plot_as_scatter(uint32_t plot_idx, double point_diameter);
PLOTAPI bool plot_as_scatterlines(uint32_t plot_idx, double line_width, double point_diameter);
PLOTAPI bool plot_fill_numbers(uint32_t plot_idx, void* numbers, Number_Types num_type, uint64_t count);
PLOTAPI bool plot_fill_points_x_y(uint32_t plot_idx, void* points_x, void* points_y, Number_Types num_type, uint64_t count);
PLOTAPI bool plot_fill_points_xy(uint32_t plot_idx, void* points_xy, Number_Types num_type, uint64_t count);
PLOTAPI bool plot_append_number(uint32_t plot_idx, double number);
PLOTAPI bool plot_append_numbers(uint32_t plot_idx, void* numbers, Number_Types num_type, uint64_t count);
PLOTAPI bool plot_append_point(uint32_t plot_idx, double point_x, double point_y);
PLOTAPI bool plot_append_points_x_y(uint32_t plot_idx, void* points_x, void* points_y, Number_Types num_type, uint64_t count);
PLOTAPI bool plot_append_points_xy(uint32_t plot_idx, void* points_xy, Number_Types num_type, uint64_t count);
PLOTAPI uint64_t plot_get_length(uint32_t plot_idx);

PLOTAPI bool plotter_show(uint32_t plotter_idx);
PLOTAPI bool plotter_add_plot(uint32_t plotter_idx, uint32_t plot_idx);
PLOTAPI bool plotter_remove_plot(uint32_t plotter_idx, uint32_t plot_idx);
PLOTAPI bool plotter_clear(uint32_t plotter_idx);
PLOTAPI bool plotter_set_name(uint32_t plotter_idx, const char* name);
PLOTAPI bool plotter_track_latest(uint32_t plotter_idx, uint64_t points_count);
PLOTAPI bool plotter_track_latest_range_x(uint32_t plotter_idx, double x_range);
PLOTAPI bool plotter_track_latest_range_xy(uint32_t plotter_idx, double x_range, double y_range);
PLOTAPI bool plotter_track_all(uint32_t plotter_idx);
PLOTAPI bool plotter_track_specific_plot(uint32_t plotter_idx, uint32_t plot_idx);

PLOTAPI bool plot3d_show(uint32_t plot3d_idx);
PLOTAPI bool plot3d_hide(uint32_t plot3d_idx);
PLOTAPI bool plot3d_clear(uint32_t plot3d_idx);
PLOTAPI bool plot3d_set_name(uint32_t plot3d_idx, const char* name);
PLOTAPI bool plot3d_as_spheres(uint32_t plot3d_idx, float point_diameter);
PLOTAPI bool plot3d_as_lines(uint32_t plot3d_idx);
PLOTAPI bool plot3d_as_triangles(uint32_t plot3d_idx);
PLOTAPI bool plot3d_as_continuous_line(uint32_t plot3d_idx);
PLOTAPI bool plot3d_append_vertex(uint32_t plot3d_idx, float x, float y, float z);
PLOTAPI bool plot3d_append_vertex_with_color(uint32_t plot3d_idx, float x, float y, float z, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
PLOTAPI bool plot3d_fill_vertices_x_y_z(uint32_t plot3d_idx, float* x, float* y, float* z, uint64_t length);
PLOTAPI bool plot3d_rotate_quaternion(uint32_t plot3d_idx, float i, float j, float k, float real);
PLOTAPI bool plot3d_set_orientation_quaternion(uint32_t plot3d_idx, float i, float j, float k, float real);
PLOTAPI bool plot3d_move(uint32_t plot3d_idx, float x, float y, float z);
PLOTAPI bool plot3d_set_position(uint32_t plot3d_idx, float x, float y, float z);

PLOTAPI bool plotter3d_show(uint32_t plotter3d_idx);
PLOTAPI bool plotter3d_add_plot3d(uint32_t plotter3d_idx, uint32_t plot3d_idx);
PLOTAPI bool plotter3d_remove_plot3d(uint32_t plotter3d_idx, uint32_t plot3d_idx);
PLOTAPI bool plotter3d_clear(uint32_t plotter3d_idx);
PLOTAPI bool plotter3d_set_name(uint32_t plotter3d_idx, const char* name);
PLOTAPI bool plotter3d_perspective_projection(uint32_t plotter3d_idx, float FOV);
PLOTAPI bool plotter3d_camera_free(uint32_t plotter3d_idx);
PLOTAPI bool plotter3d_track_point(uint32_t plotter3d_idx, float x, float y, float z);
PLOTAPI bool plotter3d_track_point_relative_to_plot3d(uint32_t plotter3d_idx, uint32_t plot3d_idx, float x, float y, float z);
PLOTAPI bool plotter3d_track_plot3d_midpoint(uint32_t plotter3d_idx, uint32_t plot3d_idx);
PLOTAPI bool plotter3d_track_plot3d_latest_vertex(uint32_t plotter3d_idx, uint32_t plot3d_idx);
    
PLOTAPI bool plotlibpanel_show(uint32_t panel_idx);
PLOTAPI bool plotlibpanel_add_leftright_tile(uint32_t panel_idx, uint32_t tile_idx);
PLOTAPI bool plotlibpanel_add_topbottom_tile(uint32_t panel_idx, uint32_t tile_idx);
PLOTAPI bool plotlibpanel_add_plotter(uint32_t panel_idx, uint32_t tile_idx, uint32_t plotter_idx);
PLOTAPI bool plotlibpanel_add_plotter3d(uint32_t panel_idx, uint32_t tile_idx, uint32_t plotter3d_idx);
PLOTAPI bool plotlibpanel_remove_plotter(uint32_t panel_idx, uint32_t plotter_idx);
PLOTAPI bool plotlibpanel_remove_plotter3d(uint32_t panel_idx, uint32_t plotter3d_idx);
PLOTAPI bool plotlibpanel_clear(uint32_t panel_idx);
```
