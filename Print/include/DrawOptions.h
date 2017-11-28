/*! Definition of classes that contain draw options.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#pragma once

#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>
#include "AnalysisTools/Core/include/RootExt.h"
#include "AnalysisTools/Core/include/PropertyConfigReader.h"
#include "AnalysisTools/Core/include/NumericPrimitives.h"
#include "PlotPrimitives.h"

namespace root_ext {
namespace draw_options {

using Item = ::analysis::PropertyConfigReader::Item;
using Size = ::root_ext::Size<double, 2>;
using Point = ::root_ext::Point<float, 2, false>;
using MarginBox = ::root_ext::MarginBox<float>;
using Box = ::root_ext::Box<double>;
using Angle = ::analysis::Angle<2>;

#define READ(r, opt, name) opt.Read(#name, name);
#define READ_ALL(...) BOOST_PP_SEQ_FOR_EACH(READ, opt, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

struct Page {
    Size canvas_size{600, 600};
    Box main_pad{0, 0, 1, 1};
    MarginBox margins{.1f, .1f, .1f, .1f};
    Size paper_size{20.,20.};
    Color canvas_color{kWhite};
    double canvas_border_size{10.};
    int canvas_border_mode{0};
    int palette{1};
    double end_error_size{2};
    int grid_x{0}, grid_y{0};
    bool tick_x{true}, tick_y{true};
    double tick_x_length{.03}, tick_y_length{.03};
    int x_n_div{510}, y_n_div{510};
    bool draw_title{false};
    Font title_font{42};
    Color title_color{kBlack};
    double title_size{.05};
    Point axis_title_sizes{.005f, .005f}, axes_title_offsets{1.f, 1.f},
          axis_label_sizes{.04f, .04f}, axis_label_offsets{.015f, .005f};

    std::string title, x_title, y_title;
    bool divide_by_bin_width{false};
    bool log_x{false}, log_y{false};
    double y_min_sf{1}, y_max_sf{1.2};

    bool draw_ratio{true};
    float y_ratio_label_size{.04f};
    double max_ratio{-1}, allowed_ratio_margin{0.2};

    double zero_threshold{-std::numeric_limits<double>::infinity()};
    bool blind{false};

    std::string legend_opt;
    std::vector<std::string> text_boxes_opt;

    explicit Page(const Item& opt)
    {
        READ_ALL(canvas_size, main_pad, margins, paper_size, canvas_color, canvas_border_size, canvas_border_mode,
                 palette, end_error_size, grid_x, grid_y, tick_x, tick_y, tick_x_length, tick_y_length, x_n_div,
                 y_n_div, draw_title, title_font, title_color, title_size, axis_title_sizes, axes_title_offsets,
                 axis_label_sizes, axis_label_offsets, title, x_title, y_title, divide_by_bin_width, log_x, log_y,
                 y_min_sf, y_max_sf, draw_ratio, y_ratio_label_size, max_ratio, allowed_ratio_margin, zero_threshold,
                 blind)

        opt.Read("legend", legend_opt);
        std::string text_boxes_str;
        opt.Read("text_boxes", text_boxes_str);
        text_boxes_opt = ::analysis::SplitValueList(text_boxes_str, false);
    }

    bool HasMainPad() const { return main_pad != Box(0, 0, 1, 1); }
};

struct Legend {
    Box pos{.6, .64, .85, .89};
    Color fill_color{kWhite};
    int fill_style{0};
    float border_size{0};
    float text_size{.026f};
    Font font{42};

    explicit Legend(const Item& opt)
    {
        READ_ALL(pos, fill_color, fill_style, border_size, text_size, font)
    }
};

struct Text {
    std::string text;
    Point pos{.5f, .5f};
    std::string pos_ref;
    Angle angle{0};
    Font font;
    TextAlign align{TextAlign::LeftCenter};

    explicit Text(const Item& opt)
    {
        READ_ALL(text, pos, pos_ref, angle, font, align);
    }
};

struct Histogram {
    int fill_style{0};
    std::string legend_style{"f"};

    bool draw_unc{false};
    std::string unc_legend_title;
    std::string unc_legend_style;
    int unc_fill_style{3013};
    Color unc_fill_color{kBlack};
    std::string unc_draw_opt{"e2"};

    explicit Histogram(const Item& opt)
    {
        READ_ALL(fill_style, legend_style, draw_unc, unc_legend_title, unc_legend_style, unc_fill_style, unc_fill_color,
                 unc_draw_opt)
    }
};

#undef READ_ALL
#undef READ

} // namespace draw_options
} // namespace root_ext
