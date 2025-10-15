#ifndef PLOTLIB_H
#define PLOTLIB_H

#include <stdint.h>

#define PLOTLIB_MAX_PLOT_IDX (1024 - 1)
#define PLOTLIB_MAX_PLOTTER_IDX (256 - 1)
#define PLOTLIB_MAX_PLOT3D_IDX (1024 - 1)
#define PLOTLIB_MAX_PLOTTER3D_IDX (256 - 1)
#define PLOTLIB_MAX_PANEL_IDX (256 - 1)

#define LIBTYPE_SHARED 1

#ifdef LIBTYPE_SHARED
    #ifdef _WIN32
        #define PLOTAPI __declspec(dllexport)
    #else
        #define PLOTAPI __attribute__((visibility("default")))
    #endif
#endif

#ifndef PLOTAPI
    #define PLOTAPI
#endif

#ifdef __cplusplus
extern "C" {
#endif

PLOTAPI void plotlib_show();
PLOTAPI void plotlib_hide();
PLOTAPI void plotlib_dark_theme();
PLOTAPI void plotlib_light_theme();
PLOTAPI void plotlib_clear_all_plots();
PLOTAPI void plotlib_interactive();
/* PLOTAPI void plotlib_reset(); */
/* PLOTAPI void plotlib_set_gui_scaling(double scaling); */

enum {
    PLOT_FLOAT32,
    PLOT_FLOAT64,
    PLOT_INT32,
    PLOT_INT64,
};
typedef uint8_t Number_Types;
    
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
/* PLOTAPI uint64_t plot_read_numbers(uint32_t plot_idx, double* numbers, uint64_t length); */
/* PLOTAPI uint64_t plot_read_points_x_y(uint32_t plot_idx, double* points_x, double* points_y, uint64_t length); */
/* PLOTAPI uint64_t plot_read_points_xy(uint32_t plot_idx, double* points_xy, uint64_t length); */

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
/* PLOTAPI bool plotter_xscale_log(uint32_t plotter_idx, double base); */
/* PLOTAPI bool plotter_xscale_sqrt(uint32_t plotter_idx); */
/* PLOTAPI bool plotter_xscale_identity(uint32_t plotter_idx); */
/* PLOTAPI bool plotter_yscale_log(uint32_t plotter_idx, double base); */
/* PLOTAPI bool plotter_yscale_sqrt(uint32_t plotter_idx); */
/* PLOTAPI bool plotter_yscale_identity(uint32_t plotter_idx); */
/* PLOTAPI bool plotter_set_xaxis_label(uint32_t plotter_idx, const char* label); */
/* PLOTAPI bool plotter_set_yaxis_label(uint32_t plotter_idx, const char* label); */
/* PLOTAPI bool plotter_set_xaxis_unit(uint32_t plotter_idx, const char* unit); */
/* PLOTAPI bool plotter_set_yaxis_unit(uint32_t plotter_idx, const char* unit); */

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
/* PLOTAPI bool plot3d_set_indices(uint32_t plot3d_idx, uint32_t* indices, uint64_t length); */
/* PLOTAPI bool plot3d_load_from_file(uint32_t plot3d_idx, const char* file_path); */

PLOTAPI bool plotter3d_show(uint32_t plotter3d_idx);
PLOTAPI bool plotter3d_add_plot3d(uint32_t plotter3d_idx, uint32_t plot3d_idx);
PLOTAPI bool plotter3d_remove_plot3d(uint32_t plotter3d_idx, uint32_t plot3d_idx);
PLOTAPI bool plotter3d_clear(uint32_t plotter3d_idx);
PLOTAPI bool plotter3d_set_name(uint32_t plotter3d_idx, const char* name);
PLOTAPI bool plotter3d_perspective_projection(uint32_t plotter3d_idx, float FOV);
/* PLOTAPI bool plotter3d_orthogonal_projection(uint32_t plotter3d_idx); */
/* PLOTAPI bool plotter3d_set_camera_center(uint32_t plotter3d_idx, float x, float y, float z); */
/* PLOTAPI bool plotter3d_set_camera_orientation_quat(uint32_t plotter3d_idx, double i, double j, double k, double real); */
/* PLOTAPI bool plotter3d_first_person(uint32_t plotter3d_idx); */
/* PLOTAPI bool plotter3d_third_person(uint32_t plotter3d_idx); */
PLOTAPI bool plotter3d_camera_free(uint32_t plotter3d_idx);
PLOTAPI bool plotter3d_track_point(uint32_t plotter3d_idx, float x, float y, float z);
PLOTAPI bool plotter3d_track_point_relative_to_plot3d(uint32_t plotter3d_idx, uint32_t plot3d_idx, float x, float y, float z);
PLOTAPI bool plotter3d_track_plot3d_midpoint(uint32_t plotter3d_idx, uint32_t plot3d_idx);
PLOTAPI bool plotter3d_track_plot3d_latest_vertex(uint32_t plotter3d_idx, uint32_t plot3d_idx);
    
PLOTAPI bool plotlibpanel_show(uint32_t panel_idx);
PLOTAPI bool plotlibpanel_add_leftright_tile(uint32_t panel_idx, uint32_t tile_idx);
PLOTAPI bool plotlibpanel_add_topbottom_tile(uint32_t panel_idx, uint32_t tile_idx);
/* PLOTAPI bool plotlibpanel_add_grid(uint32_t panel_idx, uint32_t tile_idx, uint32_t grid_count_x, uint32_t grid_count_y); */
/* TODO: remove-tile is useless, because the user doesn't know the tile idx */ PLOTAPI bool plotlibpanel_remove_tile(uint32_t panel_idx, uint32_t tile_idx);
PLOTAPI bool plotlibpanel_add_plotter(uint32_t panel_idx, uint32_t tile_idx, uint32_t plotter_idx);
PLOTAPI bool plotlibpanel_add_plotter3d(uint32_t panel_idx, uint32_t tile_idx, uint32_t plotter3d_idx);
PLOTAPI bool plotlibpanel_remove_plotter(uint32_t panel_idx, uint32_t plotter_idx);
PLOTAPI bool plotlibpanel_remove_plotter3d(uint32_t panel_idx, uint32_t plotter3d_idx);
PLOTAPI bool plotlibpanel_clear(uint32_t panel_idx);

#ifdef __cplusplus
}
#endif

#endif // PLOTLIB_H

