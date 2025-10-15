#include "plotlib.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cassert>
#include <cmath>

#include <limits>
#include <vector>
#include <mutex>
#include <thread>
#include <functional>

namespace rl {
#include "raylib/raylib.h"
#include "raylib/raymath.h"
}

// User-changeable constants

#define DEFAULT_FPS 120
#define DEFAULT_WINDOW_WIDTH 650
#define DEFAULT_WINDOW_HEIGHT 500

#define SCROLL_ZOOM_FACTOR 0.5
#define EXCESSIVE_TRAILING_ZEROS_THRESHOLD 4               // replace traling zeros for string representations of numbers with '+eX'
#define DEFAULT_ZOOM_TO_ZERO_PIXEL_DISTANCE_THRESHOLD 20   // Zoom to zero instead to the curso position if the cursor is close to zero

#define MIN_PLOT_SCREEN_TO_BOUNDS_OFFSET 8
#define DEFAULT_TICK_MARK_LEN 5              // The length of the little marks at the tick-positions
#define MAX_TICK_MARK_TEXT_LENGTH 30
#define MAX_TICK_MARK_COUNT 32

#define PRECISION_SAFTEY_FACTOR 100.0        // How many times should the plot-range be larger than the smallest representable length

#define PLOTTER3D_PAN_FACTOR 0.008
#define PLOTTER3D_ROTATE_FACTOR 0.01

// Implementation constants

#define MAX_DOUBLE 1.7976931348623157e308

#define INFO "PLOTLIB INFO: "
#define WARNING "PLOTLIB WARNING: "
#define ERROR "PLOTLIB ERROR: "

#define INVALID_IDX ((uint32_t)~0) // Max uint32 value

extern const unsigned char gui_font_binary_ttf[];
extern const unsigned int gui_font_binary_ttf_len;

typedef uint32_t Idx_Type;

typedef Idx_Type Plot_Idx;
typedef Idx_Type Plotter_Idx;
typedef Idx_Type Plot3d_Idx;
typedef Idx_Type Plotter3d_Idx;
typedef Idx_Type Panel_Idx;
typedef Idx_Type Tile_Idx;

bool valid_plot_idx(Plot_Idx plot_idx) {
    if (plot_idx <= PLOTLIB_MAX_PLOT_IDX) return true;
    printf(ERROR "The plot index '%u' is not within the valid fixed range of [0, %d]\n", plot_idx, PLOTLIB_MAX_PLOT_IDX);
    return false;
}

bool valid_plotter_idx(Plotter_Idx plotter_idx) {
    if (plotter_idx <= PLOTLIB_MAX_PLOTTER_IDX) return true;
    printf(ERROR "The plotter index '%u' is not within the valid fixed range of [0, %d]\n", plotter_idx, PLOTLIB_MAX_PLOTTER_IDX);
    return false;
}

bool valid_plot3d_idx(Plot3d_Idx plot3d_idx) {
    if (plot3d_idx <= PLOTLIB_MAX_PLOT3D_IDX) return true;
    printf(ERROR "The plot3d index '%u' is not within the valid fixed range of [0, %d]\n", plot3d_idx, PLOTLIB_MAX_PLOT3D_IDX);
    return false;
}

bool valid_plotter3d_idx(Plotter3d_Idx plotter3d_idx) {
    if (plotter3d_idx <= PLOTLIB_MAX_PLOTTER3D_IDX) return true;
    printf(ERROR "The plotter3d index '%u' is not within the valid fixed range of [0, %d]\n", plotter3d_idx, PLOTLIB_MAX_PLOTTER3D_IDX);
    return false;
}

bool valid_panel_idx(Panel_Idx panel_idx) {
    if (panel_idx <= PLOTLIB_MAX_PANEL_IDX) return true;
    printf(ERROR "The panel index '%u' is not within the valid fixed range of [0, %d]\n", panel_idx, PLOTLIB_MAX_PANEL_IDX);
    return false;
}

struct Color {
    uint8_t r, g, b, a;
};

static const Color plot_color_table[] = {
    Color{0x5e, 0x6a, 0xea, 0xff}, // BLUE
    Color{0x6a, 0xbd, 0x3c, 0xff}, // GREEN
    Color{0xeb, 0x35, 0x45, 0xff}, // RED
    Color{0xb0, 0x4c, 0xe7, 0xff}, // PURPLE
    Color{0xf1, 0xa1, 0x29, 0xff}, // ORANGE
    Color{0xe4, 0xe6, 0x5c, 0xff}, // YELLOW
    Color{0x9e, 0xbc, 0xde, 0xff}, // BLUE_LIGHT
    Color{0x95, 0xde, 0x85, 0xff}, // GREEN_LIGHT
    Color{0xec, 0x73, 0x8e, 0xff}, // RED_LIGHT
    Color{0xc0, 0x92, 0xff, 0xff}, // PURPLE_LIGHT
    Color{0xeb, 0xba, 0x6f, 0xff}, // ORANGE_LIGHT
    Color{0xfe, 0xff, 0xb2, 0xff}, // YELLOW_LIGHT
    Color{0x39, 0x34, 0xa4, 0xff}, // BLUE_DARK
    Color{0x4a, 0x6d, 0x22, 0xff}, // GREEN_DARK
    Color{0x75, 0x28, 0x28, 0xff}, // RED_DARK
    Color{0x69, 0x1c, 0xac, 0xff}, // PURPLE_DARK
    Color{0xc4, 0x60, 0x00, 0xff}, // ORANGE_DARK
    Color{0xbf, 0xb6, 0x00, 0xff}, // YELLOW_DARK
};

static const size_t plot_color_table_size = sizeof(plot_color_table) / sizeof(Color);

static rl::Color to_rl_color(Color color) {
    return rl::Color{ color.r, color.g, color.b, color.a };
}

struct Range_XY {
    double x_begin, x_end, y_begin, y_end;
};

struct Plot {
    std::vector<double> points_x;
    std::vector<double> points_y;

    Color color;
    bool show_lines = true;
    double line_width = 1.0;
    bool show_points = false;
    double point_diameter = 3.0;
    
    char* label = nullptr;
    
    Range_XY bbox = { MAX_DOUBLE, -MAX_DOUBLE, MAX_DOUBLE, -MAX_DOUBLE }; // bounding box
    
    bool has_x_coordinate() { return !points_x.empty() && points_x.size() == points_y.size(); }
    bool empty() { return points_x.empty() && points_y.empty(); }

    struct {
        std::vector<double> new_points_x;
        std::vector<double> new_points_y;
        char* new_name = nullptr;

        bool has_custom_color = false;
        Color custom_color;
        bool show_lines = true;
        double line_width = 1.0;
        bool show_points = false;
        double point_diameter = 3.0;

        bool no_changes = true; // true -> safe to skip the synchronization
        bool was_cleared = false;

        bool contains_points = false;
        bool contains_numbers = false;

        bool accepts_numbers() { return contains_numbers || !contains_points; }
        bool accepts_points() { return contains_points || !contains_numbers; }

        uint64_t plot_length = 0;
    
        // Resets everything which was just a delta and not a mirror of the actual state.
        void reset_after_sync() {
            new_points_x.clear();
            new_points_y.clear();
            has_custom_color = false;
        
            delete new_name;
            new_name = nullptr;
        
            was_cleared = false;
            no_changes = true;
        }

        void clear_plot() {
            new_points_x.clear();
            new_points_y.clear();
            contains_points = false;
            contains_numbers = false;
            was_cleared = true;
            no_changes = false;
        }
    } shared;

    void initialize(Plot_Idx plot_idx);
    void synchronize_with_shared_state(Plot_Idx plot_idx);
};

struct Legend_Item {
    enum : uint32_t {
        NORMAL,
        SUB_HEADER,
        MAIN_HEADER,
    };
    uint32_t type = NORMAL;
    char* text;
    rl::Color color;
};

struct Plotter_Visualization_Mode {
    enum : uint32_t {
        NONE,
        INTERACTIVE,
        TRACK_ALL,
        TRACK_LATEST_VALUES,
        TRACK_LATEST_VALUES_X_RANGE,
        TRACK_LATEST_VALUES_XY_RANGE,
        TRACK_SPECIFIC_PLOT,
    };
    uint32_t type = NONE;
    uint64_t n_points = 0;
    double x_range = 0;
    double y_range = 0;
    Plot_Idx specific_plot = INVALID_IDX;
};

struct Plotter {
    std::vector<Plot_Idx> plots;
    char* label = nullptr;
    Plotter_Visualization_Mode vis_mode = { .type=Plotter_Visualization_Mode::TRACK_ALL };
    int zoom_resize_cooldown = 0;

    Range_XY plot_range = Range_XY{};
    rl::Rectangle plot_screen = rl::Rectangle{ 0.0f, 0.0f, 1.0f, 1.0f };

    struct {
        std::vector<Plot_Idx> new_plots;
        std::vector<Plot_Idx> remove_plots;
        char* new_name = nullptr;
        Plotter_Visualization_Mode vis_mode{ .type=Plotter_Visualization_Mode::TRACK_ALL };
    
        bool no_changes = true;
        bool was_cleared = false;

        void reset_after_sync() {
            new_plots.clear();
            remove_plots.clear();

            delete new_name;
            new_name = nullptr;
        
            was_cleared = false;
            no_changes = true;
        }

        void clear_plotter() {
            new_plots.clear();
            remove_plots.clear();
            was_cleared = true;
            no_changes = false;
        }
    } shared;

    void initialize(Plotter_Idx plotter_idx);
    void synchronize_with_shared_state(Plotter_Idx plotter_idx);
    void get_legend_items(std::vector<Legend_Item>& legend_items);
    void draw(rl::Rectangle bounds, bool tile_in_focus);
};

struct Vertex {
    float x, y, z;
    Color color;
};

constexpr rl::Matrix transform_identity = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
};

struct Plot3d {
    rl::Mesh rl_mesh = {};
    rl::Matrix transform = transform_identity;
    char* label = nullptr;
    std::vector<Vertex> vertices;
    rl::Vector3 midpoint = {0.0f, 0.0f, 0.0f};

    enum : uint32_t {
        NONE,
        SPHERES,
        LINES,
        CONTINUOUS_LINE,
        TRIANGLES,
    };
    uint32_t type = SPHERES;
    float sphere_diameter = 3.0f;

    struct {
        std::vector<Vertex> new_vertices;
        std::vector<uint32_t> new_indices;
        char* new_name = nullptr;

        uint32_t type = SPHERES;
        float line_width = 1.0f;
        float sphere_diameter = 3.0f;

        rl::Matrix transform = transform_identity;

        bool no_changes = true;
        bool was_cleared = false;

        void reset_after_sync() {
            new_vertices.clear();
            new_indices.clear();

            delete new_name;
            new_name = nullptr;
        
            was_cleared = false;
            no_changes = true;
        }

        void clear_plot3d() {
            new_vertices.clear();
            new_indices.clear();
            was_cleared = true;
            no_changes = false;
        }
    } shared;

    void initialize(Plot3d_Idx plot3d_idx);
    void synchronize_with_shared_state(Plot3d_Idx plot3d_idx);
};

struct Plotter3d_Visualization_Mode {
    enum : uint32_t {
        NONE,
        FREE_CAMERA,
        TRACK_POINT,
        TRACK_PLOT3D_RELATIVE_POINT,
        TRACK_PLOT3D_MIDPOINT,
        TRACK_LATEST_PLOT3D_VERTEX,
    };
    uint32_t type = NONE;

    rl::Vector3 track_point = {0.0f, 0.0f, 0.0f};
    rl::Vector3 relative_point = {0.0f, 0.0f, 0.0f};

    bool auto_rotate = false;
    float auto_rotate_deg_per_s = 45;

    Plot3d_Idx plot3d_idx = INVALID_IDX;
};

struct Plotter3d {
    std::vector<Plot3d_Idx> plots3d;
    
    rl::Camera3D rl_camera = {
        .position = {-5.0, 0.0, 0.0},
        .target   = {0.0, 0.0, 0.0},
        .up       = {0.0, 0.0, -1.0},
        .fovy = 60,
        .projection = rl::CAMERA_PERSPECTIVE
    };
    
    float prev_render_target_width = 0;
    float prev_render_target_height = 0;
    rl::RenderTexture2D render_target = {};

    char* label = nullptr;

    Plotter3d_Visualization_Mode vis_mode = { .type=Plotter3d_Visualization_Mode::FREE_CAMERA };

    struct {
        std::vector<Plot3d_Idx> new_plots3d;
        std::vector<Plot3d_Idx> remove_plots3d;
        char* new_name = nullptr;
        Plotter3d_Visualization_Mode vis_mode = { .type=Plotter3d_Visualization_Mode::FREE_CAMERA };
        bool ortho_projection = false;
        float FOV = 60;

        bool no_changes = true;
        bool was_cleared = false;

        void reset_after_sync() {
            new_plots3d.clear();
            remove_plots3d.clear();
            
            delete new_name;
            new_name = nullptr;
        
            was_cleared = false;
            no_changes = true;
        }
        
        void clear_plotter3d() {
            new_plots3d.clear();
            remove_plots3d.clear();
            was_cleared = true;
            no_changes = false;
        }
    } shared;

    void initialize(Plotter3d_Idx plotter3d_idx);
    void synchronize_with_shared_state(Plotter3d_Idx plotter3d_idx);
    void get_legend_items(std::vector<Legend_Item>& legend_items);
    void draw(rl::Rectangle bounds, bool tile_in_focus);
};

struct Panel_Tile {
    enum : uint32_t {
        EMPTY,
        PLOTTER,
        PLOTTER3D,
        LEFTRIGHT,
        TOPBOTTOM,
    };
    uint32_t type = EMPTY;

    bool automatic_tile_insertion = false;
    uint32_t tile_idx = INVALID_IDX;

    Plotter_Idx plotter_idx = INVALID_IDX;
    Plotter3d_Idx plotter3d_idx = INVALID_IDX;

    Panel_Tile* first = nullptr;  // left or top
    Panel_Tile* second = nullptr; // right or bottom
    float split_weight = 0.5;

    ~Panel_Tile() {
        delete first;
        delete second;
    }
    void get_legend_items(std::vector<Legend_Item>& legend_items);
    void draw(rl::Rectangle bounds, Tile_Idx& focused_tile);
};

struct Panel {
    Panel_Tile* root_tile = nullptr;
    char* label = nullptr;

    Idx_Type next_tile_idx = 0; // 0 -> root_tile (after initialization)

    Tile_Idx focused_tile = INVALID_IDX; // The tile that was last selected/clicked on by the user

    struct {
        std::vector<Panel_Tile> new_tiles;
        std::vector<Panel_Tile> remove_tiles;
        const char* new_name = nullptr;

