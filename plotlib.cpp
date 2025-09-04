#include "plotlib.h"

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cmath>

#include <limits>
#include <vector>
#include <mutex>
#include <thread>

namespace rl {
#include "raylib/raylib.h"
}

#define DEFAULT_FPS 120
#define DEFAULT_WINDOW_WIDTH 650
#define DEFAULT_WINDOW_HEIGHT 500
#define MAX_TICK_MARK_TEXT_LENGTH 30
#define MAX_TICK_MARK_COUNT 32
#define SCROLL_ZOOM_FACTOR 0.5
#define EXCESSIVE_TRAILING_ZEROS_THRESHOLD 3 // replace traling zeros for string representations of numbers with '+eX'
#define DEFAULT_ZOOM_TO_ZERO_PIXEL_DISTANCE_THRESHOLD 20
#define MIN_PLOT_SCREEN_TO_BOUNDS_OFFSET 8
#define DEFAULT_TICK_MARK_LEN 5

#define MAX_DOUBLE 1.7976931348623157e308
#define MAX_PLOTRANGE_VALUE 1e300
#define MIN_PLOTRANGE_VALUE 1e-300
#define PRECISION_SAFTEY_FACTOR 100.0

#define INFO "PLOTLIB INFO: "
#define WARNING "PLOTLIB WARNING: "
#define ERROR "PLOTLIB ERROR: "

#define INVALID_IDX ((uint32_t)~0) // Max uint32 value

#define MAX_PLOT_SIZE (PLOTLIB_MAX_PLOT_IDX + 1)
#define MAX_PLOT_GROUP_SIZE (PLOTLIB_MAX_PLOT_GROUP_IDX + 2)
#define DEFAULT_PLOT_GROUP_IDX (PLOTLIB_MAX_PLOT_GROUP_IDX + 1)

extern const unsigned char gui_font_binary_ttf[];
extern const unsigned int gui_font_binary_ttf_len;

typedef uint32_t Plot_IDX;
typedef uint32_t Group_IDX;

static bool valid_plot_idx(Plot_IDX plot_idx) {
    if (plot_idx < MAX_PLOT_SIZE) {
        return true;
    }
    printf(ERROR "The plot index '%u' is not within the valid fixed range of [0, %d]\n", plot_idx, PLOTLIB_MAX_PLOT_IDX);
    return false;
}

static bool valid_group_idx(Group_IDX group_idx) {
    if (group_idx <= PLOTLIB_MAX_PLOT_GROUP_IDX) {
        return true;
    }
    printf(ERROR "The plot group index '%u' is not within the valid fixed range of [0, %d]\n", group_idx, PLOTLIB_MAX_PLOT_GROUP_IDX);
    return false;
}

struct Numbers {
    uint64_t length = 0;
    double* y = nullptr;
};

struct Point {
    double x = 0, y = 0;
};

struct Points_X_Y {
    uint64_t length = 0;
    double* x = nullptr, *y = nullptr;
};

struct Points_XY {
    uint64_t length= 0;
    double* xy = nullptr;
};

struct Range_XY {
    double x_begin = 0, x_end = 0, y_begin = 0, y_end = 0;
};

struct Color {
    uint8_t r = 255, g = 255, b = 255, a = 255;
};

static const Color plot_color_table[] = {
    Color{0xe9, 0xe9, 0xe9, 0xff}, // WHITE
    Color{0xeb, 0x35, 0x45, 0xff}, // RED
    Color{0x6a, 0xbd, 0x3c, 0xff}, // GREEN
    Color{0x5e, 0x6a, 0xea, 0xff}, // BLUE
    Color{0xf1, 0xa1, 0x29, 0xff}, // ORANGE
    Color{0xe4, 0xe6, 0x5c, 0xff}, // YELLOW
    Color{0xb0, 0x4c, 0xe7, 0xff}, // PURPLE
    Color{0xec, 0x73, 0x8e, 0xff}, // RED_LIGHT
    Color{0x95, 0xde, 0x85, 0xff}, // GREEN_LIGHT
    Color{0x9e, 0xbc, 0xde, 0xff}, // BLUE_LIGHT
    Color{0xeb, 0xba, 0x6f, 0xff}, // ORANGE_LIGHT
    Color{0xfe, 0xff, 0xb2, 0xff}, // YELLOW_LIGHT
    Color{0xc0, 0x92, 0xff, 0xff}, // PURPLE_LIGHT
    Color{0x75, 0x28, 0x28, 0xff}, // RED_DARK
    Color{0x4a, 0x6d, 0x22, 0xff}, // GREEN_DARK
    Color{0x39, 0x34, 0xa4, 0xff}, // BLUE_DARK
    Color{0xc4, 0x60, 0x00, 0xff}, // ORANGE_DARK
    Color{0xbf, 0xb6, 0x00, 0xff}, // YELLOW_DARK
    Color{0x69, 0x1c, 0xac, 0xff}, // PURPLE_DARK
};

static const size_t plot_color_table_size = sizeof(plot_color_table) / sizeof(Color);

static rl::Color to_rl_color(Color color) {
    return rl::Color{ color.r, color.g, color.b, color.a };
}

struct Plot {
    std::vector<double> points_x;
    std::vector<double> points_y;

    Color color;
    bool initialized = false;
    
    char* label = nullptr;
    
    Range_XY bb = { MAX_PLOTRANGE_VALUE, -MAX_PLOTRANGE_VALUE, MAX_PLOTRANGE_VALUE, -MAX_PLOTRANGE_VALUE }; // bounding box
    
    bool has_x_coordinate() { return !points_x.empty() && points_x.size() == points_y.size(); }
    bool empty() { return points_x.empty() && points_y.empty(); }
};

struct Plot_Update {
    std::vector<double> new_points_x;
    std::vector<double> new_points_y;


    bool has_custom_color = false;
    Color custom_color;
    char* new_name = nullptr;

    bool empty_update = true; // true -> safe to skip the update
    bool was_cleared = false;

    bool contains_points = false;
    bool contains_numbers = false;

    bool accepts_numbers() { return contains_numbers || !contains_points; }
    bool accepts_points() { return contains_points || !contains_numbers; }
    
    // Resets everything which was just a delta and not a mirror of the actual state.
    void reset() {
        new_points_x.clear();
        new_points_y.clear();
        has_custom_color = false;
        
        delete new_name;
        new_name = nullptr;
        
        was_cleared = false;
        empty_update = true;
    }

    void clear_plot() {
        new_points_x.clear();
        new_points_y.clear();
        contains_points = false;
        contains_numbers = false;
        was_cleared = true;
        empty_update = false;
    }
};

struct Plot_Group {
    std::vector<Plot_IDX> plots;
    char* label = nullptr;
    bool initialized = false;
};

struct Plot_Group_Update {
    std::vector<Plot_IDX> new_plots;
    std::vector<Plot_IDX> remove_plots;
    
    char* new_name = nullptr;
    
    bool empty_update = true;
    bool was_cleared = false;

    void reset() {
        new_plots.clear();
        remove_plots.clear();

        delete new_name;
        new_name = nullptr;
        
        was_cleared = false;
        empty_update = true;
    }

    void clear_group() {
        new_plots.clear();
        remove_plots.clear();
        was_cleared = true;
        empty_update = false;
    }
};

