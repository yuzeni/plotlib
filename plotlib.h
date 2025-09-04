#ifndef PLOTLIB_H
#define PLOTLIB_H

#include <stdint.h>

#define LIBTYPE_SHARED 1
#define PLOTLIB_MAX_PLOT_IDX (1024 - 1)
#define PLOTLIB_MAX_PLOT_GROUP_IDX (256 - 1)

#ifdef LIBTYPE_SHARED
    #define PLOTAPI __attribute__((visibility("default")))
#endif

#ifndef PLOTAPI
    #define PLOTAPI
#endif

#ifdef __cplusplus
extern "C" {
#endif

PLOTAPI void plotlib_show();
PLOTAPI void plotlib_mode_interactive();
PLOTAPI void plotlib_mode_show_n_points_of_tail(uint64_t points_count);
PLOTAPI void plotlib_mode_show_x_range_of_tail(double x_range);
PLOTAPI void plotlib_mode_fill_window();
PLOTAPI bool plotlib_mode_show_specific_plot(uint32_t plot_idx);
PLOTAPI void plotlib_clear_all_plots();
PLOTAPI void plotlib_enable_full_precision_display();

PLOTAPI bool plot_show(uint32_t plot_idx);
PLOTAPI bool plot_hide(uint32_t plot_idx);
PLOTAPI void plot_hide_all();
PLOTAPI bool plot_clear(uint32_t plot_idx);
PLOTAPI bool plot_set_color(uint32_t plot_idx, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
PLOTAPI bool plot_set_name(uint32_t plot_idx, const char* name);
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

#ifdef __cplusplus
}
#endif

#endif // PLOTLIB_H