        bool no_changes = true;
        bool was_cleared = false;

        void reset_after_sync() {
            new_tiles.clear();
            remove_tiles.clear();

            delete new_name;
            new_name = nullptr;
        
            was_cleared = false;
            no_changes = true;
        }

        void clear_panel() {
            new_tiles.clear();
            remove_tiles.clear();
        
            was_cleared = true;
            no_changes = false;
        }
    } shared;

    void initialize(Panel_Idx panel_idx);
    void draw(rl::Rectangle bounds);
    void synchronize_with_shared_state(Panel_Idx panel_idx);
};

struct Theme_Colors {
    rl::Color text;
    rl::Color text_low_alpha;
    rl::Color backgound;
    rl::Color tick_lines;
    rl::Color coordinate_axes;
    rl::Color border;
};

static const Theme_Colors dark_theme_colors = {
    .text               = { 0xff, 0xff, 0xff, 0xff},
    .text_low_alpha     = { 0xff, 0xff, 0xff, 0x90},
    .backgound          = { 0x25, 0x25, 0x25, 0xff },
    .tick_lines         = { 0xff, 0xff, 0xff, 0x10 },
    .coordinate_axes    = { 0xff, 0xff, 0xff, 0x40 },
    .border = { 0xff, 0xff, 0xff, 0xff },
};

static const Theme_Colors light_theme_colors = {
    .text               = { 0x00, 0x00, 0x00, 0xff},
    .text_low_alpha     = { 0x00, 0x00, 0x00, 0x90},
    .backgound          = { 0xfd, 0xfd, 0xfd, 0xff },
    .tick_lines         = { 0x00, 0x00, 0x00, 0x10 },
    .coordinate_axes    = { 0x00, 0x00, 0x00, 0x40 },
    .border = { 0x00, 0x00, 0x00, 0xff },
};

struct API_Abstraction_Level {
    enum : uint32_t {
        JUST_PLOTS   = 0,
        PLOTTERS     = 1,
        PANELS       = 2,
    };
    uint32_t type;
    
