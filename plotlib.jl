
# API Improvements
# plotting into a thing should show that thing
# show help message in GUI when nothing else is displayed

module Plotlib

plotlib = ""
if Sys.iswindows()
    plotlib = "$(@__DIR__)\\libplotlib.dll"
else
    plotlib = "$(@__DIR__)/libplotlib.so"
end

const PLOTLIB_MAX_PLOT_IDX      = 1024 - 1
const PLOTLIB_MAX_PLOTTER_IDX   = 256 - 1
const PLOTLIB_MAX_PLOT3D_IDX    = 1024 - 1
const PLOTLIB_MAX_PLOTTER3D_IDX = 256 - 1
const PLOTLIB_MAX_PANEL_IDX     = 256 - 1

const DEFAULT_IDX = 0

struct Color
    r::UInt8
    g::UInt8
    b::UInt8
    a::UInt8
end

const RED           = Color(0xeb, 0x35, 0x45, 0xff)
const RED_LIGHT     = Color(0xec, 0x73, 0x8e, 0xff)
const RED_DARK      = Color(0x75, 0x28, 0x28, 0xff)
const GREEN         = Color(0x6a, 0xbd, 0x3c, 0xff)
const GREEN_LIGHT   = Color(0x95, 0xde, 0x85, 0xff)
const GREEN_DARK    = Color(0x4a, 0x6d, 0x22, 0xff)
const BLUE          = Color(0x5e, 0x6a, 0xea, 0xff)
const BLUE_LIGHT    = Color(0x9e, 0xbc, 0xde, 0xff)
const BLUE_DARK     = Color(0x39, 0x34, 0xa4, 0xff)
const ORANGE        = Color(0xf1, 0xa1, 0x29, 0xff)
const ORANGE_LIGHT  = Color(0xeb, 0xba, 0x6f, 0xff)
const ORANGE_DARK   = Color(0xc4, 0x60, 0x00, 0xff)
const YELLOW        = Color(0xe4, 0xe6, 0x5c, 0xff)
const YELLOW_LIGHT  = Color(0xfe, 0xff, 0xb2, 0xff)
const YELLOW_DARK   = Color(0xbf, 0xb6, 0x00, 0xff)
const PURPLE        = Color(0xb0, 0x4c, 0xe7, 0xff)
const PURPLE_LIGHT  = Color(0xc0, 0x92, 0xff, 0xff)
const PURPLE_DARK   = Color(0x69, 0x1c, 0xac, 0xff)
const WHITE         = Color(0xe9, 0xe9, 0xe9, 0xff)
const GREY          = Color(0x90, 0x90, 0x90, 0xff)
const BLACK         = Color(0x0c, 0x0c, 0x0c, 0xff)

@enum Number_Types::UInt8 begin
    FLOAT32 = 0
    FLOAT64 = 1
    INT32 = 2
    INT64 = 3
end

const Number_Types_Union = Union{Float32, Float64, Int32, Int64}

function _get_number_type(::Type{T}) where { T <: Number_Types_Union}
    if T == Float32 return FLOAT32
    elseif T == Float64 return FLOAT64
    elseif T == Int32 return INT32
    elseif T == Int64 return INT64
    end
end

# Gui

show() = @ccall plotlib.plotlib_show()::Cvoid
hide() = @ccall plotlib.plotlib_hide()::Cvoid

dark_theme()  = @ccall plotlib.plotlib_dark_theme()::Cvoid
light_theme() = @ccall plotlib.plotlib_light_theme()::Cvoid

"After calling this function, the first thing you 'touch' with your mouse that can become interactive will now be interactive."
interactive() = @ccall plotlib.plotlib_interactive()::Cvoid

# clear_all_plots() = @ccall plotlib.plotlib_clear_all_plots()::Cvoid


# Plot

