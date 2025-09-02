This is a plotting library for fast interactive plotting.  
The GUI runs in its own thread so you can inspect your data live as it's being received and plotted.  
The API was designed to be convenient to use from a REPL.  

### How to use with Julia

First build the library by running `./build_shared_lib.sh` in a terminal.  
Then start Julia and include `plotlib.jl`. You might need to update the path to the shared library.

```julia
julia> include("path/to/the/plotlib.jl")
julia> Plotlib.plotlib = "path/to/the/libplotlib.so" # you don't need this if you started julia in this projects directory
julia> Plotlib.show_plot(69) # shows the Plot at index 69
julia> Plotlib.plot_fill_points_x_y(69, [cos(x) for x in 0:0.01:pi*2], [sin(x) for x in 0:0.01:pi*2]) # draws a circle
```

Please have a look at `plotlib.jl` or `plotlib.h` to get a grasp of the functionality.
