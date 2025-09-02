module Plotlib

plotlib = "./libplotlib.so"

const MAX_PLOT_IDX = 1024 - 1
const MAX_PLOT_GROUP_IDX = 256 - 1

function show()::Nothing
    @ccall plotlib.plotlib_show()::Cvoid
end

function mode_interactive()::Nothing
    @ccall plotlib.plotlib_mode_interactive()::Cvoid;
end

function mode_show_n_points_of_tail(points_count)::Nothing
    @ccall plotlib.plotlib_mode_show_n_points_of_tail(points_count::UInt64)::Cvoid;
end

function mode_show_x_range_of_tail(x_range)::Nothing
    @ccall plotlib.plotlib_mode_show_x_range_of_tail(x_range::Float64)::Cvoid;
end

function mode_fill_window()::Nothing
    @ccall plotlib.plotlib_mode_fill_window()::Cvoid;
end

function mode_show_specific_plot(plot_idx)::Bool
    @ccall plotlib.plotlib_mode_show_specific_plot(plot_idx::UInt32)::Bool
end

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

function plot_clear(plot_idx)::Bool
    @ccall plotlib.plot_clear(plot_idx::UInt32)::Bool
end

function set_plot_color(plot_idx, r, g, b, a)::Bool
    @ccall plotlib.plot_set_color(plot_idx::UInt32, r::UInt8, g::UInt8, b::UInt8, a::UInt8)::Bool
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

function group_append(plotgroup_idx, plot_idx)::Bool
    @ccall plotlib.plotgroup_append(plotgroup_idx::UInt32, plot_idx::UInt32)::Bool
end

function group_remove(plotgroup_idx, plot_idx)::Bool
    @ccall plotlib.plotgroup_remove(plotgroup_idx::UInt32, plot_idx::UInt32)::Bool
end

function clear_group(plotgroup_idx)::Bool
    @ccall plotlib.plotgroup_clear(plotgroup_idx::UInt32)::Bool
end

function set_group_name(plotgroup_idx, name::String)::Bool
    @ccall plotlib.plotgroup_set_name(plotgroup_idx::UInt32, name::Cstring)::Bool
end

end # module Plotlib