plot(idx = DEFAULT_IDX) = (
    plot_idx = idx,
    
    show  = () -> (@ccall plotlib.plot_show(idx::UInt32)::Bool;  plot(idx)),
    hide  = () -> (@ccall plotlib.plot_hide(idx::UInt32)::Bool;  plot(idx)),
    clear = () -> (@ccall plotlib.plot_clear(idx::UInt32)::Bool; plot(idx)),
    
    set_color = (color::Color) -> (@ccall plotlib.plot_set_color(idx::UInt32, color.r::UInt8, color.g::UInt8, color.b::UInt8, color.a::UInt8)::Bool; plot(idx)),
    set_name  = (name::String) -> (@ccall plotlib.plot_set_name(idx::UInt32, name::Cstring)::Bool;                                                   plot(idx)),
    
    as_lines        = (line_width=1.0) -> (@ccall plotlib.plot_as_lines(idx::UInt32, line_width::Float64)::Bool;                                           plot(idx)),
    as_scatter      = (diameter=3.0) -> (@ccall plotlib.plot_as_scatter(idx::UInt32, diameter::Float64)::Bool;                                             plot(idx)),
    as_scatterlines = (line_width=1.0, diameter=3.0) -> (@ccall plotlib.plot_as_scatterlines(idx::UInt32, line_width::Float64, diameter::Float64)::Bool;   plot(idx)),

    fill_numbers = ((numbers::AbstractVector{NumberT}) where { NumberT <: Number_Types_Union }) -> begin
        @ccall plotlib.plot_fill_numbers(idx::UInt32, numbers::Ptr{Cvoid}, _get_number_type(NumberT)::UInt8, length(numbers)::UInt64)::Bool
        plot(idx)
    end,

    fill_points_x_y = ((points_x::AbstractVector{NumberT}, points_y::AbstractVector{NumberT}) where { NumberT <: Number_Types_Union }) -> begin
        if length(points_x) != length(points_y)
            println("PLOTLIB ERROR: The length of 'points_x' and 'points_y' must match.")
            return false
        end
        @ccall plotlib.plot_fill_points_x_y(idx::UInt32, points_x::Ptr{Cvoid}, points_y::Ptr{Cvoid}, _get_number_type(NumberT)::UInt8, length(points_y)::UInt64)::Bool
        plot(idx)
    end,

    fill_points_xy = ((points_xy::AbstractVector{NumberT}) where { NumberT <: Number_Types_Union }) -> begin
        @ccall plotlib.plot_fill_points_xy(idx::UInt32, points_xy::Ptr{Cvoid}, _get_number_type(NumberT)::UInt8, length(points_xy)::UInt64)::Bool
        plot(idx)
    end,

    append_number = (number) -> begin
        @ccall plotlib.plot_append_number(idx::UInt32, number::Float64)::Bool
        plot(idx)
    end,
    
    append_numbers = ((numbers::AbstractVector{NumberT}) where { NumberT <: Number_Types_Union }) -> begin
        @ccall plotlib.plot_append_numbers(idx::UInt32, numbers::Ptr{Cvoid}, _get_number_type(NumberT)::UInt8, length(numbers)::UInt64)::Bool
        plot(idx)
    end,

    append_point = (point_x, point_y) -> begin
        @ccall plotlib.plot_append_point(idx::UInt32, point_x::Float64, point_y::Float64)::Bool
        plot(idx)
    end,

    append_points_x_y = ((points_x::AbstractVector{NumberT}, points_y::AbstractVector{NumberT}) where { NumberT <: Number_Types_Union }) -> begin
        if length(points_x) != length(points_y)
            println("PLOTLIB ERROR: The length of 'points_x' and 'points_y' must match.")
            return false
        end
        @ccall plotlib.plot_append_points_x_y(idx::UInt32, points_x::Ptr{Cvoid}, points_y::Ptr{Cvoid}, _get_number_type(NumberT)::UInt8, length(points_y)::UInt64)::Bool
        plot(idx)
    end,

    append_points_xy = ((points_xy::AbstractVector{NumberT}) where { NumberT <: Number_Types_Union }) -> begin
        @ccall plotlib.plot_append_points_xy(idx::UInt32, points_xy::Ptr{Cvoid}, _get_number_type(NumberT)::UInt8, length(points_xy)::UInt64)::Bool
        plot(idx)
    end,
)

# Plotter