    void set_level(uint32_t level) {
        if (type < level) type = level;
    }
    bool level_reached(uint32_t level) {
        return level <= type;
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
    int tick_mark_len = DEFAULT_TICK_MARK_LEN;
    
    // float plot_screen_border_width = 1;
    float offset_normal = 5;
    float offset_small = 2;
    float panel_header_gap = 24;
    
    float zoom_to_zero_pixel_distance_threshold = DEFAULT_ZOOM_TO_ZERO_PIXEL_DISTANCE_THRESHOLD;

    Panel_Idx visible_panel = 0;

    Theme_Colors colors;
    API_Abstraction_Level api_abstraction_level = { .type=API_Abstraction_Level::JUST_PLOTS };

    bool interactive_on_touch_waiting = false;

    bool window_is_init = false;
    bool window_visible = false;
    bool terminate = false;

    struct {
        Panel_Idx visible_panel = 0;

        Theme_Colors colors = dark_theme_colors;
        API_Abstraction_Level api_abstraction_level = { .type=API_Abstraction_Level::JUST_PLOTS };

        bool interactive_on_touch = false;

        bool window_visible = false;
        bool terminate = false;

        void reset_after_sync() {
            interactive_on_touch = false;
            terminate = false;
        }
    } shared;

    void synchronize_with_shared_state();
};

struct Plotlib_State {
    Plot plots[PLOTLIB_MAX_PLOT_IDX + 1];
    Plotter plotters[PLOTLIB_MAX_PLOTTER_IDX + 1];
    Plot3d plots3d[PLOTLIB_MAX_PLOT3D_IDX + 1];
    Plotter3d plotters3d[PLOTLIB_MAX_PLOTTER3D_IDX + 1];
    Panel panels[PLOTLIB_MAX_PANEL_IDX + 1];
    Gui gui;
};

static Plotlib_State gps; // Global Plotlib State
static std::mutex gps_shared_mutex;

struct Scoped_GPS_Mutext_Lock {
    Scoped_GPS_Mutext_Lock() { gps_shared_mutex.lock(); }
    ~Scoped_GPS_Mutext_Lock() { gps_shared_mutex.unlock(); }
};

////////////////////////////////////////////
// Initialization
////////////////////////////////////////////

void Plot::initialize(Plot_Idx plot_idx)
{
    const int label_len = snprintf(nullptr, 0, "[%d] Plot", plot_idx) + 1;
    label = new char[label_len];
    snprintf(label, label_len, "[%d] Plot", plot_idx);
            
    color = plot_color_table[plot_idx % plot_color_table_size];
}

void Plotter::initialize(Plotter_Idx plotter_idx)
{
    int label_len = snprintf(nullptr, 0, "[%d] Plotter", plotter_idx) + 1;
    label = new char[label_len];
    snprintf(label, label_len, "[%d] Plotter", plotter_idx);
}

void Plot3d::initialize(Plot3d_Idx plot3d_idx)
{
    const int label_len = snprintf(nullptr, 0, "[%d] 3D Plot", plot3d_idx) + 1;
    label = new char[label_len];
    snprintf(label, label_len, "[%d] 3D Plot", plot3d_idx);
}

void Plotter3d::initialize(Plotter3d_Idx plotter3d_idx)
{
    const int label_len = snprintf(nullptr, 0, "[%d] 3D Plotter", plotter3d_idx) + 1;
    label = new char[label_len];
    snprintf(label, label_len, "[%d] 3D Plotter", plotter3d_idx);
}

void Panel::initialize(Panel_Idx panel_idx)
{
    // if (panel_idx == 0) {
    //     root_tile = new Panel_Tile{ .type=Panel_Tile::PLOTTER, .tile_idx=next_tile_idx++, .plotter_idx=0 };
    // }
    // else {
    root_tile = new Panel_Tile{ .type=Panel_Tile::EMPTY, .tile_idx=next_tile_idx++ };
    // }
        
    int label_len = snprintf(nullptr, 0, "[%d] Panel", panel_idx) + 1;
    label = new char[label_len];
    snprintf(label, label_len, "[%d] Panel", panel_idx);
}

static void initialize_plotlib_state()
{
    Scoped_GPS_Mutext_Lock lock;
    
    for (Plot_Idx plot_idx = 0; plot_idx <= PLOTLIB_MAX_PLOT_IDX; ++plot_idx) {
        gps.plots[plot_idx].initialize(plot_idx);
    }

    for (Plotter_Idx plotter_idx = 0; plotter_idx <= PLOTLIB_MAX_PLOTTER_IDX; ++plotter_idx) {
        gps.plotters[plotter_idx].initialize(plotter_idx);
    }

    for (Plot3d_Idx plot3d_idx = 0; plot3d_idx <= PLOTLIB_MAX_PLOT3D_IDX; ++plot3d_idx) {
        gps.plots3d[plot3d_idx].initialize(plot3d_idx);
    }

    for (Plotter3d_Idx plotter3d_idx = 0; plotter3d_idx <= PLOTLIB_MAX_PLOTTER3D_IDX; ++plotter3d_idx) {
        gps.plotters3d[plotter3d_idx].initialize(plotter3d_idx);
    }

    for (Panel_Idx panel_idx = 0; panel_idx <= PLOTLIB_MAX_PANEL_IDX; ++panel_idx) {
        gps.panels[panel_idx].initialize(panel_idx);
    }
}

////////////////////////////////////////////
// Synchronization
////////////////////////////////////////////

void Gui::synchronize_with_shared_state()
{
    if (shared.interactive_on_touch) {
        interactive_on_touch_waiting = true;
    }
        
    visible_panel = shared.visible_panel;
    window_visible = shared.window_visible;
    terminate = shared.terminate;
    colors = shared.colors;
}

void Plot::synchronize_with_shared_state(Plot_Idx plot_idx)
{
    if (shared.has_custom_color) {
        color = shared.custom_color;
    }

    show_lines = shared.show_lines;
    line_width = shared.line_width;
    show_points = shared.show_points;
    point_diameter = shared.point_diameter;

    if (shared.new_name) {
        int label_len = snprintf(nullptr, 0, "[%d] %s", plot_idx, shared.new_name) + 1;
        delete label;
        label = new char[label_len];
        snprintf(label, label_len, "[%d] %s", plot_idx, shared.new_name);
    }

    uint64_t old_length = points_y.size();
    uint64_t new_length = old_length;
    if (shared.was_cleared || old_length == 0) {
        new_length = 0;
        bbox.x_begin = MAX_DOUBLE;
        bbox.x_end = -MAX_DOUBLE;
        bbox.y_begin = MAX_DOUBLE;
        bbox.y_end = -MAX_DOUBLE;
    }
    uint64_t new_points_offset = new_length;
    new_length += shared.new_points_y.size();
        
    if (shared.contains_points) {
        assert(shared.new_points_x.size() == shared.new_points_y.size());
        points_x.resize(new_length);
        points_y.resize(new_length);

        for (uint64_t i = 0; i < shared.new_points_y.size(); ++i) {
            points_x[i + new_points_offset] = shared.new_points_x[i];
            points_y[i + new_points_offset] = shared.new_points_y[i];

            bbox.x_begin = shared.new_points_x[i] < bbox.x_begin ? shared.new_points_x[i] : bbox.x_begin;
            bbox.x_end = shared.new_points_x[i] > bbox.x_end ? shared.new_points_x[i] : bbox.x_end;
            bbox.y_begin = shared.new_points_y[i] < bbox.y_begin ? shared.new_points_y[i] : bbox.y_begin;
            bbox.y_end = shared.new_points_y[i] > bbox.y_end ? shared.new_points_y[i] : bbox.y_end;
        }
    }
    else if (shared.contains_numbers) {
        assert(shared.new_points_x.size() == 0);
        points_y.resize(new_length);

        bbox.x_begin = 0;
        bbox.x_end = points_y.size() == 0 ? 0 : points_y.size() - 1;
        for (uint64_t i = 0; i < shared.new_points_y.size(); ++i) {
            points_y[i + new_points_offset] = shared.new_points_y[i];

            bbox.y_begin = shared.new_points_y[i] < bbox.y_begin ? shared.new_points_y[i] : bbox.y_begin;
            bbox.y_end = shared.new_points_y[i] > bbox.y_end ? shared.new_points_y[i] : bbox.y_end;
        }
    }
    else {
        assert(new_length == 0);
        points_x.resize(new_length);
        points_y.resize(new_length);
    }

    shared.plot_length = points_y.size();
}

void Plotter::synchronize_with_shared_state(Plotter_Idx plotter_idx)
{
    vis_mode = shared.vis_mode;

    if (shared.new_name) {
        int label_len = snprintf(nullptr, 0, "[%d] %s", plotter_idx, shared.new_name) + 1;
        delete label;
        label = new char[label_len];
        snprintf(label, label_len, "[%d] %s", plotter_idx, shared.new_name);
    }

    if (shared.was_cleared) {
        plots.clear();
    }

    for (size_t i = 0; i < shared.new_plots.size(); ++i) {
        bool already_contained = false;
        for (size_t j = 0; j < plots.size(); ++j) {
            already_contained |= plots[j] == shared.new_plots[i];
        }
        if (!already_contained) {
            plots.push_back(shared.new_plots[i]);
        }
    }

    for (size_t i = 0; i < shared.remove_plots.size(); ++i) {
        for (size_t j = 0; j < plots.size(); ++j) {
            if (plots[j] == shared.remove_plots[i]) {
                plots.erase(plots.begin() + j);
            }
        }
    }
}

void Plot3d::synchronize_with_shared_state(Plot3d_Idx plot3d_idx)
{
    sphere_diameter = shared.sphere_diameter;
    transform = shared.transform;
    type = shared.type;
    
    if (shared.new_name) {
        int label_len = snprintf(nullptr, 0, "[%d] %s", plot3d_idx, shared.new_name) + 1;
        delete label;
        label = new char[label_len];
        snprintf(label, label_len, "[%d] %s", plot3d_idx, shared.new_name);
    }

    uint64_t old_length = vertices.size();
    if (shared.was_cleared) {
        old_length = 0;
        midpoint = rl::Vector3{0.0f, 0.0f, 0.0f};
    }
    uint64_t new_length = old_length + shared.new_vertices.size();
    vertices.resize(new_length);
    
    double sum_x = (double) midpoint.x * old_length;
    double sum_y = (double) midpoint.y * old_length;
    double sum_z = (double) midpoint.z * old_length;

    for (uint64_t i = 0; i < shared.new_vertices.size(); ++i)
    {
        vertices[i + old_length] = shared.new_vertices[i];
        sum_x += shared.new_vertices[i].x;
        sum_y += shared.new_vertices[i].y;
        sum_z += shared.new_vertices[i].z;
    }
    
    midpoint = rl::Vector3{ (float) (sum_x / (double) new_length),
                            (float) (sum_y / (double) new_length),
                            (float) (sum_z / (double) new_length) };
}

void Plotter3d::synchronize_with_shared_state(Plotter3d_Idx plotter3d_idx)
{
    vis_mode = shared.vis_mode;

    rl_camera.fovy = shared.FOV;
    rl_camera.projection = shared.ortho_projection ? rl::CAMERA_ORTHOGRAPHIC : rl::CAMERA_PERSPECTIVE;

    if (shared.new_name) {
        int label_len = snprintf(nullptr, 0, "[%d] %s", plotter3d_idx, shared.new_name) + 1;
        delete label;
        label = new char[label_len];
        snprintf(label, label_len, "[%d] %s", plotter3d_idx, shared.new_name);
    }

    if (shared.was_cleared) {
        plots3d.clear();
    }

    for (size_t i = 0; i < shared.new_plots3d.size(); ++i) {
        bool already_contained = false;
        for (size_t j = 0; j < plots3d.size(); ++j) {
            already_contained |= plots3d[j] == shared.new_plots3d[i];
        }
        if (!already_contained) {
            plots3d.push_back(shared.new_plots3d[i]);
        }
    }

    for (size_t i = 0; i < shared.remove_plots3d.size(); ++i) {
        for (size_t j = 0; j < plots3d.size(); ++j) {
            if (plots3d[j] == shared.remove_plots3d[i]) {
                plots3d.erase(plots3d.begin() + j);
                break;
            }
        }
    }
}

// O(N) search through the tile tree
static Panel_Tile* find_tile(Panel_Tile* tile, std::function<bool(Panel_Tile* tile)> condition_check)
{
    if (!tile) return nullptr;
    if (condition_check(tile)) return tile;
    if (tile->first) {
        Panel_Tile* after_first = find_tile(tile->first, condition_check);
        if (after_first) return after_first;
    }
    if (tile->second) {
        Panel_Tile* after_second = find_tile(tile->second, condition_check);
        if (after_second) return after_second;
    }
    return nullptr;
}

void Panel::synchronize_with_shared_state(Panel_Idx panel_idx)
{
    auto reset_tile_tree = [&](){
        delete root_tile;
        next_tile_idx = 0;
        root_tile = new Panel_Tile{ .type=Panel_Tile::EMPTY, .tile_idx=next_tile_idx++ };
    };

    if (shared.new_name) {
        int label_len = snprintf(nullptr, 0, "[%d] %s", panel_idx, shared.new_name) + 1;
        label = new char[label_len];
        snprintf(label, label_len, "[%d] %s", panel_idx, shared.new_name);
    }

    if (shared.was_cleared) reset_tile_tree();

    for (size_t i = 0; i < shared.new_tiles.size(); ++i)
    {
        if (shared.new_tiles[i].type == Panel_Tile::PLOTTER) {
            Panel_Tile* similar_tile = find_tile(root_tile, [&](Panel_Tile* tile) { return tile->plotter_idx == shared.new_tiles[i].plotter_idx; });
            if (similar_tile) continue;
        }
        else if ( shared.new_tiles[i].type == Panel_Tile::PLOTTER3D) {
            Panel_Tile* similar_tile = find_tile(root_tile, [&](Panel_Tile* tile) { return tile->plotter3d_idx == shared.new_tiles[i].plotter3d_idx; });
            if (similar_tile) continue;
        }
        
        if (shared.new_tiles[i].automatic_tile_insertion)
        {
            // Try to find an empty (leaf) tile first
            Panel_Tile* empty_tile = find_tile(root_tile, [&](Panel_Tile* tile){ return tile->type == Panel_Tile::EMPTY; });
            if (empty_tile) {
                *empty_tile = Panel_Tile{shared.new_tiles[i]};
                empty_tile->tile_idx = next_tile_idx++;
                continue;
            }
            
            Panel_Tile* new_tile = new Panel_Tile{shared.new_tiles[i]};
            new_tile->tile_idx = next_tile_idx++;
            // TODO: be smarter about the tile split direction!
            Panel_Tile* new_root = new Panel_Tile{ .type=Panel_Tile::LEFTRIGHT, .tile_idx=next_tile_idx++, .first=root_tile, .second=new_tile };
            root_tile = new_root;
            continue;
        }
        
        Tile_Idx tile_idx = shared.new_tiles[i].tile_idx;
        assert(tile_idx != INVALID_IDX);
                
        Panel_Tile* tile = nullptr;
        if (tile_idx < next_tile_idx) {
            tile = find_tile(root_tile, [=](Panel_Tile* tile) { return tile->tile_idx == tile_idx; });
        }

        if (!tile) {
            printf(ERROR "The Tile could not be added at index '%u' because that tile index does not exist for Panel [%u]\n", tile_idx, panel_idx);
            continue;
        }

        if (tile == root_tile) {
            reset_tile_tree(); // invalidates tile pointer
            tile = root_tile;
        }

        tile->~Panel_Tile(); // delete children
        *tile = shared.new_tiles[i];

        if (tile->type == Panel_Tile::LEFTRIGHT || tile->type == Panel_Tile::TOPBOTTOM) {
            tile->first = new Panel_Tile{ .type=Panel_Tile::EMPTY, .tile_idx=next_tile_idx++ };
            tile->second = new Panel_Tile{ .type=Panel_Tile::EMPTY, .tile_idx=next_tile_idx++ };
        }

    }

    for (size_t i = 0; i < shared.remove_tiles.size(); ++i)
    {
        Panel_Tile& remove_tile = shared.remove_tiles[i];
        
        Panel_Tile* tile = nullptr;
        if (remove_tile.type == Panel_Tile::PLOTTER) {
            tile = find_tile(root_tile, [&](Panel_Tile* tile) { return tile->plotter_idx == remove_tile.plotter_idx; });
        }
        else if ( remove_tile.type == Panel_Tile::PLOTTER3D) {
            tile = find_tile(root_tile, [&](Panel_Tile* tile) { return tile->plotter3d_idx == remove_tile.plotter3d_idx; });
        }

        if (!tile) {
            if (remove_tile.type == Panel_Tile::PLOTTER) {
                printf(ERROR "The Plotter [%u] could not be deleted, because its not part of Panel [%u]\n", remove_tile.plotter_idx, panel_idx);
            }
            else if ( remove_tile.type == Panel_Tile::PLOTTER3D) {
                printf(ERROR "The 3D Plotter [%u] could not be deleted, because its not part of Panel [%u]\n", remove_tile.plotter3d_idx, panel_idx);
            }
            continue;
        }

        if (tile == root_tile) {
            reset_tile_tree();
            break;
        }
        tile->~Panel_Tile(); // delete children (not necessary actually)
        tile->type = Panel_Tile::EMPTY;
    }
}

static void synchronize_plotlib_state()
{
    Scoped_GPS_Mutext_Lock lock;
    
    gps.gui.synchronize_with_shared_state();
    gps.gui.shared.reset_after_sync();

    if (!gps.gui.window_visible) {
        gps_shared_mutex.unlock();
        return;
    }
    
    for (Plot_Idx plot_idx = 0; plot_idx <= PLOTLIB_MAX_PLOT_IDX; ++plot_idx)
    {
        Plot& plot = gps.plots[plot_idx];
        if (plot.shared.no_changes) continue;
        plot.synchronize_with_shared_state(plot_idx);
        plot.shared.reset_after_sync();
    }

    for (Plotter_Idx plotter_idx = 0; plotter_idx <= PLOTLIB_MAX_PLOTTER_IDX; ++plotter_idx)
    {
        Plotter& plotter = gps.plotters[plotter_idx];
        if (plotter.shared.no_changes) continue;
        plotter.synchronize_with_shared_state(plotter_idx);
        plotter.shared.reset_after_sync();
    }

    for (Plot3d_Idx plot3d_idx = 0; plot3d_idx <= PLOTLIB_MAX_PLOT3D_IDX; ++plot3d_idx)
    {
        Plot3d& plot3d = gps.plots3d[plot3d_idx];
        if (plot3d.shared.no_changes) continue;
        plot3d.synchronize_with_shared_state(plot3d_idx);
        plot3d.shared.reset_after_sync();
    }

    for (Plotter3d_Idx plotter3d_idx = 0; plotter3d_idx <= PLOTLIB_MAX_PLOTTER3D_IDX; ++plotter3d_idx)
    {
        Plotter3d& plotter3d = gps.plotters3d[plotter3d_idx];
        if (plotter3d.shared.no_changes) continue;
        plotter3d.synchronize_with_shared_state(plotter3d_idx);
        plotter3d.shared.reset_after_sync();
    }

    for (Panel_Idx panel_idx = 0; panel_idx <= PLOTLIB_MAX_PANEL_IDX; ++panel_idx)
    {
        Panel& panel = gps.panels[panel_idx];
        if (panel.shared.no_changes) continue;
        panel.synchronize_with_shared_state(panel_idx);
        panel.shared.reset_after_sync();
    }
}

////////////////////////////////////////////
// Drawing
////////////////////////////////////////////

static double linear_map(double x, double in_min, double in_max, double out_min, double out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static void plotter_update_plot_range_interactive_mode(Plotter* plotter, rl::Rectangle bounds, bool tile_in_focus)
{
    Range_XY& plot_range = plotter->plot_range;
    rl::Rectangle plot_screen = plotter->plot_screen;

    rl::Vector2 mouse_pos = rl::GetMousePosition();
    
    auto x_to_plotspace = [=](double x) -> double {
        return linear_map(x, (double) plot_screen.x, (double) plot_screen.x + plot_screen.width, plot_range.x_begin, plot_range.x_end);
    };

    auto y_to_plotspace = [=](double y) -> double {
        return linear_map(y, (double) plot_screen.y, (double) plot_screen.y + plot_screen.height, plot_range.y_end, plot_range.y_begin);
    };

    double plot_space_pan_x = 0;
    double plot_space_pan_y = 0;
    // Drag the plot_range only when the panel-tile is in focus.
    if (rl::IsMouseButtonDown(rl::MOUSE_LEFT_BUTTON) && tile_in_focus) {
        rl::Vector2 mouse_delta = rl::GetMouseDelta();
        plot_space_pan_x = x_to_plotspace(0) - x_to_plotspace(mouse_delta.x);
        plot_space_pan_y = y_to_plotspace(0) - y_to_plotspace(mouse_delta.y);
    }

    float mouse_wheel_delta = 0;
    // Zoom through scrolling only when the mouse hovers over the plotter.
    if (rl::CheckCollisionPointRec(mouse_pos, bounds)) {
        mouse_wheel_delta = rl::GetMouseWheelMove();
    }
    double zoom_factor_x = std::pow(1.2, (double) -mouse_wheel_delta * SCROLL_ZOOM_FACTOR);
    double zoom_factor_y = std::pow(1.2, (double) -mouse_wheel_delta * SCROLL_ZOOM_FACTOR);

    if (rl::IsKeyDown(rl::KEY_LEFT_CONTROL)) {
        zoom_factor_x = 1.0;
    }
    
    if (rl::IsKeyDown(rl::KEY_LEFT_SHIFT)) {
        zoom_factor_y = 1.0;
    }

    if (mouse_wheel_delta) {
        plotter->zoom_resize_cooldown = 2;
    }

    auto x_to_screenspace = [=](double x) -> float {
        return linear_map(x, plot_range.x_begin, plot_range.x_end, (double) plot_screen.x, (double) plot_screen.x + plot_screen.width);
    };

    auto y_to_screenspace = [=](double y) -> float {
        return linear_map(y, plot_range.y_begin, plot_range.y_end, (double) plot_screen.y + plot_screen.height, (double) plot_screen.y);
    };

    double zoom_center_x = x_to_plotspace(mouse_pos.x);
    double zoom_center_y = y_to_plotspace(mouse_pos.y);

    // prefer zooming to the origin / to the x=0, y=0 coordinate axes
    if (std::abs(x_to_screenspace(0) - mouse_pos.x) < gps.gui.zoom_to_zero_pixel_distance_threshold) {
        zoom_center_x = 0;
    }

    if (std::abs(y_to_screenspace(0) - mouse_pos.y) < gps.gui.zoom_to_zero_pixel_distance_threshold) {
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

static void plotter_generate_ticks(Ticks& ticks, rl::Rectangle bounds, Range_XY plot_range, int x_pixels_per_tick = gps.gui.x_pixels_per_tick)
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
    ticks.y_count = std::min((int) std::floor(bounds.height / gps.gui.y_pixels_per_tick), MAX_TICK_MARK_COUNT);

    ticks.x_spacing = calculate_tick_spacing(plot_range.x_begin, plot_range.x_end, ticks.x_count);
    ticks.x_begin = ceil(plot_range.x_begin / ticks.x_spacing) * ticks.x_spacing;
    ticks.y_spacing = calculate_tick_spacing(plot_range.y_begin, plot_range.y_end, ticks.y_count);
    ticks.y_begin = ceil(plot_range.y_begin / ticks.y_spacing) * ticks.y_spacing;

    int tick_idx = 0;
    for (double x = ticks.x_begin; x < plot_range.x_end + ticks.x_spacing/1e-3 && tick_idx < MAX_TICK_MARK_COUNT; x += ticks.x_spacing, ++tick_idx) {
        // this is a stupid hack to make sure 0 is dispayed cleanly, it works because 0 is always included as a tick,
        // if 0 is in the plot_range. So we can find it easily by comparing x to the tick-spacing
        if (std::abs(x) < ticks.x_spacing * 1e-3) x = 0.0;
        
        snprintf(ticks.x_text[tick_idx], MAX_TICK_MARK_TEXT_LENGTH, "%.14g", x);
        remove_excessive_trailing_zeros(ticks.x_text[tick_idx], MAX_TICK_MARK_TEXT_LENGTH);
        ticks.x_text_width[tick_idx] = MeasureTextEx(gps.gui.font_normal, ticks.x_text[tick_idx], gps.gui.fontsize_normal, gps.gui.fontspacing).x;
        ticks.x_text_width_max = ticks.x_text_width[tick_idx] > ticks.x_text_width_max ? ticks.x_text_width[tick_idx] : ticks.x_text_width_max;
    }

    // Regenerate the ticks if the text is too wide.
    if (ticks.x_text_width_max > x_pixels_per_tick && x_pixels_per_tick < 1000) {
        plotter_generate_ticks(ticks, bounds, plot_range, x_pixels_per_tick * 2);
        return;
    }

    tick_idx = 0;
    for (double y = ticks.y_begin; y < plot_range.y_end + ticks.y_spacing*1e-3 && tick_idx < MAX_TICK_MARK_COUNT; y += ticks.y_spacing, ++tick_idx) {
        if (std::abs(y) < ticks.y_spacing * 1e-3) y = 0.0;
        snprintf(ticks.y_text[tick_idx], MAX_TICK_MARK_TEXT_LENGTH, "%.14g", y);
        remove_excessive_trailing_zeros(ticks.y_text[tick_idx], MAX_TICK_MARK_TEXT_LENGTH);
        ticks.y_text_width[tick_idx] = MeasureTextEx(gps.gui.font_normal, ticks.y_text[tick_idx], gps.gui.fontsize_normal, gps.gui.fontspacing).x;
        ticks.y_text_width_max = ticks.y_text_width[tick_idx] > ticks.y_text_width_max ? ticks.y_text_width[tick_idx] : ticks.y_text_width_max;
    }
}

static Range_XY bounding_box_of_plot(Plot& plot, uint64_t begin_idx)
{
    Range_XY bbox = { MAX_DOUBLE, -MAX_DOUBLE, MAX_DOUBLE, -MAX_DOUBLE };
    if (plot.empty()) return bbox;
    
    if (plot.has_x_coordinate()) {
        for (uint64_t i = begin_idx; i < plot.points_y.size(); ++i) {
            bbox.x_begin = plot.points_x[i] < bbox.x_begin ? plot.points_x[i] : bbox.x_begin;
            bbox.x_end = plot.points_x[i] > bbox.x_end ? plot.points_x[i] : bbox.x_end;
            bbox.y_begin = plot.points_y[i] < bbox.y_begin ? plot.points_y[i] : bbox.y_begin;
            bbox.y_end = plot.points_y[i] > bbox.y_end ? plot.points_y[i] : bbox.y_end;
        }
    }
    else {
        bbox.x_begin = begin_idx;
        bbox.x_end = plot.points_y.size() - 1;
        for (uint64_t i = begin_idx; i < plot.points_y.size(); ++i) {
            bbox.y_begin = plot.points_y[i] < bbox.y_begin ? plot.points_y[i] : bbox.y_begin;
            bbox.y_end = plot.points_y[i] > bbox.y_end ? plot.points_y[i] : bbox.y_end;
        }
    }
    return bbox;
}

static Range_XY bounding_box_of_plots_bounding_boxes(std::vector<Plot_Idx>& plots)
{
    Range_XY bbox = { MAX_DOUBLE, -MAX_DOUBLE, MAX_DOUBLE, -MAX_DOUBLE };
    for (uint64_t i = 0; i < plots.size(); ++i) {
        Plot plot = gps.plots[plots[i]];
        bbox.x_begin = plot.bbox.x_begin < bbox.x_begin ? plot.bbox.x_begin : bbox.x_begin;
        bbox.x_end = plot.bbox.x_end > bbox.x_end ? plot.bbox.x_end : bbox.x_end;
        bbox.y_begin = plot.bbox.y_begin < bbox.y_begin ? plot.bbox.y_begin : bbox.y_begin;
        bbox.y_end = plot.bbox.y_end > bbox.y_end ? plot.bbox.y_end : bbox.y_end;
    }
    return bbox;
}

static Range_XY bounding_box_of_bounding_boxes(std::vector<Range_XY>& bbs)
{
    Range_XY bbox = { MAX_DOUBLE, -MAX_DOUBLE, MAX_DOUBLE, -MAX_DOUBLE };
    for (uint64_t i = 0; i < bbs.size(); ++i) {
        bbox.x_begin = bbs[i].x_begin < bbox.x_begin ? bbs[i].x_begin : bbox.x_begin;
        bbox.x_end = bbs[i].x_end > bbox.x_end ? bbs[i].x_end : bbox.x_end;
        bbox.y_begin = bbs[i].y_begin < bbox.y_begin ? bbs[i].y_begin : bbox.y_begin;
        bbox.y_end = bbs[i].y_end > bbox.y_end ? bbs[i].y_end : bbox.y_end;
    }
    return bbox;
}


void Plotter::draw(rl::Rectangle bounds, bool tile_in_focus)
{
    // Determine the plot range (The xy-range in plotspace that should be displayed)

    rl::BeginScissorMode(bounds.x, bounds.y, bounds.width, bounds.height);

    switch (vis_mode.type) {
    case Plotter_Visualization_Mode::INTERACTIVE:
        plotter_update_plot_range_interactive_mode(this, bounds, tile_in_focus);
        break;
    case Plotter_Visualization_Mode::TRACK_ALL:
        plot_range = bounding_box_of_plots_bounding_boxes(plots);
        break;
    case Plotter_Visualization_Mode::TRACK_LATEST_VALUES: {
        std::vector<Range_XY> bounding_boxes(plots.size());
        for (uint64_t i = 0; i < plots.size(); ++i) {
            Plot& plot = gps.plots[plots[i]];
            uint64_t plot_points_begin_idx = 0;
            if (vis_mode.n_points < plot.points_y.size()) {
                plot_points_begin_idx = plot.points_y.size() - vis_mode.n_points;
            }
            bounding_boxes[i] = bounding_box_of_plot(plot, plot_points_begin_idx);
        }
        plot_range = bounding_box_of_bounding_boxes(bounding_boxes);
    } break;
    case Plotter_Visualization_Mode::TRACK_LATEST_VALUES_X_RANGE:
        plot_range = bounding_box_of_plots_bounding_boxes(plots);
        if (plot_range.x_begin < plot_range.x_end) {
            plot_range.x_begin = plot_range.x_end - vis_mode.x_range;
        }
        break;
    case Plotter_Visualization_Mode::TRACK_LATEST_VALUES_XY_RANGE: {
        struct Point { double x, y; };
        std::vector<Point> latest_points;
        for (uint64_t i = 0; i < plots.size(); ++i) {
            Plot& plot = gps.plots[plots[i]];
            if (plot.empty()) continue;
            if (plot.has_x_coordinate()) latest_points.push_back({plot.points_x.back(), plot.points_y.back()});
            else                         latest_points.push_back({(double) plot.points_y.size() - 1, plot.points_y.back()});
        }

        Point midpoint = {0, 0};
        for (uint64_t i = 0; i < latest_points.size(); ++i) {
            midpoint.x += latest_points[i].x;
            midpoint.y += latest_points[i].y;
        }
        midpoint.x /= latest_points.size();
        midpoint.y /= latest_points.size();

        plot_range = { midpoint.x - vis_mode.x_range / 2.0,
                       midpoint.x + vis_mode.x_range / 2.0,
                       midpoint.y - vis_mode.y_range / 2.0,
                       midpoint.y + vis_mode.y_range / 2.0};
    } break;
    case Plotter_Visualization_Mode::TRACK_SPECIFIC_PLOT:
        assert(vis_mode.specific_plot != INVALID_IDX);
        plot_range = bounding_box_of_plot(gps.plots[vis_mode.specific_plot], 0);
        break;
    }

    // Fix the plot range if it is malformed

    if (plot_range.x_begin == plot_range.x_end) {
        plot_range.x_begin -= 1.0;
        plot_range.x_end += 1.0;
    }
    else if (plot_range.x_begin > plot_range.x_end) {
        plot_range.x_begin = -1.0;
        plot_range.x_end = 1.0;
    }
            
    if (plot_range.y_begin == plot_range.y_end) {
        plot_range.y_begin -= 1.0;
        plot_range.y_end += 1.0;
    }
    else if (plot_range.y_begin > plot_range.y_end) {
        plot_range.y_begin = -1.0;
        plot_range.y_end = 1.0;
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
    plotter_generate_ticks(ticks, bounds, plot_range);

    // Determine the size of the plot-screen (the part of the window where the plots should be drawn into)

    float left_plot_screen_offset = ticks.y_text_width_max + 2 * gps.gui.offset_normal;
    if (zoom_resize_cooldown > 0) {
        left_plot_screen_offset = plot_screen.x - bounds.x; // previous offset
        zoom_resize_cooldown--;
    }
    float right_plot_screen_offset = MIN_PLOT_SCREEN_TO_BOUNDS_OFFSET; // std::max((float) MIN_PLOT_SCREEN_TO_BOUNDS_OFFSET, legend_width);
    float top_plot_screen_offset = MIN_PLOT_SCREEN_TO_BOUNDS_OFFSET;
    float bottom_plot_screen_offset = gps.gui.fontsize_normal + gps.gui.offset_normal;

    plot_screen = { bounds.x + left_plot_screen_offset,
                    bounds.y + top_plot_screen_offset,
                    bounds.width - (left_plot_screen_offset + right_plot_screen_offset),
                    bounds.height - (top_plot_screen_offset + bottom_plot_screen_offset) };

    if (plot_screen.width < 1) plot_screen.width = 1;
    if (plot_screen.height < 1) plot_screen.height = 1;

    auto x_to_screenspace = [&](double x) -> float {
        return linear_map(x, plot_range.x_begin, plot_range.x_end, (double) plot_screen.x, (double) plot_screen.x + plot_screen.width);
    };

    auto y_to_screenspace = [&](double y) -> float {
        return linear_map(y, plot_range.y_begin, plot_range.y_end, (double) plot_screen.y + plot_screen.height, (double) plot_screen.y);
    };

    // Draw plot_screen border and ticks (tick-lines and tick-labels)

    rl::DrawLineV({ plot_screen.x, plot_screen.y - 1 },
                  { plot_screen.x, plot_screen.y + plot_screen.height + 1 },
                  gps.gui.colors.border);
    rl::DrawLineV({ plot_screen.x, plot_screen.y + plot_screen.height + 1 },
                  { plot_screen.x + plot_screen.width + 1, plot_screen.y + plot_screen.height + 1 },
                  gps.gui.colors.border);
    // rl::DrawRectangleLinesEx({plot_screen.x - bw, plot_screen.y - bw, plot_screen.width + 2*bw, plot_screen.height + 2*bw},
                             // bw, gps.gui.colors.border);
            
    int tick_idx = 0;
    for (double x = ticks.x_begin; x < plot_range.x_end + ticks.x_spacing*1e-3; x += ticks.x_spacing, ++tick_idx) {
        float x_screenspace = x_to_screenspace(x);
        rl::DrawLineV({x_screenspace, plot_screen.y}, {x_screenspace, plot_screen.y + plot_screen.height}, gps.gui.colors.tick_lines);
        int text_x = x_screenspace - ticks.x_text_width[tick_idx]/2.f;
        int text_y = plot_screen.y + plot_screen.height;
        rl::DrawTextEx(gps.gui.font_normal, ticks.x_text[tick_idx], {(float) text_x, (float) text_y }, gps.gui.fontsize_normal, gps.gui.fontspacing, gps.gui.colors.text);
    }
            
    tick_idx = 0;
    for (double y = ticks.y_begin; y < plot_range.y_end + ticks.y_spacing*1e-3; y += ticks.y_spacing, ++tick_idx) {
        float y_screenspace = y_to_screenspace(y);
        rl::DrawLineV({plot_screen.x, y_screenspace}, {plot_screen.x + plot_screen.width, y_screenspace}, gps.gui.colors.tick_lines);
        int text_x = plot_screen.x - (ticks.y_text_width[tick_idx] + gps.gui.offset_normal);
        int text_y = std::min(plot_screen.y + plot_screen.height - gps.gui.fontsize_normal,
                                std::max(plot_screen.y - 4, y_screenspace - gps.gui.fontsize_normal/2.f));
        rl::DrawTextEx(gps.gui.font_normal, ticks.y_text[tick_idx], {(float) text_x, (float) text_y}, gps.gui.fontsize_normal, gps.gui.fontspacing, gps.gui.colors.text);
    }

    // Draw in the plot-screen
            
    rl::BeginScissorMode(plot_screen.x, plot_screen.y, plot_screen.width, plot_screen.height);
    {
        // Draw x=0, y=0 coordinate-axes
                
        rl::DrawLineV({plot_screen.x, y_to_screenspace(0)}, {plot_screen.x + plot_screen.width, y_to_screenspace(0)}, gps.gui.colors.coordinate_axes);
        rl::DrawLineV({x_to_screenspace(0), plot_screen.y}, {x_to_screenspace(0), plot_screen.y + plot_screen.height}, gps.gui.colors.coordinate_axes);
            
        // Draw the plots
                
        for (size_t i = 0; i < plots.size(); ++i)
        {
            Plot_Idx plot_idx = plots[i];
            Plot& plot = gps.plots[plot_idx];
                    
            if (plot.empty()) continue;
            
            uint64_t plot_points_begin_idx = 0;
            if (vis_mode.type == Plotter_Visualization_Mode::TRACK_LATEST_VALUES && vis_mode.n_points < plot.points_y.size()) {
                plot_points_begin_idx = plot.points_y.size() - vis_mode.n_points;
            }

            if (plot.has_x_coordinate()) {
                float x_prev = x_to_screenspace(plot.points_x[plot_points_begin_idx]);
                float y_prev = y_to_screenspace(plot.points_y[plot_points_begin_idx]);
                        
                if (plot.show_points) {
                    rl::DrawCircleV({x_prev, y_prev}, plot.point_diameter/2.0, to_rl_color(plot.color));
                }
        
                for (uint64_t i = plot_points_begin_idx + 1; i < plot.points_y.size(); ++i) {
                    float x = x_to_screenspace(plot.points_x[i]);
                    float y = y_to_screenspace(plot.points_y[i]);
                    if (plot.show_lines) {
                        // Plotting with width 1.0 through DrawLineV looks much worse than through DrawLineEx
                        if (plot.line_width == 1.0)
                            rl::DrawLineV({x_prev, y_prev}, {x, y}, to_rl_color(plot.color));
                        else
                            rl::DrawLineEx({x_prev, y_prev}, {x, y}, plot.line_width, to_rl_color(plot.color));
                    }
                    if (plot.show_points) {
                        rl::DrawCircleV({x, y}, plot.point_diameter/2.0, to_rl_color(plot.color));
                    }
                    x_prev = x;
                    y_prev = y;
                }
            }
            else {
                float y_prev = y_to_screenspace(plot.points_y[plot_points_begin_idx]);

                if (plot.show_points) {
                    rl::DrawCircleV({x_to_screenspace(plot_points_begin_idx), y_prev}, plot.point_diameter/2.0, to_rl_color(plot.color));
                }
        
                for (uint64_t i = plot_points_begin_idx + 1; i < plot.points_y.size(); ++i) {
                    float y = y_to_screenspace(plot.points_y[i]);
                    if (plot.show_lines) {
                        if (plot.line_width == 1.0)
                            rl::DrawLineV({x_to_screenspace(i - 1), y_prev}, {x_to_screenspace(i), y}, to_rl_color(plot.color));
                        else
                            rl::DrawLineEx({x_to_screenspace(i - 1), y_prev}, {x_to_screenspace(i), y}, plot.line_width, to_rl_color(plot.color));
                    }
                    if (plot.show_points) {
                        rl::DrawCircleV({x_to_screenspace(i), y}, plot.point_diameter/2.0, to_rl_color(plot.color));
                    }
                    y_prev = y;
                }
            }
        }
    }
    rl::EndScissorMode();

    // Draw ticks marks (they have to be drawn after the plots, because they need to be on top)

    tick_idx = 0;
    for (double x = ticks.x_begin; x < plot_range.x_end + ticks.x_spacing*1e-3; x += ticks.x_spacing, ++tick_idx) {
        float x_screenspace = x_to_screenspace(x);
        rl::DrawLineEx({x_screenspace, plot_screen.y + plot_screen.height - gps.gui.tick_mark_len}, {x_screenspace, plot_screen.y + plot_screen.height}, 1, gps.gui.colors.border);
    }
            
    tick_idx = 0;
    for (double y = ticks.y_begin; y < plot_range.y_end + ticks.y_spacing*1e-3; y += ticks.y_spacing, ++tick_idx) {
        float y_screenspace = y_to_screenspace(y);
        rl::DrawLineEx({plot_screen.x, y_screenspace}, {plot_screen.x + gps.gui.tick_mark_len, y_screenspace}, 1, gps.gui.colors.border);
    }

    rl::EndScissorMode();
}

void Plotter::get_legend_items(std::vector<Legend_Item>& legend_items)
{
    if (gps.gui.api_abstraction_level.level_reached(API_Abstraction_Level::PLOTTERS)) {
        legend_items.push_back(Legend_Item{ .type=Legend_Item::SUB_HEADER, .text=label, .color=gps.gui.colors.text });
    }
    for (uint64_t i = 0; i < plots.size(); ++i) {
        Plot& plot = gps.plots[plots[i]];
        legend_items.push_back(Legend_Item{ .type=Legend_Item::NORMAL, .text=plot.label, .color=to_rl_color(plot.color) });
    }
}

static rl::Vector4 operator*(rl::Matrix M, rl::Vector4 v)
{
    return rl::Vector4 {
        M.m0 * v.x + M.m4 * v.y + M.m8 * v.z + M.m12 * v.w,
        M.m1 * v.x + M.m5 * v.y + M.m9 * v.z + M.m13 * v.w,
        M.m2 * v.x + M.m6 * v.y + M.m10 * v.z + M.m14 * v.w,
        M.m3 * v.x + M.m7 * v.y + M.m11 * v.z + M.m15 * v.w
    };
}

static rl::Vector4 point_to_homo(rl::Vector3 v) {
    return rl::Vector4{ v.x, v.y, v.z, 1.0f };
}

// static rl::Vector4 vector_to_homo(rl::Vector3 v) {
//     return rl::Vector4{ v.x, v.y, v.z, 0.0f };
// }

static rl::Vector3 from_homo(rl::Vector4 v) {
    return rl::Vector3{ v.x, v.y, v.z };
}

// static rl::Matrix inverse_of_view_matrix(rl::Matrix M)
// {
//     // V = (R, -R*C)
//     //     (0,  1  )
//     // 
//     // V^-1 = (R^T, C)
//     //        (0,   1)
    
//     rl::Vector3 translation = {
//         -(M.m0 * M.m12 + M.m1 * M.m13 + M.m2 * M.m14),
//         -(M.m4 * M.m12 + M.m5 * M.m13 + M.m6 * M.m14),
//         -(M.m8 * M.m12 + M.m9 * M.m13 + M.m10 * M.m14)
//     };
//     return rl::Matrix {
//         M.m0, M.m1, M.m2, translation.x,
//         M.m4, M.m5, M.m6, translation.y,
//         M.m8, M.m9, M.m10, translation.z,
//         0.0f, 0.0f, 0.0f, 1.0f,
//     };
// }

void Plotter3d::draw(rl::Rectangle bounds, bool tile_in_focus)
{
    // Update the camera

    // Determine the new camera center
    switch (vis_mode.type) {
    case Plotter3d_Visualization_Mode::FREE_CAMERA:
        // Do nothing
        break;
    case Plotter3d_Visualization_Mode::TRACK_POINT:
        rl_camera.target = vis_mode.track_point;
        break;
    case Plotter3d_Visualization_Mode::TRACK_PLOT3D_RELATIVE_POINT:
    {
        Plot3d& plot3d = gps.plots3d[vis_mode.plot3d_idx];
        rl_camera.target = from_homo(plot3d.transform * point_to_homo(vis_mode.relative_point));
    } break;
    case Plotter3d_Visualization_Mode::TRACK_PLOT3D_MIDPOINT:
    {
        Plot3d& plot3d = gps.plots3d[vis_mode.plot3d_idx];
        rl_camera.target = from_homo(plot3d.transform * point_to_homo(plot3d.midpoint));
    } break;
    case Plotter3d_Visualization_Mode::TRACK_LATEST_PLOT3D_VERTEX:
    {
        Plot3d& plot3d = gps.plots3d[vis_mode.plot3d_idx];
        if (plot3d.vertices.empty()) break;
        rl::Vector3 latest_point = { plot3d.vertices.back().x, plot3d.vertices.back().y, plot3d.vertices.back().z };
        rl_camera.target = from_homo(plot3d.transform * point_to_homo(latest_point));
    } break;
    default:
        assert(false);
        break;
    }

    rl::Vector2 mouse_delta = rl::GetMouseDelta();
    
    // Paning
    if (vis_mode.type == Plotter3d_Visualization_Mode::FREE_CAMERA)
    {
        float pan_x = 0;
        float pan_y = 0;
        if (rl::IsMouseButtonDown(rl::MOUSE_BUTTON_RIGHT) && tile_in_focus) {
            pan_x = mouse_delta.x * PLOTTER3D_PAN_FACTOR;
            pan_y = mouse_delta.y * PLOTTER3D_PAN_FACTOR;
        }

        // TODO: This distance correction is not good, do the actual thing!
        float distance_correction = 1.0f;
        if (rl_camera.projection == rl::CAMERA_PERSPECTIVE) {
            distance_correction = std::cos(rl_camera.fovy / 2.0f) * rl::Vector3Distance(rl_camera.position, rl_camera.target);
        }
            
        rl::Vector3 pan_direction_right = rl::Vector3Normalize(rl::Vector3CrossProduct(rl_camera.position - rl_camera.target, rl_camera.up)) * distance_correction;
        rl::Vector3 pan_direction_up = rl::Vector3Normalize(rl::Vector3CrossProduct(pan_direction_right, rl_camera.target - rl_camera.position)) * distance_correction;

        rl::Vector3 pan_delta = pan_direction_right * pan_x + pan_direction_up * pan_y;

        rl_camera.position = rl_camera.position + pan_delta;
        rl_camera.target   = rl_camera.target + pan_delta;
    }

    // Rotation
    float delta_pitch = 0.0f;
    float delta_yaw = 0.0f;
    if (rl::IsMouseButtonDown(rl::MOUSE_BUTTON_LEFT) && tile_in_focus) {
        delta_pitch = -mouse_delta.y * PLOTTER3D_ROTATE_FACTOR;
        delta_yaw = -mouse_delta.x * PLOTTER3D_ROTATE_FACTOR;
    }

    rl::Vector3 camera_rel_pos = rl_camera.position - rl_camera.target;
    float current_pitch = rl::Vector3Angle(rl_camera.up, camera_rel_pos);
    if (current_pitch - delta_pitch >= M_PI - EPSILON || current_pitch - delta_pitch <= 0.0f + EPSILON) {
        delta_pitch = 0.0f;
    }
    
    rl::Vector3 axis_yaw = rl_camera.up;
    rl::Vector3 axis_pitch = rl::Vector3CrossProduct(camera_rel_pos, rl_camera.up);

    rl::Vector3 new_rel_pos = camera_rel_pos;
    new_rel_pos = rl::Vector3RotateByAxisAngle(new_rel_pos, axis_pitch, delta_pitch);
    new_rel_pos = rl::Vector3RotateByAxisAngle(new_rel_pos, axis_yaw, delta_yaw);

    // Zooming
    float zoom_factor = 1.0f;
    if (rl::CheckCollisionPointRec(rl::GetMousePosition(), bounds)) {
        zoom_factor = std::pow(1.2f, -rl::GetMouseWheelMove() * SCROLL_ZOOM_FACTOR);
    }
    new_rel_pos *= zoom_factor;

    // Updating the camera position
    rl_camera.position = rl_camera.target + new_rel_pos;

    // Determine the screen size and prepare the render texture

    rl::Rectangle plot_screen = { .x = bounds.x + gps.gui.offset_normal,
                                  .y = bounds.y + gps.gui.offset_normal,
                                  .width = std::max(bounds.width - gps.gui.offset_normal * 2.0f, 1.0f),
                                  .height = std::max(bounds.height - gps.gui.offset_normal * 2.0f, 1.0f) };

    // Resizing the render texture if necessary
    if (prev_render_target_width != plot_screen.width || prev_render_target_height != plot_screen.height)
    {
        rl::UnloadRenderTexture(render_target);
        render_target = rl::LoadRenderTexture(plot_screen.width, plot_screen.height);
        prev_render_target_width = plot_screen.width;
        prev_render_target_height = plot_screen.height;
    }

    // Draw the 3d plots
    
    rl::BeginTextureMode(render_target);
    rl::BeginMode3D(rl_camera);

    rl::ClearBackground(gps.gui.colors.backgound);

    for (size_t i = 0; i < plots3d.size(); ++i) {
        Plot3d& plot3d = gps.plots3d[plots3d[i]];
        switch(plot3d.type) {
        case Plot3d::SPHERES:
            for (size_t i = 0; i < plot3d.vertices.size(); ++i) {
                rl::DrawSphere(rl::Vector3Transform({ plot3d.vertices[i].x, plot3d.vertices[i].y, plot3d.vertices[i].z }, plot3d.transform),
                               plot3d.sphere_diameter,
                               to_rl_color(plot3d.vertices[i].color));
            }
            break;
        case Plot3d::LINES: {
            for (size_t i = 1; i < plot3d.vertices.size(); i += 2) {
                rl::DrawLine3D(rl::Vector3Transform({ plot3d.vertices[i - 1].x, plot3d.vertices[i - 1].y, plot3d.vertices[i - 1].z }, plot3d.transform),
                               rl::Vector3Transform({ plot3d.vertices[i].x, plot3d.vertices[i].y, plot3d.vertices[i].z }, plot3d.transform),
                               to_rl_color(plot3d.vertices[i].color));
            }
        } break;
        case Plot3d::CONTINUOUS_LINE: {
            if (plot3d.vertices.empty()) break;
            rl::Vector3 begin = rl::Vector3Transform({ plot3d.vertices[0].x, plot3d.vertices[0].y, plot3d.vertices[0].z }, plot3d.transform);
            for (size_t i = 1; i < plot3d.vertices.size(); ++i) {
                rl::Vector3 end = rl::Vector3Transform({ plot3d.vertices[i].x, plot3d.vertices[i].y, plot3d.vertices[i].z }, plot3d.transform);
                rl::DrawLine3D(begin, end, to_rl_color(plot3d.vertices[i].color));
                begin = end;
            }
        } break;
        case Plot3d::TRIANGLES:
            for (size_t i = 2; i < plot3d.vertices.size(); i += 3) {
                // NOTE (YUZENI): We are drawing the triangel twice (once for each winding order),
                // because we do want to show the backface here. (this is not optimal obviously).
                rl::DrawTriangle3D(rl::Vector3Transform({ plot3d.vertices[i - 2].x, plot3d.vertices[i - 2].y, plot3d.vertices[i - 2].z }, plot3d.transform),
                                   rl::Vector3Transform({ plot3d.vertices[i - 1].x, plot3d.vertices[i - 1].y, plot3d.vertices[i - 1].z }, plot3d.transform),
                                   rl::Vector3Transform({ plot3d.vertices[i].x, plot3d.vertices[i].y, plot3d.vertices[i].z }, plot3d.transform),
                                   to_rl_color(plot3d.vertices[i].color));
                rl::DrawTriangle3D(rl::Vector3Transform({ plot3d.vertices[i].x, plot3d.vertices[i].y, plot3d.vertices[i].z }, plot3d.transform),
                                   rl::Vector3Transform({ plot3d.vertices[i - 1].x, plot3d.vertices[i - 1].y, plot3d.vertices[i - 1].z }, plot3d.transform),
                                   rl::Vector3Transform({ plot3d.vertices[i - 2].x, plot3d.vertices[i - 2].y, plot3d.vertices[i - 2].z }, plot3d.transform),
                                   to_rl_color(plot3d.vertices[i].color));
            }
            break;
        default:
            assert(false);
            break;
        }
    }
    
    // rl::DrawMesh()
    
    rl::EndMode3D();
    rl::EndTextureMode();

    rl::DrawTexture(render_target.texture, plot_screen.x, plot_screen.y, rl::Color{255, 255, 255, 255});

}

void Plotter3d::get_legend_items(std::vector<Legend_Item>& legend_items)
{
    if (gps.gui.api_abstraction_level.level_reached(API_Abstraction_Level::PLOTTERS)) {
        legend_items.push_back(Legend_Item{ .type=Legend_Item::SUB_HEADER, .text=label, .color=gps.gui.colors.text });
    }
    for (uint64_t i = 0; i < plots3d.size(); ++i) {
        Plot3d& plot3d = gps.plots3d[plots3d[i]];
        legend_items.push_back(Legend_Item{ .type=Legend_Item::NORMAL, .text=plot3d.label, .color=gps.gui.colors.text });
    }
}

void Panel_Tile::draw(rl::Rectangle bounds, Tile_Idx& focused_tile)
{
    if ((rl::IsMouseButtonPressed(rl::MOUSE_BUTTON_LEFT) || rl::IsMouseButtonPressed(rl::MOUSE_BUTTON_RIGHT))
        && rl::CheckCollisionPointRec(rl::GetMousePosition(), bounds))
    {
        focused_tile = tile_idx;

        if (gps.gui.interactive_on_touch_waiting)
        {
            switch(type) {
            case PLOTTER:
                gps.plotters[plotter_idx].vis_mode = { .type=Plotter_Visualization_Mode::INTERACTIVE };
                gps.gui.interactive_on_touch_waiting = false;
                break;
            case PLOTTER3D:
                gps.plotters3d[plotter3d_idx].vis_mode = { .type=Plotter3d_Visualization_Mode::FREE_CAMERA };
                gps.gui.interactive_on_touch_waiting = false;
                break;
            }
        }
    }
    
    switch(type) {
    case EMPTY:
    {
        int label_len = snprintf(nullptr, 0, "[%d] Tile", tile_idx) + 1;
        char* label = new char[label_len];
        snprintf(label, label_len, "[%d] Tile", tile_idx);

        rl::Vector2 label_size = rl::MeasureTextEx(gps.gui.font_large, label, gps.gui.fontsize_large, gps.gui.fontspacing);
        int label_x = bounds.x + (bounds.width - label_size.x) * 0.5f;
        int label_y = bounds.y + (bounds.height - label_size.y) * 0.5f;

        rl::DrawTextEx(gps.gui.font_large, label, {(float) label_x, (float) label_y}, gps.gui.fontsize_large, gps.gui.fontspacing, gps.gui.colors.text_low_alpha);

        delete[] label;
    }
    break;
    case PLOTTER:
        gps.plotters[plotter_idx].draw(bounds, focused_tile == tile_idx);
        break;
    case PLOTTER3D:
        gps.plotters3d[plotter3d_idx].draw(bounds, focused_tile == tile_idx);
        break;
    case LEFTRIGHT:
    {
        assert(first && second);
        rl::Rectangle bounds_left = bounds;
        bounds_left.width *= split_weight;
        first->draw(bounds_left, focused_tile);
        
        rl::Rectangle bounds_right = bounds;
        bounds_right.width *= (1.0f - split_weight);
        bounds_right.x += bounds.width - bounds_right.width;
        second->draw(bounds_right, focused_tile);

        // draw a seperating line
        float line_x = bounds_right.x;
        float line_y_begin = bounds.y;
        float line_y_end = bounds.y + bounds.height;
        rl::DrawLineV({line_x, line_y_begin}, {line_x, line_y_end}, gps.gui.colors.border);
    }
    break;
    case TOPBOTTOM:
    {
        assert(first && second);
        rl::Rectangle bounds_top = bounds;
        bounds_top.height *= split_weight;
        first->draw(bounds_top, focused_tile);
        
        rl::Rectangle bounds_bottom = bounds;
        bounds_bottom.height *= (1.0f - split_weight);
        bounds_bottom.y += bounds.height - bounds_bottom.height;
        second->draw(bounds_bottom, focused_tile);

        // draw a seperating line
        float line_y = bounds_bottom.y;
        float line_x_begin = bounds.x;
        float line_x_end = bounds.x + bounds.width;
        rl::DrawLineV({line_x_begin, line_y}, {line_x_end, line_y}, gps.gui.colors.border);
    }
    break;
    default:
        assert(false);
    }
}

void Panel_Tile::get_legend_items(std::vector<Legend_Item>& legend_items)
{
    switch(type) {
    case PLOTTER:
        gps.plotters[plotter_idx].get_legend_items(legend_items);
        break;
    case PLOTTER3D:
        gps.plotters3d[plotter3d_idx].get_legend_items(legend_items);
        break;
    case LEFTRIGHT:
    case TOPBOTTOM:
        assert(first && second);
        first->get_legend_items(legend_items);
        second->get_legend_items(legend_items);
        break;
    default:
        break;
    }
}

void Panel::draw(rl::Rectangle bounds)
{
    // Draw panel legend
    
    std::vector<Legend_Item> legend_items;
    if (gps.gui.api_abstraction_level.level_reached(API_Abstraction_Level::PANELS)) {
        legend_items.push_back(Legend_Item{ .type=Legend_Item::MAIN_HEADER, .text=label, .color=gps.gui.colors.text });
    }
    root_tile->get_legend_items(legend_items);

    float normal_item_x_offset      = gps.gui.api_abstraction_level.type * gps.gui.offset_normal;
    float sub_header_item_x_offset  = std::max(((int) gps.gui.api_abstraction_level.type - 1) * gps.gui.offset_normal, 0.0f);

    // Measure legend text width
    float legend_text_width = 0;
    for (size_t i = 0; i < legend_items.size(); ++i)
    {
        Legend_Item& legend_item = legend_items[i];
        float text_width = 0;
        switch(legend_item.type) {
        case Legend_Item::NORMAL:
            text_width = rl::MeasureTextEx(gps.gui.font_normal, legend_item.text, gps.gui.fontsize_normal, gps.gui.fontspacing).x + normal_item_x_offset;
            break;
        case Legend_Item::SUB_HEADER:
            text_width = rl::MeasureTextEx(gps.gui.font_large, legend_item.text, gps.gui.fontsize_large, gps.gui.fontspacing).x + sub_header_item_x_offset;
            break;
        case Legend_Item::MAIN_HEADER:
            text_width = rl::MeasureTextEx(gps.gui.font_large, legend_item.text, gps.gui.fontsize_large, gps.gui.fontspacing).x;
            break;
        }
        legend_text_width = text_width > legend_text_width ? text_width : legend_text_width;
    }

    rl::Rectangle legend_bounds = {.x = bounds.x + bounds.width - (legend_text_width + 2 * gps.gui.offset_normal),
                                   .y = bounds.y,
                                   .width = legend_text_width + 2 * gps.gui.offset_normal,
                                   .height = bounds.height};

    int legend_y = legend_bounds.y + gps.gui.offset_normal;
    
    for (size_t i = 0; i < legend_items.size(); ++i)
    {
        Legend_Item& legend_item = legend_items[i];
        switch(legend_item.type) {
        case Legend_Item::NORMAL:
        {
            int text_x = legend_bounds.x + normal_item_x_offset + gps.gui.offset_normal;
            rl::DrawTextEx(gps.gui.font_normal, legend_item.text, {(float) text_x, (float) legend_y}, gps.gui.fontsize_normal, gps.gui.fontspacing, legend_item.color);
            legend_y += gps.gui.fontsize_normal;
        } break;
        case Legend_Item::SUB_HEADER:
        {
            int text_x = legend_bounds.x + sub_header_item_x_offset + gps.gui.offset_normal;
            rl::DrawTextEx(gps.gui.font_large, legend_item.text, {(float) text_x, (float) legend_y}, gps.gui.fontsize_large, gps.gui.fontspacing, legend_item.color);
            legend_y += gps.gui.fontsize_large;
        } break;
        case Legend_Item::MAIN_HEADER:
        {
            int text_x = legend_bounds.x + gps.gui.offset_normal;
            rl::DrawTextEx(gps.gui.font_large, legend_item.text, {(float) text_x, (float) legend_y}, gps.gui.fontsize_large, gps.gui.fontspacing, legend_item.color);
            legend_y += gps.gui.fontsize_large;
            rl::DrawLineV({(float) text_x, (float) legend_y}, {text_x + legend_text_width, (float) legend_y}, legend_item.color);
            legend_y += gps.gui.offset_small;
        } break;
        }
    }

    // draw a seperating line
    float line_x = legend_bounds.x;
    float line_y_begin = legend_bounds.y;
    float line_y_end = legend_bounds.y + legend_bounds.height;
    rl::DrawLineV({line_x, line_y_begin}, {line_x, line_y_end}, gps.gui.colors.border);

    // Draw the root tile (all tiles recursively)

    rl::Rectangle root_tile_bounds = bounds;
    root_tile_bounds.width -= legend_bounds.width;

    if ((rl::IsMouseButtonPressed(rl::MOUSE_BUTTON_LEFT) || rl::IsMouseButtonPressed(rl::MOUSE_BUTTON_RIGHT))
        && !rl::CheckCollisionPointRec(rl::GetMousePosition(), root_tile_bounds))
    {
        focused_tile = INVALID_IDX;
    }
    
    root_tile->draw(root_tile_bounds, focused_tile);
}

////////////////////////////////////////////
// Gui
////////////////////////////////////////////

void gui_loop()
{
    initialize_plotlib_state();
    
    while (true)
    {
        // Update the gui
        
        synchronize_plotlib_state();

        // (Re)open window if not open but should be open
        if (!gps.gui.window_is_init && gps.gui.window_visible) {
            rl::SetConfigFlags(rl::FLAG_WINDOW_RESIZABLE);
            rl::SetTraceLogLevel(rl::LOG_ERROR);
            rl::InitWindow(gps.gui.window_width, gps.gui.window_height, "Plotlib");
            rl::SetTargetFPS(gps.gui.target_fps);
            gps.gui.font_normal = rl::LoadFontFromMemory(".ttf", gui_font_binary_ttf, gui_font_binary_ttf_len, gps.gui.fontsize_normal, nullptr, 0);
            gps.gui.font_large = rl::LoadFontFromMemory(".ttf", gui_font_binary_ttf, gui_font_binary_ttf_len, gps.gui.fontsize_large, nullptr, 0);
            gps.gui.window_is_init = true;
        }

        // Close window if open but shouldn't be open
        if (gps.gui.window_is_init && (rl::WindowShouldClose() || gps.gui.terminate)) {
            rl::CloseWindow();
            gps.gui.window_is_init = false;
            gps.gui.window_visible = false;
            gps.gui.terminate = false;
            
            gps_shared_mutex.lock();
            gps.gui.shared.window_visible = false;
            gps_shared_mutex.unlock();
            gps.gui.window_visible = false;
        }

        if (!gps.gui.window_visible) {
            // Don't busy-loop the gui thread
            std::this_thread::sleep_for(std::chrono::microseconds(1000));
            continue;
        }

        gps.gui.window_width = rl::GetScreenWidth();
        gps.gui.window_height = rl::GetScreenHeight();

        // Draw the gui

        rl::BeginDrawing();
        {
            rl::ClearBackground(gps.gui.colors.backgound);

            rl::Rectangle panel_bounds = { 0, 0, (float) gps.gui.window_width, (float) gps.gui.window_height };
        
            assert(valid_panel_idx(gps.gui.visible_panel));
            gps.panels[gps.gui.visible_panel].draw(panel_bounds);
        }
        rl::EndDrawing();
    }
}

static void start_gui_thread()
{
    static std::thread gui_thread(gui_loop);
    gui_thread.detach();
}

static void start_gui_thread_if_not_started()
{
    static bool started_gui_thread = false;
    if (!started_gui_thread) {
        start_gui_thread();
        started_gui_thread = true;
    }
}

////////////////////////////////////////////
// Plotlib API
////////////////////////////////////////////

PLOTAPI void plotlib_show()
{
    Scoped_GPS_Mutext_Lock lock;
    gps.gui.shared.window_visible = true;
    start_gui_thread_if_not_started();
}

PLOTAPI void plotlib_hide()
{
    Scoped_GPS_Mutext_Lock lock;
    gps.gui.shared.terminate = true;
}

PLOTAPI void plotlib_dark_theme()
{
    Scoped_GPS_Mutext_Lock lock;
    gps.gui.shared.colors = dark_theme_colors;
}

PLOTAPI void plotlib_light_theme()
{
    Scoped_GPS_Mutext_Lock lock;
    gps.gui.shared.colors = light_theme_colors;
}

PLOTAPI void plotlib_clear_all_plots()
{
    Scoped_GPS_Mutext_Lock lock;
    for (Plot_Idx plot_idx = 0; plot_idx <= PLOTLIB_MAX_PLOT_IDX; ++plot_idx) {
        gps.plots[plot_idx].shared.clear_plot();
    }
}

PLOTAPI void plotlib_interactive()
{
    Scoped_GPS_Mutext_Lock lock;
    gps.gui.shared.interactive_on_touch = true;
}

PLOTAPI bool plot_show(Plot_Idx plot_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot_idx(plot_idx)) return false;
    gps.panels[0].shared.new_tiles.push_back(Panel_Tile{.type=Panel_Tile::PLOTTER, .automatic_tile_insertion=true, .plotter_idx=0 });
    gps.panels[0].shared.no_changes = false;
    gps.gui.shared.visible_panel = 0;
    
    gps.plotters[0].shared.new_plots.push_back(plot_idx);
    gps.plotters[0].shared.no_changes = false;
    
    gps.gui.shared.window_visible = true;
    start_gui_thread_if_not_started();
    return true;
}    

PLOTAPI bool plot_hide(Plot_Idx plot_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot_idx(plot_idx)) return false;
    gps.plotters[0].shared.remove_plots.push_back(plot_idx);
    gps.plotters[0].shared.no_changes = false;
    return true;
}

PLOTAPI bool plot_clear(uint32_t plot_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot_idx(plot_idx)) return false;
    gps.plots[plot_idx].shared.clear_plot();
    return true;
}

PLOTAPI bool plot_set_color(uint32_t plot_idx, uint8_t r, uint8_t g, uint8_t b, uint8_t a) 
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot_idx(plot_idx)) return false;
    gps.plots[plot_idx].shared.custom_color = Color{r, g, b, a};
    gps.plots[plot_idx].shared.has_custom_color = true;
    gps.plots[plot_idx].shared.no_changes = false;
    return true;
}