struct Visualization_Mode {
    enum : uint32_t {
        NONE,
        INTERACTIVE,
        SHOW_N_POINTS_OF_TAIL,
        SHOW_X_RANGE_OF_TAIL,
        SHOW_ENTIRE_PLOT_GROUP,
        SHOW_SPECIFIC_PLOT,
    };
    uint32_t type = NONE;
    uint64_t n_points = 0;
    double x_range = 0;
    Plot_IDX specific_plot = INVALID_IDX;
};

struct Plot_State {
    Plot plots[MAX_PLOT_SIZE];
    Plot_Group plot_groups[MAX_PLOT_GROUP_SIZE];
    
    Group_IDX visible_group = DEFAULT_PLOT_GROUP_IDX;
    Visualization_Mode vis_mode { .type=Visualization_Mode::SHOW_ENTIRE_PLOT_GROUP };
    Range_XY plot_range = Range_XY{};
    rl::Rectangle plot_screen = rl::Rectangle{ 0, 0, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT };

    bool window_visible = false;
};

struct Plot_State_Update {
    Plot_Update plot_updates[MAX_PLOT_SIZE];
    Plot_Group_Update plot_group_updates[MAX_PLOT_GROUP_SIZE];
    
    Group_IDX visible_group = DEFAULT_PLOT_GROUP_IDX;
    Visualization_Mode vis_mode { .type=Visualization_Mode::SHOW_ENTIRE_PLOT_GROUP };

    bool window_visible = false;
    bool terminate = false;

    void reset() {
        for (Plot_IDX plot_idx = 0; plot_idx < MAX_PLOT_SIZE; ++plot_idx) {
            if (!plot_updates[plot_idx].empty_update) {
                plot_updates[plot_idx].reset();
            }
        }
        for (Group_IDX group_idx = 0; group_idx < MAX_PLOT_GROUP_SIZE; ++group_idx) {
            if (!plot_group_updates[group_idx].empty_update) {
                plot_group_updates[group_idx].reset();
            }
        }
    }
};

struct Gui {
    int target_fps = DEFAULT_FPS;
    rl::Font font_normal;
    rl::Font font_large;
    float fontsize_normal = 22;
    float fontsize_large = 24;
    float fontspacing = 0;
    
    int window_width = DEFAULT_WINDOW_WIDTH;
    int window_height = DEFAULT_WINDOW_HEIGHT;

    int x_pixels_per_tick = 50;
    int y_pixels_per_tick = 50;

    Color color_backgound_2 = Color{ 0x25, 0x25, 0x25, 0xff };
    Color color_backgound_1 = Color{ 0xff, 0xff, 0xff, 0x10 };
    Color color_plot_screen_border = Color{ 0xff, 0xff, 0xff };
    Color color_coordinate_axes = Color{ 0xff, 0xff, 0xff, 0x40 };
    float plot_screen_border_width = 1;
    int tick_mark_len = DEFAULT_TICK_MARK_LEN;

    float offset_normal = 5;
    float offset_small = 2;

    float zoom_to_zero_pixel_distance_threshold = DEFAULT_ZOOM_TO_ZERO_PIXEL_DISTANCE_THRESHOLD;
};

static Gui g_gui;
static Plot_State g_plot_state;
static Plot_State_Update g_plot_state_update;
static std::mutex g_plot_state_update_mutex;

static Range_XY bounding_box_of_plot(Plot& plot, uint64_t begin_idx)
{
    Range_XY bb = { MAX_PLOTRANGE_VALUE, -MAX_PLOTRANGE_VALUE, MAX_PLOTRANGE_VALUE, -MAX_PLOTRANGE_VALUE };
    if (plot.has_x_coordinate()) {
        for (uint64_t i = begin_idx; i < plot.points_y.size(); ++i) {
            bb.x_begin = plot.points_x[i] < bb.x_begin ? plot.points_x[i] : bb.x_begin;
            bb.x_end = plot.points_x[i] > bb.x_end ? plot.points_x[i] : bb.x_end;
            bb.y_begin = plot.points_y[i] < bb.y_begin ? plot.points_y[i] : bb.y_begin;
            bb.y_end = plot.points_y[i] > bb.y_end ? plot.points_y[i] : bb.y_end;
        }
    }
    else {
        bb.x_begin = begin_idx;
        bb.x_end = plot.points_y.size() - 1;
        for (uint64_t i = begin_idx; i < plot.points_y.size(); ++i) {
            bb.y_begin = plot.points_y[i] < bb.y_begin ? plot.points_y[i] : bb.y_begin;
            bb.y_end = plot.points_y[i] > bb.y_end ? plot.points_y[i] : bb.y_end;
        }
    }
    return bb;
}

static Range_XY bounding_box_of_plots_bounding_boxes(std::vector<Plot_IDX>& plots)
{
    Range_XY bb = { MAX_PLOTRANGE_VALUE, -MAX_PLOTRANGE_VALUE, MAX_PLOTRANGE_VALUE, -MAX_PLOTRANGE_VALUE };
    for (uint64_t i = 0; i < plots.size(); ++i) {
        Plot plot = g_plot_state.plots[plots[i]];
        bb.x_begin = plot.bb.x_begin < bb.x_begin ? plot.bb.x_begin : bb.x_begin;
        bb.x_end = plot.bb.x_end > bb.x_end ? plot.bb.x_end : bb.x_end;
        bb.y_begin = plot.bb.y_begin < bb.y_begin ? plot.bb.y_begin : bb.y_begin;
        bb.y_end = plot.bb.y_end > bb.y_end ? plot.bb.y_end : bb.y_end;
    }
    return bb;
}

static Range_XY bounding_box_of_bounding_boxes(std::vector<Range_XY>& bbs)
{
    Range_XY bb = { MAX_PLOTRANGE_VALUE, -MAX_PLOTRANGE_VALUE, MAX_PLOTRANGE_VALUE, -MAX_PLOTRANGE_VALUE };
    for (uint64_t i = 0; i < bbs.size(); ++i) {
        bb.x_begin = bbs[i].x_begin < bb.x_begin ? bbs[i].x_begin : bb.x_begin;
        bb.x_end = bbs[i].x_end > bb.x_end ? bbs[i].x_end : bb.x_end;
        bb.y_begin = bbs[i].y_begin < bb.y_begin ? bbs[i].y_begin : bb.y_begin;
        bb.y_end = bbs[i].y_end > bb.y_end ? bbs[i].y_end : bb.y_end;
    }
    return bb;
}