plotter(idx = DEFAULT_IDX) = (
    plotter_idx = idx,
    
    show  = () -> (@ccall plotlib.plotter_show(idx::UInt32)::Bool;  plotter(idx)),
    clear = () -> (@ccall plotlib.plotter_clear(idx::UInt32)::Bool; plotter(idx)),
    
    add_plot    = (plot_idx) -> (@ccall plotlib.plotter_add_plot(idx::UInt32, plot_idx::UInt32)::Bool;    plotter(idx)),
    remove_plot = (plot_idx) -> (@ccall plotlib.plotter_remove_plot(idx::UInt32, plot_idx::UInt32)::Bool; plotter(idx)),

    set_name = (name::String) -> (@ccall plotlib.plotter_set_name(idx::UInt32, name::Cstring)::Bool; plotter(idx)),
    
    track_latest          = (points_count) -> (@ccall plotlib.plotter_track_latest(idx::UInt32, points_count::UInt64)::Bool; plotter(idx)),
    track_latest_range_x  = (x_range) -> (@ccall plotlib.plotter_track_latest_range_x(idx::UInt32, x_range::Float64)::Bool; plotter(idx)),
    track_latest_range_xy = (x_range, y_range) -> (@ccall plotlib.plotter_track_latest_range_xy(idx::UInt32, x_range::Float64, y_range::Float64)::Bool; plotter(idx)),
    track_all             = () -> (@ccall plotlib.plotter_track_all(idx::UInt32)::Bool; plotter(idx)),
    track_specific_plot   = (plot_idx) -> (@ccall plotlib.plotter_track_specific_plot(idx::UInt32, plot_idx::UInt32)::Bool; plotter(idx)),
)

# Plot3d

plot3d(idx = DEFAULT_IDX) = (
    plot3d_idx = idx,
    
    show  = () -> (@ccall plotlib.plot3d_show(idx::UInt32)::Bool;  plot3d(idx)),
    hide  = () -> (@ccall plotlib.plot3d_hide(idx::UInt32)::Bool;  plot3d(idx)),
    clear = () -> (@ccall plotlib.plot3d_clear(idx::UInt32)::Bool; plot3d(idx)),
    
    set_name = (name::String) -> (@ccall plotlib.plot3d_set_name(idx::UInt32, name::Cstring)::Bool; plot3d(idx)),
    
    as_spheres         = (point_diameter) -> (@ccall plotlib.plot3d_as_spheres(idx::UInt32, point_diameter::Float32)::Bool; plot3d(idx)),
    as_lines           = () -> (@ccall plotlib.plot3d_as_lines(idx::UInt32)::Bool;                                          plot3d(idx)),
    as_triangles       = () -> (@ccall plotlib.plot3d_as_triangles(idx::UInt32)::Bool;                                      plot3d(idx)),
    as_continuous_line = () -> (@ccall plotlib.plot3d_as_continuous_line(idx::UInt32)::Bool;                                plot3d(idx)),
    
    append_vertex = (x, y, z) -> (@ccall plotlib.plot3d_append_vertex(idx::UInt32, x::Float32, y::Float32, z::Float32)::Bool; plot3d(idx)),

    append_vertex_with_color = (x, y, z, color::Color) -> begin
        @ccall plotlib.plot3d_append_vertex_with_color(plot3d_idx::UInt32, x::Float32, y::Float32, z::Float32,
                                                       color.r::UInt8, color.g::UInt8, color.b::UInt8, color.a::UInt8)::Bool
        plot3d(idx)
    end,

    fill_vertices_x_y_z = (vertices_x::Vector{Float32}, vertices_y::Vector{Float32}, vertices_z::Vector{Float32}) -> begin
        if length(vertices_x) != length(vertices_y) || length(vertices_x) != length(vertices_z)
            println("PLOTLIB ERROR: The length of 'vertices_x', 'vertices_y' and 'vertices_z' must match.")
            return false
        end
        @ccall plotlib.plot3d_fill_vertices_x_y_z(plot3d_idx::UInt32, vertices_x::Ptr{Float32}, vertices_y::Ptr{Float32}, vertices_z::Ptr{Float32},
                                                  length(vertices_x)::UInt64)::Bool
        plot3d(idx)
    end,

    rotate_quaternion = (i, j, k, real) -> begin
        @ccall plotlib.plot3d_rotate_quaternion(plot3d_idx::UInt32, i::Float32, j::Float32, k::Float32, real::Float32)::Bool
        plot3d(idx)
    end,

    set_orientation_quaternion = (i, j, k, real) -> begin
        @ccall plotlib.plot3d_set_orientation_quaternion(plot3d_idx::UInt32, i::Float32, j::Float32, k::Float32, real::Float32)::Bool
        plot3d(idx)
    end,

    move         = (x, y, z) -> (@ccall plotlib.plot3d_move(plot3d_idx::UInt32, x::Float32, y::Float32, z::Float32)::Bool; plot3d(idx)),
    set_position = (x, y, z) -> (@ccall plotlib.plot3d_set_position(plot3d_idx::UInt32, x::Float32, y::Float32, z::Float32)::Bool; plot3d(idx)),
)