PLOTAPI bool plot_set_name(uint32_t plot_idx, const char* name)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot_idx(plot_idx)) return false;
    Plot& plot = gps.plots[plot_idx];
    plot.shared.new_name = new char[strlen(name) + 1];
    strcpy(plot.shared.new_name, name);
    plot.shared.no_changes = false;
    return true;
}

PLOTAPI bool plot_as_lines(uint32_t plot_idx, double line_width)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot_idx(plot_idx)) return false;
    gps.plots[plot_idx].shared.show_lines = true;
    gps.plots[plot_idx].shared.show_points = false;
    gps.plots[plot_idx].shared.line_width = line_width;
    gps.plots[plot_idx].shared.no_changes = false;
    return true;
}

PLOTAPI bool plot_as_scatter(uint32_t plot_idx, double diameter)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot_idx(plot_idx)) return false;
    gps.plots[plot_idx].shared.show_points = true;
    gps.plots[plot_idx].shared.show_lines = false;
    gps.plots[plot_idx].shared.point_diameter = diameter;
    gps.plots[plot_idx].shared.no_changes = false;
    return true;
}

PLOTAPI bool plot_as_scatterlines(uint32_t plot_idx, double line_width, double diameter)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot_idx(plot_idx)) return false;
    gps.plots[plot_idx].shared.show_points = true;
    gps.plots[plot_idx].shared.show_lines = true;
    gps.plots[plot_idx].shared.line_width = line_width;
    gps.plots[plot_idx].shared.point_diameter = diameter;
    gps.plots[plot_idx].shared.no_changes = false;
    return true;
}

