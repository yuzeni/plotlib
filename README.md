This is a plotting library for fast interactive plotting.  
The GUI runs on its own thread so you can inspect your data live as it's being received and plotted.  
The API was designed to be convenient to use from a REPL.  

### How to use with Julia

First build the library by running `./build_shared_lib.sh` in your terminal.  
Then start Julia and include `plotlib.jl`. You might need to update the path to the shared library.

```julia
julia> include("path/to/plotlib.jl")
julia> Plotlib.plotlib = "path/to/libplotlib.so" # you don't need this if you started julia in this projects directory
julia> Plotlib.show_plot(69) # shows the Plot at index 69
julia> for θ in 0:0.001:12π Plotlib.plot_append_point(69, cos(θ*sin(θ)), sin(θ*cos(θ))) end # draw a kandinsky
julia> Plotlib.mode_interactive() # Interactive mode lets you navigate the plot with your mouse
```

Please have a look at `plotlib.jl` or `plotlib.h` to get a grasp of the functionality.