# Plotter3d

plotter3d(idx = DEFAULT_IDX) = (
    plotter3d_idx = idx,
    
    show  = () -> (@ccall plotlib.plotter3d_show(idx::UInt32)::Bool;  plotter3d(idx)),
    clear = () -> (@ccall plotlib.plotter3d_clear(idx::UInt32)::Bool; plotter3d(idx)),
    
    add_plot3d    = (plot3d_idx) -> (@ccall plotlib.plotter3d_add_plot3d(idx::UInt32, plot3d_idx::UInt32)::Bool;    plotter3d(idx)),
    remove_plot3d = (plot3d_idx) -> (@ccall plotlib.plotter3d_remove_plot3d(idx::UInt32, plot3d_idx::UInt32)::Bool; plotter3d(idx)),
    
    set_name = (name::String) -> (@ccall plotlib.plotter3d_set_name(idx::UInt32, name::String)::Bool; plotter3d(idx)),
    
    perspective_projection = (FOV) -> (@ccall plotlib.plotter3d_perspective_projection(idx::UInt32, FOV::Float32)::Bool; plotter3d(idx)),
    camera_free            = () -> (@ccall plotlib.plotter3d_camera_free(idx::UInt32)::Bool;                             plotter3d(idx)),
    
    track_point               = (x, y, z) -> (@ccall plotlib.plotter3d_track_point(idx::UInt32, x::Float32, y::Float32, z::Float32)::Bool;    plotter3d(idx)),
    track_point_relative_to_plot3d = (plot3d_idx, x, y, z) -> begin
        @ccall plotlib.plotter3d_track_point_relative_to_plot3d(idx::UInt32, plot3d_idx::UInt32, x::Float32, y::Float32, z::Float32)::Bool
        plotter3d(idx)
    end,
    track_plot3d_midpoint      = (plot3d_idx) -> (@ccall plotlib.plotter3d_track_plot3d_midpoint(idx::UInt32, plot3d_idx::UInt32)::Bool;      plotter3d(idx)),
    track_plot3d_latest_vertex = (plot3d_idx) -> (@ccall plotlib.plotter3d_track_plot3d_latest_vertex(idx::UInt32, plot3d_idx::UInt32)::Bool; plotter3d(idx)),
)

# Panel

panel(idx = DEFAULT_IDX) = (
    panel_idx = idx,
    
    show  = () -> (@ccall plotlib.plotlibpanel_show(idx::UInt32)::Bool;  panel(idx)),
    clear = () -> (@ccall plotlib.plotlibpanel_clear(idx::UInt32)::Bool; panel(idx)),

    tile = (tile_idx = DEFAULT_IDX) -> (
        tile_idx = tile_idx,
        
        make_leftright = () -> (@ccall plotlib.plotlibpanel_add_leftright_tile(idx::UInt32, tile_idx::UInt32)::Bool; panel(idx)),
        make_topbottom = () -> (@ccall plotlib.plotlibpanel_add_topbottom_tile(idx::UInt32, tile_idx::UInt32)::Bool; panel(idx)),
        
        insert_plotter   = (plotter_idx) -> (@ccall plotlib.plotlibpanel_add_plotter(idx::UInt32, tile_idx::UInt32, plotter_idx::UInt32)::Bool;       panel(idx)),
        insert_plotter3d = (plotter3d_idx) -> (@ccall plotlib.plotlibpanel_add_plotter3d(idx::UInt32, tile_idx::UInt32, plotter3d_idx::UInt32)::Bool; panel(idx))
    ),
    
    remove_plotter   = (plotter_idx) -> (@ccall plotlib.plotlibpanel_remove_plotter(idx::UInt32, plotter_idx::UInt32)::Bool;       panel(idx)),
    remove_plotter3d = (plotter3d_idx) -> (@ccall plotlib.plotlibpanel_remove_plotter3d(idx::UInt32, plotter3d_idx::UInt32)::Bool; panel(idx)),
)