PLOTAPI bool plot_fill_numbers(uint32_t plot_idx, void* numbers, const Number_Types num_type, uint64_t count)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot_idx(plot_idx)) return false;

    Plot& plot = gps.plots[plot_idx];
    plot.shared.clear_plot();
    
    plot.shared.new_points_y.resize(count);

    // NOTE (YUZENI): The switch-case will be performed prior to the loop when compiling with optimizations.
    for (uint64_t i = 0; i < count; ++i) {
        switch (num_type) {
        case PLOT_FLOAT32: plot.shared.new_points_y[i] = ((float*) numbers)[i]; break;
        case PLOT_FLOAT64: plot.shared.new_points_y[i] = ((double*) numbers)[i]; break;
        case PLOT_INT32:   plot.shared.new_points_y[i] = ((int32_t*) numbers)[i]; break;
        case PLOT_INT64:   plot.shared.new_points_y[i] = ((int64_t*) numbers)[i]; break;
        }
    }
    
    plot.shared.contains_numbers = true;
    plot.shared.no_changes = false;
    return true;
}

PLOTAPI bool plot_fill_points_x_y(uint32_t plot_idx, void* points_x, void* points_y, const Number_Types num_type, uint64_t count)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot_idx(plot_idx)) return false;
    Plot& plot = gps.plots[plot_idx];
    plot.shared.clear_plot();

    plot.shared.new_points_x.resize(count);
    plot.shared.new_points_y.resize(count);
    
    for (uint64_t i = 0; i < count; ++i) {
        switch (num_type) {
        case PLOT_FLOAT32:
            plot.shared.new_points_x[i] = ((float*) points_x)[i];
            plot.shared.new_points_y[i] = ((float*) points_y)[i];
            break;
        case PLOT_FLOAT64:
            plot.shared.new_points_x[i] = ((double*) points_x)[i];
            plot.shared.new_points_y[i] = ((double*) points_y)[i];
            break;
        case PLOT_INT32:
            plot.shared.new_points_x[i] = ((int32_t*) points_x)[i];
            plot.shared.new_points_y[i] = ((int32_t*) points_y)[i];
            break;
        case PLOT_INT64:
            plot.shared.new_points_x[i] = ((int64_t*) points_x)[i];
            plot.shared.new_points_y[i] = ((int64_t*) points_y)[i];
            break;
        }
    }
    
    plot.shared.contains_points = true;
    plot.shared.no_changes = false;
    return true;    
}