static void apply_and_reset_plot_state_update()
{
    g_plot_state_update_mutex.lock();
    
    g_plot_state.visible_group = g_plot_state_update.visible_group;
    g_plot_state.window_visible = g_plot_state_update.window_visible;
    g_plot_state.vis_mode = g_plot_state_update.vis_mode;

    if (!g_plot_state.window_visible) {
        g_plot_state_update_mutex.unlock();
        return;
    }

    for (Plot_IDX plot_idx = 0; plot_idx < MAX_PLOT_SIZE; ++plot_idx)
    {
        Plot_Update& update = g_plot_state_update.plot_updates[plot_idx];
        if (update.empty_update) continue;

        Plot& plot = g_plot_state.plots[plot_idx];

        if (!plot.initialized) {
            int label_len = 12; // should be enough for "[plot_idx]"
            plot.label = new char[label_len];
            snprintf(plot.label, label_len, "[%d]", plot_idx);
            plot.label[label_len - 1] = '\0';
            
            plot.color = plot_color_table[plot_idx % plot_color_table_size];
            
            plot.initialized = true;
        }

        if (update.has_custom_color) {
            plot.color = update.custom_color;
        }

        if (update.new_name) {
            int label_len = 12 + strlen(update.new_name) + 1;
            delete plot.label;
            plot.label = new char[label_len];
            snprintf(plot.label, label_len, "[%d] %s", plot_idx, update.new_name);
            plot.label[label_len - 1] = '\0';
        }

        uint64_t old_length = plot.points_y.size();
        uint64_t new_length = old_length;
        if (update.was_cleared || old_length == 0) {
            new_length = 0;
            plot.bb.x_begin = MAX_PLOTRANGE_VALUE;
            plot.bb.x_end = -MAX_PLOTRANGE_VALUE;
            plot.bb.y_begin = MAX_PLOTRANGE_VALUE;
            plot.bb.y_end = -MAX_PLOTRANGE_VALUE;
        }
        new_length += update.new_points_y.size();
        uint64_t points_update_offset = new_length - update.new_points_y.size();
        
        if (update.contains_points) {
            assert(update.new_points_x.size() == update.new_points_y.size());
            plot.points_x.resize(new_length);
            plot.points_y.resize(new_length);

            for (uint64_t i = 0; i < update.new_points_y.size(); ++i) {
                plot.points_x[i + points_update_offset] = update.new_points_x[i];
                plot.points_y[i + points_update_offset] = update.new_points_y[i];

                plot.bb.x_begin = update.new_points_x[i] < plot.bb.x_begin ? update.new_points_x[i] : plot.bb.x_begin;
                plot.bb.x_end = update.new_points_x[i] > plot.bb.x_end ? update.new_points_x[i] : plot.bb.x_end;
                plot.bb.y_begin = update.new_points_y[i] < plot.bb.y_begin ? update.new_points_y[i] : plot.bb.y_begin;
                plot.bb.y_end = update.new_points_y[i] > plot.bb.y_end ? update.new_points_y[i] : plot.bb.y_end;
            }
        }
        else {
            assert(update.new_points_x.size() == 0);
            plot.points_y.resize(new_length);

            plot.bb.x_begin = 0;
            plot.bb.x_end = plot.points_y.size() == 0 ? 0 : plot.points_y.size() - 1;
            for (uint64_t i = 0; i < update.new_points_y.size(); ++i) {
                plot.points_y[i + points_update_offset] = update.new_points_y[i];

                plot.bb.y_begin = update.new_points_y[i] < plot.bb.y_begin ? update.new_points_y[i] : plot.bb.y_begin;
                plot.bb.y_end = update.new_points_y[i] > plot.bb.y_end ? update.new_points_y[i] : plot.bb.y_end;
            }
        }
    }

    for (Group_IDX group_idx = 0; group_idx < MAX_PLOT_GROUP_SIZE; ++group_idx)
    {
        Plot_Group_Update& update = g_plot_state_update.plot_group_updates[group_idx];
        if (update.empty_update) continue;
        
        Plot_Group& group = g_plot_state.plot_groups[group_idx];

        if (!group.initialized) {
            int label_len = 24; // should be enough for "[group_idx]"
            group.label = new char[label_len];
            snprintf(group.label, label_len, "[%d] Plot Group", group_idx);
            group.label[label_len - 1] = '\0';
            
            group.initialized = true;
        }

        if (update.new_name) {
            int label_len = 12 + strlen(update.new_name) + 1;
            delete group.label;
            group.label = new char[label_len];
            snprintf(group.label, label_len, "[%d] %s", group_idx, update.new_name);
            group.label[label_len - 1] = '\0';
        }

        if (update.was_cleared) {
            group.plots.clear();
        }

        for (size_t i = 0; i < update.new_plots.size(); ++i) {
            bool already_contained = false;
            for (size_t j = 0; j < group.plots.size(); ++j) {
                already_contained |= group.plots[j] == update.new_plots[i];
            }
            if (!already_contained) {
                group.plots.push_back(update.new_plots[i]);
            }
        }

        for (size_t i = 0; i < update.remove_plots.size(); ++i) {
            for (size_t j = 0; j < group.plots.size(); ++j) {
                if (group.plots[j] == update.remove_plots[i]) {
                    group.plots.erase(group.plots.begin() + j);
                }
            }
        }
    }

    g_plot_state_update.reset();
    
    g_plot_state_update_mutex.unlock();
}

