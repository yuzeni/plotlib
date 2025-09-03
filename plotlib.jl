module Plotlib

plotlib = "./libplotlib.so"

const MAX_PLOT_IDX = 1024 - 1
const MAX_PLOT_GROUP_IDX = 256 - 1

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

"Shows the GUI."
function show()::Nothing
    @ccall plotlib.plotlib_show()::Cvoid
end

"""
Enables zooming and paning in the GUI.
Use Left-CTRL + Mouse-Wheel for vertical zooming
Use Left-Schift + Mouse-Wheel for horizontal zooming
"""
function mode_interactive()::Nothing
    @ccall plotlib.plotlib_mode_interactive()::Cvoid;
end

"Shows the last n points/numbers of the plot."
function mode_show_n_points_of_tail(points_count)::Nothing
    @ccall plotlib.plotlib_mode_show_n_points_of_tail(points_count::UInt64)::Cvoid;
end

"Constrains the length of the x-axis to 'x_range' and shows the right-most part of the plot."
function mode_show_x_range_of_tail(x_range)::Nothing
    @ccall plotlib.plotlib_mode_show_x_range_of_tail(x_range::Float64)::Cvoid;
end

"Fills the window with all plots which are currently displayed."
function mode_fill_window()::Nothing
    @ccall plotlib.plotlib_mode_fill_window()::Cvoid;
end

"Fills the window with a specific plot, it doesn't need to be visible."
function mode_show_specific_plot(plot_idx)::Bool
    @ccall plotlib.plotlib_mode_show_specific_plot(plot_idx::UInt32)::Bool
end

"Deletes all strored points/numbers in all plots."
function clear_all_plots()::Nothing
    @ccall plotlib.plotlib_clear_all_plots()::Cvoid;
end

function show_plot(plot_idx)::Bool
    @ccall plotlib.plot_show(plot_idx::UInt32)::Bool
end

function hide_plot(plot_idx)::Bool
    @ccall plotlib.plot_hide(plot_idx::UInt32)::Bool
end

function hide_all_plots()::Nothing
    @ccall plotlib.plot_hide_all()::Cvoid
end

"Deletes all strored points/numbers in the plot."
function clear_plot(plot_idx)::Bool
    @ccall plotlib.plot_clear(plot_idx::UInt32)::Bool
end

function set_plot_color(plot_idx, color::Color)::Bool
    @ccall plotlib.plot_set_color(plot_idx::UInt32, color.r::UInt8, color.g::UInt8, color.b::UInt8, color.a::UInt8)::Bool
end

function set_plot_name(plot_idx, name::String)::Bool
    @ccall plotlib.plot_set_name(plot_idx::UInt32, name::Cstring)::Bool
end

function plot_fill_numbers(plot_idx, numbers::Vector{Float64})::Bool
    @ccall plotlib.plot_fill_numbers(plot_idx::UInt32, numbers::Ptr{Float64}, length(numbers)::UInt64)::Bool
end

function plot_fill_points_x_y(plot_idx, points_x::Vector{Float64}, points_y::Vector{Float64})::Bool
    if length(points_x) != length(points_y)
        println("PLOTLIB ERROR: The length of 'points_x' and 'points_y' must match.")
        return false
    end
    return @ccall plotlib.plot_fill_points_x_y(plot_idx::UInt32, points_x::Ptr{Float64}, points_y::Ptr{Float64}, length(points_y)::UInt64)::Bool
end

function plot_fill_points_xy(plot_idx, points_xy::Vector{Float64})::Bool
    @ccall plotlib.plot_fill_points_xy(plot_idx::UInt32, points_xy::Ptr{Float64}, length(points_xy)::UInt64)::Bool
end

function plot_append_number(plot_idx, number)::Bool
    @ccall plotlib.plot_append_number(plot_idx::UInt32, number::Float64)::Bool
end

function plot_append_numbers(plot_idx, numbers::Vector{Float64})::Bool
    @ccall plotlib.plot_append_numbers(plot_idx::UInt32, numbers::Ptr{Float64}, length(numbers)::UInt64)::Bool
end

function plot_append_point(plot_idx, point_x, point_y)::Bool
    @ccall plotlib.plot_append_point(plot_idx::UInt32, point_x::Float64, point_y::Float64)::Bool
end

function plot_append_points_x_y(plot_idx, points_x::Vector{Float64}, points_y::Vector{Float64}, length::UInt64)::Bool
    if length(points_x) != length(points_y)
        println("PLOTLIB ERROR: The length of 'points_x' and 'points_y' must match.")
        return false
    end
    @ccall plotlib.plot_append_points_x_y(plot_idx::UInt32, points_x::Ptr{Float64}, points_y::Ptr{Float64}, length(points_y)::UInt64)::Bool
end

function plot_append_points_xy(plot_idx, points_xy::Vector{Float64})::Bool
    @ccall plotlib.plot_append_points_xy(plot_idx::UInt32, points_xy::Ptr{Float64}, length(points_xy)::UInt64)::Bool
end

function show_group(plotgroup_idx)::Bool
    @ccall plotlib.plotgroup_show(plotgroup_idx::UInt32)::Bool
end

"Adds the plot to the group."
function group_append(plotgroup_idx, plot_idx)::Bool
    @ccall plotlib.plotgroup_append(plotgroup_idx::UInt32, plot_idx::UInt32)::Bool
end

"Removes the plot from the group."
function group_remove(plotgroup_idx, plot_idx)::Bool
    @ccall plotlib.plotgroup_remove(plotgroup_idx::UInt32, plot_idx::UInt32)::Bool
end

"Removes all plots from the group."
function clear_group(plotgroup_idx)::Bool
    @ccall plotlib.plotgroup_clear(plotgroup_idx::UInt32)::Bool
end

function set_group_name(plotgroup_idx, name::String)::Bool
    @ccall plotlib.plotgroup_set_name(plotgroup_idx::UInt32, name::Cstring)::Bool
end

end # module Plotlib