PLOTAPI bool plot_fill_points_xy(uint32_t plot_idx, void* points_xy, const Number_Types num_type, uint64_t count)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot_idx(plot_idx)) return false;
    if (count % 2 != 0) {
        printf(ERROR "The count provided is not divisible by 2 but 'plot_fill_points_xy' expects an array of Points.\n");
        return false;
    }
    Plot& plot = gps.plots[plot_idx];
    plot.shared.clear_plot();

    plot.shared.new_points_x.resize(count / 2);
    plot.shared.new_points_y.resize(count / 2);
    
    for (uint64_t i = 0; i < count / 2; ++i) {
        switch (num_type) {
        case PLOT_FLOAT32:
            plot.shared.new_points_x[i] = ((float*) points_xy)[i * 2];
            plot.shared.new_points_y[i] = ((float*) points_xy)[i * 2 + 1];
            break;
        case PLOT_FLOAT64:
            plot.shared.new_points_x[i] = ((double*) points_xy)[i * 2];
            plot.shared.new_points_y[i] = ((double*) points_xy)[i * 2 + 1];
            break;
        case PLOT_INT32:
            plot.shared.new_points_x[i] = ((int32_t*) points_xy)[i * 2];
            plot.shared.new_points_y[i] = ((int32_t*) points_xy)[i * 2 + 1];
            break;
        case PLOT_INT64:
            plot.shared.new_points_x[i] = ((int64_t*) points_xy)[i * 2];
            plot.shared.new_points_y[i] = ((int64_t*) points_xy)[i * 2 + 1];
            break;
        }
    }
    
    plot.shared.contains_points = true;
    plot.shared.no_changes = false;
    return true;    
}