static double linear_map(double x, double in_min, double in_max, double out_min, double out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static void gui_update_plot_range_interactive_mode(Range_XY& plot_range, rl::Rectangle plot_screen)
{
    auto x_to_plotspace = [=](double x) -> double {
        return linear_map(x, (double) plot_screen.x, (double) plot_screen.x + plot_screen.width, plot_range.x_begin, plot_range.x_end);
    };

    auto y_to_plotspace = [=](double y) -> double {
        return linear_map(y, (double) plot_screen.y, (double) plot_screen.y + plot_screen.height, plot_range.y_end, plot_range.y_begin);
    };

    double plot_space_pan_x = 0;
    double plot_space_pan_y = 0;
    if (rl::IsMouseButtonDown(rl::MOUSE_LEFT_BUTTON)) {
        rl::Vector2 mouse_delta = rl::GetMouseDelta();
        plot_space_pan_x = x_to_plotspace(0) - x_to_plotspace(mouse_delta.x);
        plot_space_pan_y = y_to_plotspace(0) - y_to_plotspace(mouse_delta.y);
    }

    float mouse_wheel_delta = rl::GetMouseWheelMove();
    double zoom_factor_x = std::pow(1.2, (double) -mouse_wheel_delta * SCROLL_ZOOM_FACTOR);
    double zoom_factor_y = std::pow(1.2, (double) -mouse_wheel_delta * SCROLL_ZOOM_FACTOR);

    if (rl::IsKeyDown(rl::KEY_LEFT_CONTROL)) {
        zoom_factor_x = 1.0;
    }
    
    if (rl::IsKeyDown(rl::KEY_LEFT_SHIFT)) {
        zoom_factor_y = 1.0;
    }

    auto x_to_screenspace = [=](double x) -> float {
        return linear_map(x, plot_range.x_begin, plot_range.x_end, (double) plot_screen.x, (double) plot_screen.x + plot_screen.width);
    };

    auto y_to_screenspace = [=](double y) -> float {
        return linear_map(y, plot_range.y_begin, plot_range.y_end, (double) plot_screen.y + plot_screen.height, (double) plot_screen.y);
    };

    rl::Vector2 mouse_pos = rl::GetMousePosition();
    double zoom_center_x = x_to_plotspace(mouse_pos.x);
    double zoom_center_y = y_to_plotspace(mouse_pos.y);

    // prefer zooming to the origin / to the x=0, y=0 coordinate axes
    if (std::abs(x_to_screenspace(0) - mouse_pos.x) < g_gui.zoom_to_zero_pixel_distance_threshold) {
        zoom_center_x = 0;
    }

    if (std::abs(y_to_screenspace(0) - mouse_pos.y) < g_gui.zoom_to_zero_pixel_distance_threshold) {
        zoom_center_y = 0;
    }

    // xy = zoom_matrix * (xy - mouse_pos) + mouse_pos + mouse_pan
    plot_range.x_begin = zoom_factor_x * (plot_range.x_begin - zoom_center_x) + zoom_center_x + plot_space_pan_x;
    plot_range.x_end = zoom_factor_x * (plot_range.x_end - zoom_center_x) + zoom_center_x + plot_space_pan_x;
    plot_range.y_begin = zoom_factor_y * (plot_range.y_begin - zoom_center_y) + zoom_center_y + plot_space_pan_y;
    plot_range.y_end = zoom_factor_y * (plot_range.y_end - zoom_center_y) + zoom_center_y + plot_space_pan_y;
}

struct Ticks {
    int x_count = 0;
    int y_count = 0;

    double x_spacing;
    double x_begin;
    double y_spacing;
    double y_begin;

    char x_text[MAX_TICK_MARK_COUNT][MAX_TICK_MARK_TEXT_LENGTH];
    char y_text[MAX_TICK_MARK_COUNT][MAX_TICK_MARK_TEXT_LENGTH];
    float x_text_width[MAX_TICK_MARK_COUNT];
    float x_text_width_max = 0;
    float y_text_width[MAX_TICK_MARK_COUNT];
    float y_text_width_max = 0;
};

static void gui_generate_ticks(Ticks& ticks, rl::Rectangle bounds, Range_XY plot_range, int x_pixels_per_tick = g_gui.x_pixels_per_tick)
{
    auto calculate_tick_spacing = [](double begin, double end, int tick_count) -> double {
        double raw_step = (end - begin) / tick_count;
        double exponent = std::floor(std::log10(raw_step));
        double base = std::pow(10, exponent);
        double fraction = raw_step / base;
        const double nice_fractions[] = {1, 2, 2.5, 5, 10};
        double best_fraction = 1;
        for (size_t i = 0; i < sizeof(nice_fractions) / sizeof(double); ++i) {
            if (fraction <= nice_fractions[i]) {
                best_fraction = nice_fractions[i];
                break;
            }
        }
        return best_fraction * base;
    };

    auto remove_excessive_trailing_zeros = [](char* number_str, int max_len){
        int true_len = strnlen(number_str, max_len);
        if (true_len <= 0) return;

        int trailing_zeros_cnt = 0;
        int str_idx = true_len;
        while (str_idx > 0 && number_str[--str_idx] == '0') ++trailing_zeros_cnt;

        if (trailing_zeros_cnt >= EXCESSIVE_TRAILING_ZEROS_THRESHOLD) {
            snprintf(&number_str[str_idx + 1], max_len - str_idx, "+e%d", trailing_zeros_cnt);
        }
    };

    ticks.x_count = std::min((int) std::floor(bounds.width / x_pixels_per_tick), MAX_TICK_MARK_COUNT);
    ticks.y_count = std::min((int) std::floor(bounds.height / g_gui.y_pixels_per_tick), MAX_TICK_MARK_COUNT);

    ticks.x_spacing = calculate_tick_spacing(plot_range.x_begin, plot_range.x_end, ticks.x_count);
    ticks.x_begin = ceil(plot_range.x_begin / ticks.x_spacing) * ticks.x_spacing;
    ticks.y_spacing = calculate_tick_spacing(plot_range.y_begin, plot_range.y_end, ticks.y_count);
    ticks.y_begin = ceil(plot_range.y_begin / ticks.y_spacing) * ticks.y_spacing;

    int tick_idx = 0;
    for (double x = ticks.x_begin; x < plot_range.x_end && tick_idx < MAX_TICK_MARK_COUNT; x += ticks.x_spacing, ++tick_idx) {
        // this is a stupid hack to make sure 0 is dispayed cleanly, it works because 0 is always included as a tick,
        // if 0 is in the plot_range. So we can find it easily by comparing x to the tick-spacing
        if (std::abs(x) < ticks.x_spacing * 1e-3) x = 0.0;
        
        snprintf(ticks.x_text[tick_idx], MAX_TICK_MARK_TEXT_LENGTH, "%.14g", x);
        remove_excessive_trailing_zeros(ticks.x_text[tick_idx], MAX_TICK_MARK_TEXT_LENGTH);
        ticks.x_text_width[tick_idx] = MeasureTextEx(g_gui.font_normal, ticks.x_text[tick_idx], g_gui.fontsize_normal, g_gui.fontspacing).x;
        ticks.x_text_width_max = ticks.x_text_width[tick_idx] > ticks.x_text_width_max ? ticks.x_text_width[tick_idx] : ticks.x_text_width_max;
    }

    // Regenerate the ticks if the text is too wide.
    if (ticks.x_text_width_max > x_pixels_per_tick && x_pixels_per_tick < 1000) {
        gui_generate_ticks(ticks, bounds, plot_range, x_pixels_per_tick * 2);
        return;
    }

    tick_idx = 0;
    for (double y = ticks.y_begin; y < plot_range.y_end && tick_idx < MAX_TICK_MARK_COUNT; y += ticks.y_spacing, ++tick_idx) {
        if (std::abs(y) < ticks.y_spacing * 1e-3) y = 0.0;
        snprintf(ticks.y_text[tick_idx], MAX_TICK_MARK_TEXT_LENGTH, "%.14g", y);
        remove_excessive_trailing_zeros(ticks.y_text[tick_idx], MAX_TICK_MARK_TEXT_LENGTH);
        ticks.y_text_width[tick_idx] = MeasureTextEx(g_gui.font_normal, ticks.y_text[tick_idx], g_gui.fontsize_normal, g_gui.fontspacing).x;
        ticks.y_text_width_max = ticks.y_text_width[tick_idx] > ticks.y_text_width_max ? ticks.y_text_width[tick_idx] : ticks.y_text_width_max;
    }
}

void gui_loop()
{
    static bool window_is_init = false;

    Range_XY& plot_range = g_plot_state.plot_range;
    rl::Rectangle& plot_screen = g_plot_state.plot_screen;
    
    while (true)
    {
        apply_and_reset_plot_state_update();
                
        if (!window_is_init && g_plot_state.window_visible) {
            rl::SetConfigFlags(rl::FLAG_WINDOW_RESIZABLE);
            rl::SetTraceLogLevel(rl::LOG_ERROR);
            rl::InitWindow(g_gui.window_width, g_gui.window_height, "Plotlib Window");
            rl::SetTargetFPS(g_gui.target_fps);
            g_gui.font_normal = rl::LoadFontFromMemory(".ttf", gui_font_binary_ttf, gui_font_binary_ttf_len, g_gui.fontsize_normal, nullptr, 0);
            g_gui.font_large = rl::LoadFontFromMemory(".ttf", gui_font_binary_ttf, gui_font_binary_ttf_len, g_gui.fontsize_large, nullptr, 0);
            window_is_init = true;
        }

        if (window_is_init && rl::WindowShouldClose()) {
            rl::CloseWindow();
            window_is_init = false;
            g_plot_state.window_visible = false;
            
            // This is a special occasions where we need to mutate the 'g_plot_state_update' from within the gui-thread.
            // Overwriting the commands from the api-functions like this should only happen when absolutely necessary.
            g_plot_state_update_mutex.lock();
            g_plot_state_update.window_visible = false;
            g_plot_state_update_mutex.unlock();
            g_plot_state.window_visible = false;

            std::this_thread::sleep_for(std::chrono::microseconds(1000));
            continue;
        }

        if (!g_plot_state.window_visible) {
            std::this_thread::sleep_for(std::chrono::microseconds(1000));
            continue;
        }

        g_gui.window_width = rl::GetScreenWidth();
        g_gui.window_height = rl::GetScreenHeight();

        rl::Rectangle bounds = {0, 0, (float) g_gui.window_width, (float) g_gui.window_height};

        rl::BeginDrawing();
        {
            Plot_Group& group = g_plot_state.plot_groups[g_plot_state.visible_group];

            // Draw Background

            rl::DrawRectangleRec(bounds, to_rl_color(g_gui.color_backgound_2));

            // Determine the plot range (The xy-range in plotspace that should be displayed)

            switch (g_plot_state.vis_mode.type) {
            case Visualization_Mode::INTERACTIVE:
                gui_update_plot_range_interactive_mode(plot_range, plot_screen);
                break;
            case Visualization_Mode::SHOW_ENTIRE_PLOT_GROUP:
                plot_range = bounding_box_of_plots_bounding_boxes(group.plots);
                break;
            case Visualization_Mode::SHOW_X_RANGE_OF_TAIL:
                plot_range = bounding_box_of_plots_bounding_boxes(group.plots);
                plot_range.x_begin = plot_range.x_end - g_plot_state.vis_mode.x_range;
                break;
            case Visualization_Mode::SHOW_N_POINTS_OF_TAIL:
            {
                std::vector<Range_XY> bounding_boxes(group.plots.size());
                for (uint64_t i = 0; i < group.plots.size(); ++i) {
                    Plot& plot = g_plot_state.plots[group.plots[i]];
                    uint64_t plot_points_begin_idx = 0;
                    if (g_plot_state.vis_mode.n_points < plot.points_y.size()) {
                        plot_points_begin_idx = plot.points_y.size() - g_plot_state.vis_mode.n_points;
                    }
                    bounding_boxes[i] = bounding_box_of_plot(plot, plot_points_begin_idx);
                }
                plot_range = bounding_box_of_bounding_boxes(bounding_boxes);
            }
            break;
            case Visualization_Mode::SHOW_SPECIFIC_PLOT:
                assert(g_plot_state.vis_mode.specific_plot != INVALID_IDX);
                plot_range = bounding_box_of_plot(g_plot_state.plots[g_plot_state.vis_mode.specific_plot], 0);
                break;
            }

            // Fix the plot range if it is malformed

            auto clamp_plot_range = [](double& val) -> bool {
                if (val > MAX_PLOTRANGE_VALUE) {
                    val = MAX_PLOTRANGE_VALUE;
                    return true;
                }
                else if (val < -MAX_PLOTRANGE_VALUE) {
                    val = -MAX_PLOTRANGE_VALUE;
                    return true;
                }
                else if (val > 0 && val < MIN_PLOTRANGE_VALUE) {
                    val = MIN_PLOTRANGE_VALUE;
                    return true;
                }
                else if (val < 0 && val > -MIN_PLOTRANGE_VALUE) {
                    val = -MIN_PLOTRANGE_VALUE;
                    return true;
                }
                return false;
            };

            bool was_clamped = false;
            was_clamped |= clamp_plot_range(plot_range.x_begin);
            was_clamped |= clamp_plot_range(plot_range.x_end);
            was_clamped |= clamp_plot_range(plot_range.y_begin);
            was_clamped |= clamp_plot_range(plot_range.y_end);

            if (was_clamped) {
                printf(WARNING "Coordinates of the plot-space were clamped to the range [%g, %g]\n", MIN_PLOTRANGE_VALUE, MAX_PLOTRANGE_VALUE);
            }

            if (plot_range.x_begin == plot_range.x_end) {
                plot_range.x_begin -= 0.5;
                plot_range.x_end += 0.5;
            }
            else if (plot_range.x_begin > plot_range.x_end) {
                plot_range.x_begin = -0.5;
                plot_range.x_end = 0.5;
            }
            
            if (plot_range.y_begin == plot_range.y_end) {
                plot_range.y_begin -= 0.5;
                plot_range.y_end += 0.5;
            }
            else if (plot_range.y_begin > plot_range.y_end) {
                plot_range.y_begin = -0.5;
                plot_range.y_end = 0.5;
            }

            auto limit_range_to_tolerable_precision = [](double& range_begin, double& range_end) -> void {
                double nextafter_begin = std::nextafter(range_begin, std::numeric_limits<double>::infinity());
                assert(range_end - range_begin  >= 0);
                assert(nextafter_begin - range_begin >= 0);
                double precision_correction = (range_end - range_begin) - (nextafter_begin - range_begin) * PRECISION_SAFTEY_FACTOR;
                if (precision_correction < 0) {
                    range_begin -= std::abs(precision_correction) / 2.0;
                    range_end   += std::abs(precision_correction) / 2.0;
                }  
            };

            limit_range_to_tolerable_precision(plot_range.x_begin, plot_range.x_end);
            limit_range_to_tolerable_precision(plot_range.y_begin, plot_range.y_end);

            // Calculate the tick spacing and generate the tick labels

            Ticks ticks;
            gui_generate_ticks(ticks, bounds, plot_range);

            // Draw plot legend

            float legend_content_width = 0;
            if (g_plot_state.visible_group != DEFAULT_PLOT_GROUP_IDX) {
                legend_content_width = rl::MeasureTextEx(g_gui.font_large, group.label, g_gui.fontsize_large, g_gui.fontspacing).x;
            }
            for (uint64_t i = 0; i < group.plots.size(); ++i) {
                Plot& plot = g_plot_state.plots[group.plots[i]];
                float label_text_width = rl::MeasureTextEx(g_gui.font_normal, plot.label, g_gui.fontsize_normal, g_gui.fontspacing).x;
                legend_content_width = label_text_width > legend_content_width ? label_text_width : legend_content_width;
            }

            float legend_x = bounds.x + bounds.width - (legend_content_width + g_gui.offset_normal);
            float legend_y = g_gui.offset_normal;
            float legend_width = legend_content_width + 2 * g_gui.offset_normal;

            if (g_plot_state.visible_group != DEFAULT_PLOT_GROUP_IDX) {
                rl::DrawTextEx(g_gui.font_large, group.label, {legend_x, legend_y}, g_gui.fontsize_large, g_gui.fontspacing, rl::WHITE);
                legend_y += g_gui.fontsize_large + g_gui.offset_small;
                
                rl::DrawLineV({legend_x, legend_y}, {legend_x + legend_content_width, legend_y}, rl::WHITE);
                legend_y += g_gui.offset_small;
            }

            for (uint64_t i = 0; i < group.plots.size(); ++i) {
                Plot& plot = g_plot_state.plots[group.plots[i]];
                rl::DrawTextEx(g_gui.font_normal, plot.label, {legend_x, legend_y}, g_gui.fontsize_normal, g_gui.fontspacing, to_rl_color(plot.color));
                legend_y += g_gui.fontsize_normal;
            }

            // Determine the size of the plot-screen (the part of the window where the plots should be drawn into)

            float left_plot_screen_offset = ticks.y_text_width_max + 2 * g_gui.offset_normal;
            float right_plot_screen_offset = std::max((float) MIN_PLOT_SCREEN_TO_BOUNDS_OFFSET, legend_width);
            float top_plot_screen_offset = MIN_PLOT_SCREEN_TO_BOUNDS_OFFSET;
            float bottom_plot_screen_offset = g_gui.fontsize_normal + g_gui.offset_normal;

            plot_screen = { bounds.x + left_plot_screen_offset,
                            bounds.y + top_plot_screen_offset,
                            bounds.width - (left_plot_screen_offset + right_plot_screen_offset),
                            bounds.height - (top_plot_screen_offset + bottom_plot_screen_offset) };

            if (plot_screen.width < 1) plot_screen.width = 1;
            if (plot_screen.height < 1) plot_screen.height = 1;

            auto x_to_screenspace = [=](double x) -> float {
                return linear_map(x, plot_range.x_begin, plot_range.x_end, (double) plot_screen.x, (double) plot_screen.x + plot_screen.width);
            };

            auto y_to_screenspace = [=](double y) -> float {
                return linear_map(y, plot_range.y_begin, plot_range.y_end, (double) plot_screen.y + plot_screen.height, (double) plot_screen.y);
            };

            // Draw plot_screen border and ticks (tick-lines and tick-labels)

            float bw = g_gui.plot_screen_border_width;
            rl::DrawRectangleLinesEx({plot_screen.x - bw, plot_screen.y - bw, plot_screen.width + 2*bw, plot_screen.height + 2*bw},
                                     bw, to_rl_color(g_gui.color_plot_screen_border));
            
            int tick_idx = 0;
            for (double x = ticks.x_begin; x < plot_range.x_end; x += ticks.x_spacing, ++tick_idx) {
                float x_screenspace = x_to_screenspace(x);
                rl::DrawLine(x_screenspace, plot_screen.y, x_screenspace, plot_screen.y + plot_screen.height, to_rl_color(g_gui.color_backgound_1));
                rl::DrawTextEx(g_gui.font_normal, ticks.x_text[tick_idx], {x_screenspace, plot_screen.y + plot_screen.height},
                           g_gui.fontsize_normal, g_gui.fontspacing, rl::WHITE);
            }
            
            tick_idx = 0;
            for (double y = ticks.y_begin; y < plot_range.y_end; y += ticks.y_spacing, ++tick_idx) {
                float y_screenspace = y_to_screenspace(y);
                rl::DrawLine(plot_screen.x, y_screenspace, plot_screen.x + plot_screen.width, y_screenspace, to_rl_color(g_gui.color_backgound_1));
                rl::DrawTextEx(g_gui.font_normal, ticks.y_text[tick_idx], {bounds.x + g_gui.offset_normal, y_screenspace - g_gui.fontsize_normal},
                           g_gui.fontsize_normal, g_gui.fontspacing, rl::WHITE);
            }

            // Draw x=0, y=0 coordinate-axes
            
            rl::DrawLineV({plot_screen.x, y_to_screenspace(0)}, {plot_screen.x + plot_screen.width, y_to_screenspace(0)}, to_rl_color(g_gui.color_coordinate_axes));
            rl::DrawLineV({x_to_screenspace(0), plot_screen.y}, {x_to_screenspace(0), plot_screen.y + plot_screen.height}, to_rl_color(g_gui.color_coordinate_axes));
            
            // Draw the plots

            for (uint64_t i = 0; i < group.plots.size(); ++i)
            {
                Plot_IDX plot_idx = group.plots[i];
                Plot& plot = g_plot_state.plots[plot_idx];
                    
                if (plot.empty()) continue;
                
                uint64_t plot_points_begin_idx = 0;
                if (g_plot_state.vis_mode.type == Visualization_Mode::SHOW_N_POINTS_OF_TAIL && g_plot_state.vis_mode.n_points < plot.points_y.size()) {
                    plot_points_begin_idx = plot.points_y.size() - g_plot_state.vis_mode.n_points;
                }

                rl::BeginScissorMode(plot_screen.x, plot_screen.y, plot_screen.width, plot_screen.height);
                {
                    if (plot.has_x_coordinate()) {
                        float x_prev = x_to_screenspace(plot.points_x[plot_points_begin_idx]);
                        float y_prev = y_to_screenspace(plot.points_y[plot_points_begin_idx]);
        
                        for (uint64_t i = plot_points_begin_idx + 1; i < plot.points_y.size(); ++i) {
                            float x = x_to_screenspace(plot.points_x[i]);
                            float y = y_to_screenspace(plot.points_y[i]);
                            rl::DrawLineV({x_prev, y_prev}, {x, y}, to_rl_color(plot.color));
                            x_prev = x;
                            y_prev = y;
                        }
                    }
                    else {
                        float y_prev = y_to_screenspace(plot.points_y[plot_points_begin_idx]);
        
                        for (uint64_t i = plot_points_begin_idx + 1; i < plot.points_y.size(); ++i) {
                            float y = y_to_screenspace(plot.points_y[i]);
                            rl::DrawLineV({x_to_screenspace(i - 1), y_prev}, {x_to_screenspace(i), y}, to_rl_color(plot.color));
                            y_prev = y;
                        }
                    }
                }
                rl::EndScissorMode();
            }

            // Draw ticks marks (they have to be drawn over the plots)

            tick_idx = 0;
            for (double x = ticks.x_begin; x < plot_range.x_end; x += ticks.x_spacing, ++tick_idx) {
                float x_screenspace = x_to_screenspace(x);
                rl::DrawLineEx({x_screenspace, plot_screen.y + plot_screen.height - g_gui.tick_mark_len}, {x_screenspace, plot_screen.y + plot_screen.height},
                               g_gui.plot_screen_border_width, to_rl_color(g_gui.color_plot_screen_border));
            }
            
            tick_idx = 0;
            for (double y = ticks.y_begin; y < plot_range.y_end; y += ticks.y_spacing, ++tick_idx) {
                float y_screenspace = y_to_screenspace(y);
                rl::DrawLineEx({plot_screen.x, y_screenspace}, {plot_screen.x + g_gui.tick_mark_len, y_screenspace},
                               g_gui.plot_screen_border_width, to_rl_color(g_gui.color_plot_screen_border));
            }
        }
        rl::EndDrawing();
    }
}

static void start_gui_thread() {
    static std::thread gui_thread(gui_loop);
    gui_thread.detach();
}

static void start_gui_thread_if_not_started() {
    static bool started_gui_thread = false;
    if (!started_gui_thread) {
        start_gui_thread();
        started_gui_thread = true;
    }
}

PLOTAPI void plotlib_show()
{
    g_plot_state_update_mutex.lock();
    g_plot_state_update.window_visible = true;
    start_gui_thread_if_not_started();
    g_plot_state_update_mutex.unlock();
}

PLOTAPI void plotlib_mode_interactive()
{
    g_plot_state_update_mutex.lock();
    g_plot_state_update.vis_mode = Visualization_Mode { .type=Visualization_Mode::INTERACTIVE };
    g_plot_state_update_mutex.unlock();
}

PLOTAPI void plotlib_mode_show_n_points_of_tail(uint64_t points_count)
{
    g_plot_state_update_mutex.lock();
    g_plot_state_update.vis_mode = Visualization_Mode { .type=Visualization_Mode::SHOW_N_POINTS_OF_TAIL, .n_points=points_count };
    g_plot_state_update_mutex.unlock();
}

PLOTAPI void plotlib_mode_show_x_range_of_tail(double x_range)
{
    g_plot_state_update_mutex.lock();
    g_plot_state_update.vis_mode = Visualization_Mode { .type=Visualization_Mode::SHOW_X_RANGE_OF_TAIL, .x_range=x_range };
    g_plot_state_update_mutex.unlock();
}

PLOTAPI void plotlib_mode_fill_window()
{
    g_plot_state_update_mutex.lock();
    g_plot_state_update.vis_mode = Visualization_Mode { .type=Visualization_Mode::SHOW_ENTIRE_PLOT_GROUP };
    g_plot_state_update_mutex.unlock();
}

PLOTAPI bool plotlib_mode_show_specific_plot(uint32_t plot_idx)
{
    if (!valid_plot_idx(plot_idx)) return false;
    g_plot_state_update_mutex.lock();
    g_plot_state_update.vis_mode = Visualization_Mode { .type=Visualization_Mode::SHOW_SPECIFIC_PLOT, .specific_plot=plot_idx };
    g_plot_state_update_mutex.unlock();
    return true;
}

PLOTAPI void plotlib_clear_all_plots()
{
    g_plot_state_update_mutex.lock();
    
    for (Plot_IDX plot_idx = 0; plot_idx < MAX_PLOT_SIZE; ++plot_idx) {
        g_plot_state_update.plot_updates[plot_idx].clear_plot();
    }
    for (Group_IDX group_idx = 0; group_idx < MAX_PLOT_GROUP_SIZE; ++group_idx) {
        g_plot_state_update.plot_group_updates[group_idx].clear_group();
    }
    
    g_plot_state_update_mutex.unlock();    
}

PLOTAPI bool plot_show(Plot_IDX plot_idx)
{
    if (!valid_plot_idx(plot_idx)) return false;
    g_plot_state_update_mutex.lock();

    Plot_Group_Update& group_update = g_plot_state_update.plot_group_updates[DEFAULT_PLOT_GROUP_IDX];

    if (g_plot_state_update.visible_group != DEFAULT_PLOT_GROUP_IDX) {
        group_update.clear_group();
    }

    group_update.new_plots.push_back(plot_idx);
    group_update.empty_update = false;
    g_plot_state_update.visible_group = DEFAULT_PLOT_GROUP_IDX;
    g_plot_state_update.window_visible = true;
            
    g_plot_state_update_mutex.unlock();
    start_gui_thread_if_not_started();
    return true;
}

PLOTAPI bool plot_hide(Plot_IDX plot_idx)
{
    if (!valid_plot_idx(plot_idx)) return false;
    g_plot_state_update_mutex.lock();

    Plot_Group_Update& group_update = g_plot_state_update.plot_group_updates[DEFAULT_PLOT_GROUP_IDX];

    group_update.remove_plots.push_back(plot_idx);
    group_update.empty_update = false;
            
    g_plot_state_update_mutex.unlock();
    return true;
}

PLOTAPI void plot_hide_all()
{
    g_plot_state_update_mutex.lock();

    Plot_Group_Update& group_update = g_plot_state_update.plot_group_updates[DEFAULT_PLOT_GROUP_IDX];

    group_update.clear_group();
    group_update.empty_update = false;
            
    g_plot_state_update_mutex.unlock();
}

PLOTAPI bool plot_clear(uint32_t plot_idx)
{
    if (!valid_plot_idx(plot_idx)) return false;
    g_plot_state_update_mutex.lock();

    g_plot_state_update.plot_updates[plot_idx].clear_plot();

    g_plot_state_update_mutex.unlock();
    return true;
}

PLOTAPI bool plot_set_color(uint32_t plot_idx, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    if (!valid_plot_idx(plot_idx)) return false;
    g_plot_state_update_mutex.lock();

    g_plot_state_update.plot_updates[plot_idx].custom_color = Color{r, g, b, a};
    g_plot_state_update.plot_updates[plot_idx].has_custom_color = true;
    g_plot_state_update.plot_updates[plot_idx].empty_update = false;

    g_plot_state_update_mutex.unlock();
    return true;
}

PLOTAPI bool plot_set_name(uint32_t plot_idx, const char* name)
{
    if (!valid_plot_idx(plot_idx)) return false;
    g_plot_state_update_mutex.lock();
    
    Plot_Update& plot_update = g_plot_state_update.plot_updates[plot_idx];
    
    plot_update.new_name = new char[strlen(name) + 1];
    strcpy(plot_update.new_name, name);
    plot_update.empty_update = false;

    g_plot_state_update_mutex.unlock();
    return true;
}

PLOTAPI bool plot_fill_numbers(uint32_t plot_idx, double* numbers, uint64_t length)
{
    if (!valid_plot_idx(plot_idx)) return false;
    g_plot_state_update_mutex.lock();
    
    Plot_Update& plot_update = g_plot_state_update.plot_updates[plot_idx];
    plot_update.clear_plot();
    
    plot_update.new_points_y.resize(length);
    for (uint64_t i = 0; i < length; ++i) {
        plot_update.new_points_y[i] = numbers[i];
    }
    plot_update.contains_numbers = true;
    plot_update.empty_update = false;

    g_plot_state_update_mutex.unlock();
    return true;
}

PLOTAPI bool plot_fill_points_x_y(uint32_t plot_idx, double* points_x, double* points_y, uint64_t length)
{
    if (!valid_plot_idx(plot_idx)) return false;
    g_plot_state_update_mutex.lock();

    Plot_Update& plot_update = g_plot_state_update.plot_updates[plot_idx];
    plot_update.clear_plot();

    plot_update.new_points_x.resize(length);
    plot_update.new_points_y.resize(length);
    for (uint64_t i = 0; i < length; ++i) {
        plot_update.new_points_x[i] = points_x[i];
        plot_update.new_points_y[i] = points_y[i];
    }
    plot_update.contains_points = true;
    plot_update.empty_update = false;

    g_plot_state_update_mutex.unlock();
    return true;    
}

PLOTAPI bool plot_fill_points_xy(uint32_t plot_idx, double* points_xy, uint64_t length)
{
    if (!valid_plot_idx(plot_idx)) return false;
    if (length % 2 != 0) {
        printf(ERROR "The length provided is not divisible by 2 but 'plot_fill_points_xy' expects an array of Points.\n");
        return false;
    }
    g_plot_state_update_mutex.lock();

    Plot_Update& plot_update = g_plot_state_update.plot_updates[plot_idx];
    plot_update.clear_plot();

    plot_update.new_points_x.resize(length / 2);
    plot_update.new_points_y.resize(length / 2);
    for (uint64_t i = 0; i < length / 2; ++i) {
        plot_update.new_points_x[i] = points_xy[i * 2];
        plot_update.new_points_y[i] = points_xy[i * 2 + 1];
    }
    plot_update.contains_points = true;
    plot_update.empty_update = false;

    g_plot_state_update_mutex.unlock();
    return true;    
}

PLOTAPI bool plot_append_number(uint32_t plot_idx, double number)
{
    if (!valid_plot_idx(plot_idx)) return false;
    g_plot_state_update_mutex.lock();
    
    Plot_Update& plot_update = g_plot_state_update.plot_updates[plot_idx];
    if (plot_update.contains_points) {
        printf(ERROR "The Plot with index '%d' contains points and cannot be appended with the number '%f'.\n", plot_idx, number);
        return false;
    }
    
    plot_update.new_points_y.push_back(number);
    plot_update.contains_numbers = true;
    plot_update.empty_update = false;

    g_plot_state_update_mutex.unlock();
    return true;    
}

PLOTAPI bool plot_append_numbers(uint32_t plot_idx, double* numbers, uint64_t length)
{
    if (!valid_plot_idx(plot_idx)) return false;
    g_plot_state_update_mutex.lock();
    
    Plot_Update& plot_update = g_plot_state_update.plot_updates[plot_idx];
    if (plot_update.contains_points) {
        printf(ERROR "The Plot with index '%d' contains points and cannot be appended with numbers.\n", plot_idx);
        return false;
    }

    uint64_t old_size = plot_update.new_points_y.size();
    plot_update.new_points_y.resize(old_size + length);
    for (uint64_t i = 0; i < length; ++i) {
        plot_update.new_points_y[old_size + i] = numbers[i];
    }
    plot_update.contains_numbers = true;
    plot_update.empty_update = false;

    g_plot_state_update_mutex.unlock();
    return true;
}

PLOTAPI bool plot_append_point(uint32_t plot_idx, double point_x, double point_y)
{
    if (!valid_plot_idx(plot_idx)) return false;
    g_plot_state_update_mutex.lock();
    
    Plot_Update& plot_update = g_plot_state_update.plot_updates[plot_idx];
    if (plot_update.contains_numbers) {
        printf(ERROR "The Plot with index '%d' contains numbers and cannot be appended with the point '(%f, %f)'.\n", plot_idx, point_x, point_y);
        return false;
    }

    plot_update.new_points_x.push_back(point_x);
    plot_update.new_points_y.push_back(point_y);
    plot_update.contains_points = true;
    plot_update.empty_update = false;

    g_plot_state_update_mutex.unlock();
    return true;    
}

PLOTAPI bool plot_append_points_x_y(uint32_t plot_idx, double* points_x, double* points_y, uint64_t length)
{
    if (!valid_plot_idx(plot_idx)) return false;
    g_plot_state_update_mutex.lock();
    
    Plot_Update& plot_update = g_plot_state_update.plot_updates[plot_idx];
    if (plot_update.contains_numbers) {
        printf(ERROR "The Plot with index '%d' contains numbers and cannot be appended with points.\n", plot_idx);
        return false;
    }

    uint64_t old_size = plot_update.new_points_y.size();
    plot_update.new_points_x.resize(old_size + length);
    plot_update.new_points_y.resize(old_size + length);
    for (uint64_t i = 0; i < length; ++i) {
        plot_update.new_points_x[old_size + i] = points_x[i];
        plot_update.new_points_y[old_size + i] = points_y[i];
    }
    plot_update.contains_points = true;
    plot_update.empty_update = false;

    g_plot_state_update_mutex.unlock();
    return true;
}

PLOTAPI bool plot_append_points_xy(uint32_t plot_idx, double* points_xy, uint64_t length)
{
    if (!valid_plot_idx(plot_idx)) return false;
    if (length % 2 != 0) {
        printf(ERROR "The length provided is not divisible by 2 but 'plot_append_points_xy' expects an array of Points.\n");
        return false;
    }
    g_plot_state_update_mutex.lock();
    
    Plot_Update& plot_update = g_plot_state_update.plot_updates[plot_idx];
    if (plot_update.contains_numbers) {
        printf(ERROR "The Plot with index '%d' contains numbers and cannot be appended with points.\n", plot_idx);
        return false;
    }

    uint64_t old_size = plot_update.new_points_y.size();
    plot_update.new_points_x.resize(old_size + length / 2);
    plot_update.new_points_y.resize(old_size + length / 2);
    for (uint64_t i = 0; i < length / 2; ++i) {
        plot_update.new_points_x[old_size + i] = points_xy[i * 2];
        plot_update.new_points_y[old_size + i] = points_xy[i * 2 + 1];
    }
    plot_update.contains_points = true;
    plot_update.empty_update = false;

    g_plot_state_update_mutex.unlock();
    return true;
}

PLOTAPI bool plotgroup_show(uint32_t plotgroup_idx)
{
    if (!valid_group_idx(plotgroup_idx)) return false;
    g_plot_state_update_mutex.lock();

    g_plot_state_update.visible_group = plotgroup_idx;
    g_plot_state_update.window_visible = true;
            
    g_plot_state_update_mutex.unlock();
    start_gui_thread_if_not_started();
    return true;
}

PLOTAPI bool plotgroup_append(uint32_t plotgroup_idx, uint32_t plot_idx)
{
    if (!valid_group_idx(plotgroup_idx)) return false;
    if (!valid_plot_idx(plot_idx)) return false;
    g_plot_state_update_mutex.lock();

    Plot_Group_Update& group_update = g_plot_state_update.plot_group_updates[plotgroup_idx];

    group_update.new_plots.push_back(plot_idx);
    group_update.empty_update = false;
            
    g_plot_state_update_mutex.unlock();
    return true;
}

PLOTAPI bool plotgroup_remove(uint32_t plotgroup_idx, uint32_t plot_idx)
{
    if (!valid_group_idx(plotgroup_idx)) return false;
    if (!valid_plot_idx(plot_idx)) return false;
    g_plot_state_update_mutex.lock();

    Plot_Group_Update& group_update = g_plot_state_update.plot_group_updates[plotgroup_idx];

    group_update.remove_plots.push_back(plot_idx);
    group_update.empty_update = false;
            
    g_plot_state_update_mutex.unlock();
    return true;
}

PLOTAPI bool plotgroup_clear(uint32_t plotgroup_idx)
{
    if (!valid_group_idx(plotgroup_idx)) return false;
    g_plot_state_update_mutex.lock();

    g_plot_state_update.plot_group_updates[plotgroup_idx].clear_group();
            
    g_plot_state_update_mutex.unlock();
    return true;    
}

PLOTAPI bool plotgroup_set_name(uint32_t plotgroup_idx, const char* name)
{
    if (!valid_group_idx(plotgroup_idx)) return false;
    g_plot_state_update_mutex.lock();

    Plot_Group_Update& group_update = g_plot_state_update.plot_group_updates[plotgroup_idx];

    group_update.new_name = new char[strlen(name) + 1];
    strcpy(group_update.new_name, name);
    group_update.empty_update = false;
            
    g_plot_state_update_mutex.unlock();
    return true;    
}