# The default `show()` for the named tuples of functions returned by `plot(x)` etc. is really unhelpful
# This just displays what `plot(x)` etc. stand for more abstractly (ie. the plot at index x).

Base.show(io::IO, plot_x::typeof(plot()))           = print(io, "Plot [$(plot_x.plot_idx)]")
Base.show(io::IO, plotter_x::typeof(plotter()))     = print(io, "Plotter [$(plotter_x.plotter_idx)]")
Base.show(io::IO, plot3d_x::typeof(plot3d()))       = print(io, "Plot3d [$(plot3d_x.plot3d_idx)]")
Base.show(io::IO, plotter3d_x::typeof(plotter3d())) = print(io, "Plotter3d [$(plotter3d_x.plotter3d_idx)]")
Base.show(io::IO, panel_x::typeof(panel()))         = print(io, "Panel [$(panel_x.panel_idx)]")
Base.show(io::IO, tile_x::typeof(panel().tile()))   = print(io, "Panel-Tile [$(tile_x.tile_idx)]")

# Gets called when this module (Plotlib) is loaded
function __init__()
    
    if Sys.islinux()
        global plotlib  = "$(@__DIR__)/libplotlib.so"
        plotlib_cpp     = "$(@__DIR__)/plotlib.cpp"
        
        requires_rebuild = true
        if !isfile(plotlib)
            @info("'libplotlib.so' not found in the folder which contains 'plotlib.jl', building from source with GCC (g++)...")
        elseif stat(plotlib).mtime < stat(plotlib_cpp).mtime
            @info("'libplotlib.so' older than 'plotlib.cpp' rebuilding from source with GCC (g++)...")
        else
            requires_rebuild = false
        end
        
        if requires_rebuild
            try
                # Change to directory of this file, build the shared library and go back to the original working directory
                working_dir = pwd()
                cd(@__DIR__)
                run(`gcc bin_to_strliteral.c -o bin_to_strliteral`)
                run(`./bin_to_strliteral --extern --ident gui_font_binary_ttf ClearSans-Regular.ttf gui_font_binary_ttf.cpp`)
                run(`g++ -std=c++20 -fPIC -fvisibility=hidden -ggdb -c -Wall -Wextra -pedantic -Wno-infinite-recursion -Wno-missing-field-initializers -shared plotlib.cpp gui_font_binary_ttf.cpp`)
                run(`g++ -shared -o libplotlib.so plotlib.o gui_font_binary_ttf.o -L"./raylib/" -lraylib_5_5_linux -lGL -lm -lpthread -ldl -lrt -lX11`)
                run(`rm bin_to_strliteral gui_font_binary_ttf.cpp gui_font_binary_ttf.o plotlib.o`)
                cd(working_dir)
            catch e
                @error("Failed to build `libplotlib.so`. Please include 'plotlib.jl' again, after resolving the error(s).")
            else
                global plotlib = "$(@__DIR__)/libplotlib.so"
                @assert(isfile(plotlib))
                @info("Successfully built 'libplotlib.so'!")
            end
        end
        
    elseif Sys.iswindows()
        global plotlib = "$(@__DIR__)\\libplotlib.dll"
        
        if !isfile(plotlib)
            error("'libplotlib.dll' not found in the folder which contains 'plotlib.jl', and also cannot be compiled
                  automatically because this needs to happen inside the 64-bit Visual Studio devloper shell.
                  You need to manually execute 'build_shared_lib.bat' there. Consult the 'README.md' for more details.");
        end
    else
        @error("Your operating system with kernel '$(Sys.KERNEL)' is not supported by Plotlib! Only Linux and Microsoft Windows are supported.")
    end
end

end # module Plotlib