PLOTAPI bool plot_append_number(uint32_t plot_idx, double number)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot_idx(plot_idx)) return false;
    Plot& plot = gps.plots[plot_idx];
    if (plot.shared.contains_points) {
        printf(ERROR "The Plot with index '%d' contains points and cannot be appended with the number '%f'.\n", plot_idx, number);
        return false;
    }
    
    plot.shared.new_points_y.push_back(number);
    plot.shared.contains_numbers = true;
    plot.shared.no_changes = false;
    return true;    
}

PLOTAPI bool plot_append_numbers(uint32_t plot_idx, void* numbers, const Number_Types num_type, uint64_t count)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot_idx(plot_idx)) return false;
    Plot& plot = gps.plots[plot_idx];
    if (plot.shared.contains_points) {
        printf(ERROR "The Plot with index '%d' contains points and cannot be appended with numbers.\n", plot_idx);
        return false;
    }

    uint64_t old_size = plot.shared.new_points_y.size();
    plot.shared.new_points_y.resize(old_size + count);
    
    for (uint64_t i = 0; i < count; ++i) {
        switch (num_type) {
        case PLOT_FLOAT32: plot.shared.new_points_y[old_size + i] = ((float*) numbers)[i]; break;
        case PLOT_FLOAT64: plot.shared.new_points_y[old_size + i] = ((double*) numbers)[i]; break;
        case PLOT_INT32:   plot.shared.new_points_y[old_size + i] = ((int32_t*) numbers)[i]; break;
        case PLOT_INT64:   plot.shared.new_points_y[old_size + i] = ((int64_t*) numbers)[i]; break;
        }
    }
    
    plot.shared.contains_numbers = true;
    plot.shared.no_changes = false;
    return true;
}

PLOTAPI bool plot_append_point(uint32_t plot_idx, double point_x, double point_y)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot_idx(plot_idx)) return false;
    Plot& plot = gps.plots[plot_idx];
    if (plot.shared.contains_numbers) {
        printf(ERROR "The Plot with index '%d' contains numbers and cannot be appended with the point '(%f, %f)'.\n", plot_idx, point_x, point_y);
        return false;
    }

    plot.shared.new_points_x.push_back(point_x);
    plot.shared.new_points_y.push_back(point_y);
    plot.shared.contains_points = true;
    plot.shared.no_changes = false;
    return true;    
}

PLOTAPI bool plot_append_points_x_y(uint32_t plot_idx, void* points_x, void* points_y, const Number_Types num_type, uint64_t count)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot_idx(plot_idx)) return false;
    Plot& plot = gps.plots[plot_idx];
    if (plot.shared.contains_numbers) {
        printf(ERROR "The Plot with index '%d' contains numbers and cannot be appended with points.\n", plot_idx);
        return false;
    }

    uint64_t old_size = plot.shared.new_points_y.size();
    plot.shared.new_points_x.resize(old_size + count);
    plot.shared.new_points_y.resize(old_size + count);
    
    for (uint64_t i = 0; i < count; ++i) {
        switch (num_type) {
        case PLOT_FLOAT32:
            plot.shared.new_points_x[old_size + i] = ((float*) points_x)[i];
            plot.shared.new_points_y[old_size + i] = ((float*) points_y)[i];
            break;
        case PLOT_FLOAT64:
            plot.shared.new_points_x[old_size + i] = ((double*) points_x)[i];
            plot.shared.new_points_y[old_size + i] = ((double*) points_y)[i];
            break;
        case PLOT_INT32:
            plot.shared.new_points_x[old_size + i] = ((int32_t*) points_x)[i];
            plot.shared.new_points_y[old_size + i] = ((int32_t*) points_y)[i];
            break;
        case PLOT_INT64:
            plot.shared.new_points_x[old_size + i] = ((int64_t*) points_x)[i];
            plot.shared.new_points_y[old_size + i] = ((int64_t*) points_y)[i];
            break;
        }
    }
    
    plot.shared.contains_points = true;
    plot.shared.no_changes = false;
    
    return true;
}

PLOTAPI bool plot_append_points_xy(uint32_t plot_idx, void* points_xy, const Number_Types num_type, uint64_t count)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot_idx(plot_idx)) return false;
    if (count % 2 != 0) {
        printf(ERROR "The count provided is not divisible by 2 but 'plot_append_points_xy' expects an array of Points.\n");
        return false;
    }
    
    Plot& plot = gps.plots[plot_idx];
    if (plot.shared.contains_numbers) {
        printf(ERROR "The Plot with index '%d' contains numbers and cannot be appended with points.\n", plot_idx);
        return false;
    }

    uint64_t old_size = plot.shared.new_points_y.size();
    plot.shared.new_points_x.resize(old_size + count / 2);
    plot.shared.new_points_y.resize(old_size + count / 2);

    for (uint64_t i = 0; i < count / 2; ++i) {
        switch (num_type) {
        case PLOT_FLOAT32:
            plot.shared.new_points_x[old_size + i] = ((float*) points_xy)[i * 2];
            plot.shared.new_points_y[old_size + i] = ((float*) points_xy)[i * 2 + 1];
            break;
        case PLOT_FLOAT64:
            plot.shared.new_points_x[old_size + i] = ((double*) points_xy)[i * 2];
            plot.shared.new_points_y[old_size + i] = ((double*) points_xy)[i * 2 + 1];
            break;
        case PLOT_INT32:
            plot.shared.new_points_x[old_size + i] = ((int32_t*) points_xy)[i * 2];
            plot.shared.new_points_y[old_size + i] = ((int32_t*) points_xy)[i * 2 + 1];
            break;
        case PLOT_INT64:
            plot.shared.new_points_x[old_size + i] = ((int64_t*) points_xy)[i * 2];
            plot.shared.new_points_y[old_size + i] = ((int64_t*) points_xy)[i * 2 + 1];
            break;
        }
    }
    
    plot.shared.contains_points = true;
    plot.shared.no_changes = false;
    return true;
}

PLOTAPI uint64_t plot_get_length(uint32_t plot_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    return gps.plots[plot_idx].shared.plot_length;
}

PLOTAPI bool plotter_show(uint32_t plotter_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plotter_idx(plotter_idx)) return false;
    
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PLOTTERS);
    gps.panels[0].shared.new_tiles.push_back(Panel_Tile{.type=Panel_Tile::PLOTTER, .automatic_tile_insertion=true, .plotter_idx=plotter_idx });
    gps.panels[0].shared.no_changes = false;
    gps.gui.shared.visible_panel = 0;
    gps.gui.shared.window_visible = true;
    
    start_gui_thread_if_not_started();
    return true;
}

PLOTAPI bool plotter_add_plot(uint32_t plotter_idx, uint32_t plot_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if ( !valid_plotter_idx(plotter_idx) || !valid_plot_idx(plot_idx)) return false;
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PLOTTERS);

    Plotter& plotter = gps.plotters[plotter_idx];
    plotter.shared.new_plots.push_back(plot_idx);
    plotter.shared.no_changes = false;
    return true;
}

PLOTAPI bool plotter_remove_plot(uint32_t plotter_idx, uint32_t plot_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plotter_idx(plotter_idx) || !valid_plot_idx(plot_idx)) return false;
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PLOTTERS);

    Plotter& plotter = gps.plotters[plotter_idx];
    plotter.shared.remove_plots.push_back(plot_idx);
    plotter.shared.no_changes = false;
    return true;
}

PLOTAPI bool plotter_clear(uint32_t plotter_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plotter_idx(plotter_idx)) return false;
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PLOTTERS);
    
    gps.plotters[plotter_idx].shared.clear_plotter();
    gps.plotters[plotter_idx].shared.no_changes = false;
    return true;    
}

PLOTAPI bool plotter_set_name(uint32_t plotter_idx, const char* name)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plotter_idx(plotter_idx)) return false;
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PLOTTERS);

    Plotter& plotter = gps.plotters[plotter_idx];
    plotter.shared.new_name = new char[strlen(name) + 1];
    strcpy(plotter.shared.new_name, name);
    plotter.shared.no_changes = false;
    return true;
}

PLOTAPI bool plotter_track_latest(uint32_t plotter_idx, uint64_t points_count)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plotter_idx(plotter_idx)) return false;
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PLOTTERS);

    gps.plotters[plotter_idx].shared.vis_mode = Plotter_Visualization_Mode { .type=Plotter_Visualization_Mode::TRACK_LATEST_VALUES, .n_points=points_count };
    gps.plotters[plotter_idx].shared.no_changes = false;
    return true;
}

PLOTAPI bool plotter_track_latest_range_x(uint32_t plotter_idx, double x_range)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plotter_idx(plotter_idx)) return false;
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PLOTTERS);

    gps.plotters[plotter_idx].shared.vis_mode = Plotter_Visualization_Mode { .type=Plotter_Visualization_Mode::TRACK_LATEST_VALUES_X_RANGE, .x_range=x_range };
    gps.plotters[plotter_idx].shared.no_changes = false;
    return true;
}

PLOTAPI bool plotter_track_latest_range_xy(uint32_t plotter_idx, double x_range, double y_range)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plotter_idx(plotter_idx)) return false;
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PLOTTERS);

    gps.plotters[plotter_idx].shared.vis_mode = Plotter_Visualization_Mode { .type=Plotter_Visualization_Mode::TRACK_LATEST_VALUES_XY_RANGE,
                                                                             .x_range=x_range, .y_range=y_range };
    gps.plotters[plotter_idx].shared.no_changes = false;
    return true;
}

PLOTAPI bool plotter_track_all(uint32_t plotter_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plotter_idx(plotter_idx)) return false;
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PLOTTERS);
    
    gps.plotters[plotter_idx].shared.vis_mode = Plotter_Visualization_Mode { .type=Plotter_Visualization_Mode::TRACK_ALL };
    gps.plotters[plotter_idx].shared.no_changes = false;
    return true;
}

PLOTAPI bool plotter_track_specific_plot(uint32_t plotter_idx, uint32_t plot_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plotter_idx(plotter_idx) || !valid_plot_idx(plot_idx)) return false;
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PLOTTERS);
    
    gps.plotters[plotter_idx].shared.vis_mode = Plotter_Visualization_Mode { .type=Plotter_Visualization_Mode::TRACK_SPECIFIC_PLOT, .specific_plot=plot_idx };
    gps.plotters[plotter_idx].shared.no_changes = false;
    return true;
}

// PLOTAPI bool plotter_interactive(uint32_t plotter_idx)
// {
//     Scoped_GPS_Mutext_Lock lock;
//     if (!valid_plotter_idx(plotter_idx)) return false;
//     gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PLOTTERS);
    
//     gps.plotters[plotter_idx].shared.vis_mode = Plotter_Visualization_Mode { .type=Plotter_Visualization_Mode::INTERACTIVE };
//     gps.plotters[plotter_idx].shared.no_changes = false;
//     return true;
// }

PLOTAPI bool plot3d_show(uint32_t plot3d_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot3d_idx(plot3d_idx)) return false;
    gps.panels[0].shared.new_tiles.push_back(Panel_Tile{.type=Panel_Tile::PLOTTER3D, .automatic_tile_insertion=true, .plotter3d_idx=0 });
    gps.panels[0].shared.no_changes = false;
    gps.gui.shared.visible_panel = 0;
    
    gps.plotters3d[0].shared.new_plots3d.push_back(plot3d_idx);
    gps.plotters3d[0].shared.no_changes = false;

    gps.gui.shared.window_visible = true;
    start_gui_thread_if_not_started();
    return true;
}

PLOTAPI bool plot3d_hide(uint32_t plot3d_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot3d_idx(plot3d_idx)) return false;
    gps.plotters3d[0].shared.remove_plots3d.push_back(plot3d_idx);
    gps.plotters3d[0].shared.no_changes = false;
    return true;
}

PLOTAPI bool plot3d_clear(uint32_t plot3d_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot3d_idx(plot3d_idx)) return false;
    gps.plotters3d[0].shared.clear_plotter3d();
    gps.plotters3d[0].shared.no_changes = false;
    return true;
}

PLOTAPI bool plot3d_set_name(uint32_t plot3d_idx, const char* name)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot3d_idx(plot3d_idx)) return false;
    Plot3d& plot3d = gps.plots3d[plot3d_idx];
    plot3d.shared.new_name = new char[strlen(name) + 1];
    strcpy(plot3d.shared.new_name, name);
    plot3d.shared.no_changes = false;
    return true;
}

PLOTAPI bool plot3d_as_spheres(uint32_t plot3d_idx, float sphere_diameter)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot3d_idx(plot3d_idx)) return false;
    gps.plots3d[plot3d_idx].shared.type = Plot3d::SPHERES;
    gps.plots3d[plot3d_idx].shared.sphere_diameter = sphere_diameter;
    gps.plots3d[plot3d_idx].shared.no_changes = false;
    return true;
}

PLOTAPI bool plot3d_as_lines(uint32_t plot3d_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot3d_idx(plot3d_idx)) return false;
    gps.plots3d[plot3d_idx].shared.type = Plot3d::LINES;
    gps.plots3d[plot3d_idx].shared.no_changes = false;
    return true;
}

PLOTAPI bool plot3d_as_triangles(uint32_t plot3d_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot3d_idx(plot3d_idx)) return false;
    gps.plots3d[plot3d_idx].shared.type = Plot3d::TRIANGLES;
    gps.plots3d[plot3d_idx].shared.no_changes = false;
    return true;
}

PLOTAPI bool plot3d_as_continuous_line(uint32_t plot3d_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot3d_idx(plot3d_idx)) return false;
    gps.plots3d[plot3d_idx].shared.type = Plot3d::CONTINUOUS_LINE;
    gps.plots3d[plot3d_idx].shared.no_changes = false;
    return true;
}

PLOTAPI bool plot3d_append_vertex(uint32_t plot3d_idx, float x, float y, float z)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot3d_idx(plot3d_idx)) return false;
    static int color_idx = 0;
    color_idx = (color_idx + 1) % plot_color_table_size;
    
    gps.plots3d[plot3d_idx].shared.new_vertices.push_back(Vertex{ .x=x, .y=y, .z=z, .color=plot_color_table[color_idx] });
    gps.plots3d[plot3d_idx].shared.no_changes = false;
    return true;
}

PLOTAPI bool plot3d_append_vertex_with_color(uint32_t plot3d_idx, float x, float y, float z, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot3d_idx(plot3d_idx)) return false;
    gps.plots3d[plot3d_idx].shared.new_vertices.push_back(Vertex{ .x=x, .y=y, .z=z, .color={r, g, b, a} });
    gps.plots3d[plot3d_idx].shared.no_changes = false;
    return true;
}

PLOTAPI bool plot3d_fill_vertices_x_y_z(uint32_t plot3d_idx, float* x, float* y, float* z, uint64_t length)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot3d_idx(plot3d_idx)) return false;

    Plot3d& plot3d = gps.plots3d[plot3d_idx];
    plot3d.shared.clear_plot3d();

    plot3d.shared.new_vertices.resize(length);
    for (uint64_t i = 0; i < length; ++i) {
        plot3d.shared.new_vertices[i] = Vertex{ .x=x[i], .y=y[i], .z=z[i], .color=plot_color_table[i % plot_color_table_size] };
    }
    
    plot3d.shared.no_changes = false;
    return true;
}

PLOTAPI bool plot3d_rotate_quaternion(uint32_t plot3d_idx, float i, float j, float k, float real)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot3d_idx(plot3d_idx)) return false;
    
    rl::Matrix& transform = gps.plots3d[plot3d_idx].shared.transform;
    rl::Vector3 translation = { transform.m12, transform.m13, transform.m14 };
    
    transform.m12 = 0;
    transform.m13 = 0;
    transform.m14 = 0;
    
    transform = rl::QuaternionToMatrix(rl::Quaternion{.x=i, .y=j, .z=k, .w=real}) * transform;
    
    transform.m12 = translation.x;
    transform.m13 = translation.y;
    transform.m14 = translation.z;
    
    gps.plots3d[plot3d_idx].shared.no_changes = false;
    return true;
}

PLOTAPI bool plot3d_set_orientation_quaternion(uint32_t plot3d_idx, float i, float j, float k, float real)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot3d_idx(plot3d_idx)) return false;

    rl::Matrix& transform = gps.plots3d[plot3d_idx].shared.transform;
    rl::Vector3 translation = { transform.m12, transform.m13, transform.m14 };

    transform = rl::QuaternionToMatrix(rl::Quaternion{.x=i, .y=j, .z=k, .w=real});

    transform.m12 = translation.x;
    transform.m13 = translation.y;
    transform.m14 = translation.z;
    
    gps.plots3d[plot3d_idx].shared.no_changes = false;
    return true;
}

PLOTAPI bool plot3d_move(uint32_t plot3d_idx, float x, float y, float z)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot3d_idx(plot3d_idx)) return false;
    gps.plots3d[plot3d_idx].shared.transform = rl::MatrixTranslate(x, y, z) * gps.plots3d[plot3d_idx].shared.transform;
    gps.plots3d[plot3d_idx].shared.no_changes = false;
    return true;
}

PLOTAPI bool plot3d_set_position(uint32_t plot3d_idx, float x, float y, float z)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plot3d_idx(plot3d_idx)) return false;
    gps.plots3d[plot3d_idx].shared.transform.m12 = x;
    gps.plots3d[plot3d_idx].shared.transform.m13 = y;
    gps.plots3d[plot3d_idx].shared.transform.m14 = z;
    gps.plots3d[plot3d_idx].shared.no_changes = false;
    return true;
}

/* PLOTAPI bool plot3d_set_indices(uint32_t plot3d_idx, uint32_t* indices, uint64_t length); */
/* PLOTAPI bool plot3d_load_from_file(uint32_t plot3d_idx, const char* file_path); */

PLOTAPI bool plotter3d_show(uint32_t plotter3d_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plotter3d_idx(plotter3d_idx)) return false;
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PLOTTERS);

    gps.panels[0].shared.new_tiles.push_back(Panel_Tile{.type=Panel_Tile::PLOTTER3D, .automatic_tile_insertion=true, .plotter3d_idx=plotter3d_idx });
    gps.panels[0].shared.no_changes = false;
    gps.gui.shared.visible_panel = 0;
    gps.gui.shared.window_visible = true;
            
    start_gui_thread_if_not_started();
    return true;
}

PLOTAPI bool plotter3d_add_plot3d(uint32_t plotter3d_idx, uint32_t plot3d_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plotter3d_idx(plotter3d_idx) || !valid_plot3d_idx(plot3d_idx)) return false;
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PLOTTERS);

    Plotter3d& plotter3d = gps.plotters3d[plotter3d_idx];
    plotter3d.shared.new_plots3d.push_back(plot3d_idx);
    plotter3d.shared.no_changes = false;
    return true;
}

PLOTAPI bool plotter3d_remove_plot3d(uint32_t plotter3d_idx, uint32_t plot3d_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plotter3d_idx(plotter3d_idx) || !valid_plot3d_idx(plot3d_idx)) return false;
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PLOTTERS);

    Plotter3d& plotter3d = gps.plotters3d[plotter3d_idx];
    plotter3d.shared.remove_plots3d.push_back(plot3d_idx);
    plotter3d.shared.no_changes = false;
    return true;
}

PLOTAPI bool plotter3d_clear(uint32_t plotter3d_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plotter3d_idx(plotter3d_idx)) return false;
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PLOTTERS);

    gps.plotters3d[plotter3d_idx].shared.clear_plotter3d();
    gps.plotters3d[plotter3d_idx].shared.no_changes = false;
    return true;
}

PLOTAPI bool plotter3d_set_name(uint32_t plotter3d_idx, const char* name)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plotter3d_idx(plotter3d_idx)) return false;
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PLOTTERS);

    Plotter3d& plotter3d = gps.plotters3d[plotter3d_idx];
    plotter3d.shared.new_name = new char[strlen(name) + 1];
    strcpy(plotter3d.shared.new_name, name);
    plotter3d.shared.no_changes = false;
    return true;
}

PLOTAPI bool plotter3d_orthogonal_projection(uint32_t plotter3d_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plotter3d_idx(plotter3d_idx)) return false;
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PLOTTERS);
    
    gps.plotters3d[plotter3d_idx].shared.ortho_projection = true;
    gps.plotters3d[plotter3d_idx].shared.no_changes = false;
    return true;    
}

PLOTAPI bool plotter3d_perspective_projection(uint32_t plotter3d_idx, float FOV)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plotter3d_idx(plotter3d_idx)) return false;
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PLOTTERS);
    
    gps.plotters3d[plotter3d_idx].shared.ortho_projection = false;
    gps.plotters3d[plotter3d_idx].shared.FOV = FOV;
    gps.plotters3d[plotter3d_idx].shared.no_changes = false;
    return true;
}

/* PLOTAPI bool plotter3d_set_camera_center(uint32_t plotter3d_idx, float x, float y, float z); */
/* PLOTAPI bool plotter3d_set_camera_orientation_quat(uint32_t plotter3d_idx, double i, double j, double k, double real); */
/* PLOTAPI bool plotter3d_first_person(uint32_t plotter3d_idx); */
/* PLOTAPI bool plotter3d_third_person(uint32_t plotter3d_idx); */

PLOTAPI bool plotter3d_camera_free(uint32_t plotter3d_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plotter3d_idx(plotter3d_idx)) return false;
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PLOTTERS);
    
    gps.plotters3d[plotter3d_idx].shared.vis_mode = Plotter3d_Visualization_Mode{ .type=Plotter3d_Visualization_Mode::FREE_CAMERA };
    gps.plotters3d[plotter3d_idx].shared.no_changes = false;
    return true;    
}

PLOTAPI bool plotter3d_track_point(uint32_t plotter3d_idx, float x, float y, float z)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plotter3d_idx(plotter3d_idx)) return false;
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PLOTTERS);
    
    gps.plotters3d[plotter3d_idx].shared.vis_mode = Plotter3d_Visualization_Mode{ .type=Plotter3d_Visualization_Mode::TRACK_POINT, .track_point=rl::Vector3{x, y, z} };
    gps.plotters3d[plotter3d_idx].shared.no_changes = false;
    return true;    
}

PLOTAPI bool plotter3d_track_point_relative_to_plot3d(uint32_t plotter3d_idx, uint32_t plot3d_idx, float x, float y, float z)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plotter3d_idx(plotter3d_idx)) return false;
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PLOTTERS);
    
    gps.plotters3d[plotter3d_idx].shared.vis_mode = Plotter3d_Visualization_Mode{ .type=Plotter3d_Visualization_Mode::TRACK_PLOT3D_RELATIVE_POINT,
                                                                                  .relative_point=rl::Vector3{x, y, z},
                                                                                  .plot3d_idx=plot3d_idx};
    gps.plotters3d[plotter3d_idx].shared.no_changes = false;
    return true;    
}

PLOTAPI bool plotter3d_track_plot3d_midpoint(uint32_t plotter3d_idx, uint32_t plot3d_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plotter3d_idx(plotter3d_idx)) return false;
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PLOTTERS);
    
    gps.plotters3d[plotter3d_idx].shared.vis_mode = Plotter3d_Visualization_Mode{ .type=Plotter3d_Visualization_Mode::TRACK_PLOT3D_MIDPOINT, .plot3d_idx=plot3d_idx };
    gps.plotters3d[plotter3d_idx].shared.no_changes = false;
    return true;    
}

PLOTAPI bool plotter3d_track_plot3d_latest_vertex(uint32_t plotter3d_idx, uint32_t plot3d_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_plotter3d_idx(plotter3d_idx)) return false;
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PLOTTERS);
    
    gps.plotters3d[plotter3d_idx].shared.vis_mode = Plotter3d_Visualization_Mode{ .type=Plotter3d_Visualization_Mode::TRACK_LATEST_PLOT3D_VERTEX, .plot3d_idx=plot3d_idx };
    gps.plotters3d[plotter3d_idx].shared.no_changes = false;
    return true;    
}

PLOTAPI bool plotlibpanel_show(uint32_t panel_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_panel_idx(panel_idx)) return false;
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PANELS);
    
    gps.gui.shared.visible_panel = panel_idx;
    return true;
}

PLOTAPI bool plotlibpanel_add_leftright_tile(uint32_t panel_idx, uint32_t tile_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_panel_idx(panel_idx)) return false;
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PANELS);
    
    gps.panels[panel_idx].shared.new_tiles.push_back(Panel_Tile{ .type=Panel_Tile::LEFTRIGHT, .tile_idx=tile_idx });
    gps.panels[panel_idx].shared.no_changes = false;
    return true;
}

PLOTAPI bool plotlibpanel_add_topbottom_tile(uint32_t panel_idx, uint32_t tile_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_panel_idx(panel_idx)) return false;
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PANELS);
    
    gps.panels[panel_idx].shared.new_tiles.push_back(Panel_Tile{ .type=Panel_Tile::TOPBOTTOM, .tile_idx=tile_idx });
    gps.panels[panel_idx].shared.no_changes = false;
    return true;
}

PLOTAPI bool plotlibpanel_add_plotter(uint32_t panel_idx, uint32_t tile_idx, uint32_t plotter_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_panel_idx(panel_idx) || !valid_plotter_idx(plotter_idx)) return false;
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PANELS);
    
    gps.panels[panel_idx].shared.new_tiles.push_back(Panel_Tile{ .type=Panel_Tile::PLOTTER, .tile_idx=tile_idx, .plotter_idx=plotter_idx});
    gps.panels[panel_idx].shared.no_changes = false;
    return true;
}

PLOTAPI bool plotlibpanel_add_plotter3d(uint32_t panel_idx, uint32_t tile_idx, uint32_t plotter3d_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_panel_idx(panel_idx) || !valid_plotter3d_idx(plotter3d_idx)) return false;
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PANELS);
    
    gps.panels[panel_idx].shared.new_tiles.push_back(Panel_Tile{.type=Panel_Tile::PLOTTER3D, .tile_idx=tile_idx, .plotter3d_idx=plotter3d_idx});
    gps.panels[panel_idx].shared.no_changes = false;
    return true;
}

PLOTAPI bool plotlibpanel_remove_plotter(uint32_t panel_idx, uint32_t plotter_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_panel_idx(panel_idx) || !valid_plotter_idx(plotter_idx)) return false;
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PANELS);

    gps.panels[panel_idx].shared.remove_tiles.push_back(Panel_Tile{.type=Panel_Tile::PLOTTER, .plotter_idx=plotter_idx});
    gps.panels[panel_idx].shared.no_changes = false;
    return true;
}

PLOTAPI bool plotlibpanel_remove_plotter3d(uint32_t panel_idx, uint32_t plotter3d_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_panel_idx(panel_idx) || !valid_plotter3d_idx(plotter3d_idx)) return false;
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PANELS);

    gps.panels[panel_idx].shared.remove_tiles.push_back(Panel_Tile{.type=Panel_Tile::PLOTTER3D, .plotter3d_idx=plotter3d_idx});
    gps.panels[panel_idx].shared.no_changes = false;
    return true;
}

PLOTAPI bool plotlibpanel_clear(uint32_t panel_idx)
{
    Scoped_GPS_Mutext_Lock lock;
    if (!valid_panel_idx(panel_idx)) return false;
    gps.gui.api_abstraction_level.set_level(API_Abstraction_Level::PANELS);
    
    gps.panels[panel_idx].shared.clear_panel();
    gps.panels[panel_idx].shared.no_changes = false;
    return true;
}
