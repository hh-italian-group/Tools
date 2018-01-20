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
using ItemCollection = ::analysis::PropertyConfigReader::ItemCollection;
using Size = ::root_ext::Size<float, 2>;
using SizeI = ::root_ext::Size<int, 2>;
using Point = ::root_ext::Point<float, 2, false>;
using PointI = ::root_ext::Point<int, 2, false>;
using MarginBox = ::root_ext::MarginBox<float>;
using Box = ::root_ext::Box<float>;
using Angle = ::analysis::Angle<2>;
using Flag2D = ::root_ext::Point<bool, 2, false>;

#define READ(name) opt.Read(#name, name);
#define READ_VAR(r, opt, name) READ(name)
#define READ_ALL(...) BOOST_PP_SEQ_FOR_EACH(READ_VAR, opt, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__))

struct Page {
    SizeI canvas_size{600, 600};
    Box main_pad{0, 0, 1, 1};
    MarginBox margins{.1f, .1f, .1f, .1f};
    Size paper_size{20.,20.};
    Color canvas_color{kWhite};
    short canvas_border_size{10};
    short canvas_border_mode{0};
    int palette{1};
    float end_error_size{2};
    Flag2D grid_xy{false, false}, tick_xy{true, true};
    Point tick_length_xy{.03f, .03f};
    PointI n_div_xy{510, 510};
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
    float ratio_pad_size{.1f}, ratio_pad_spacing{.01f};

    double zero_threshold{-std::numeric_limits<double>::infinity()};
    bool blind{false};

    std::string legend_opt;
    std::vector<std::string> text_boxes_opt;

    Page() {}

    explicit Page(const Item& opt)
    {
        READ_ALL(canvas_size, main_pad, margins, paper_size, canvas_color, canvas_border_size, canvas_border_mode,
                 palette, end_error_size, grid_xy, tick_xy, tick_length_xy, n_div_xy, draw_title, title_font,
                 title_color, title_size, axis_title_sizes, axes_title_offsets, axis_label_sizes, axis_label_offsets,
                 title, x_title, y_title, divide_by_bin_width, log_x, log_y, y_min_sf, y_max_sf, draw_ratio,
                 y_ratio_label_size, max_ratio, allowed_ratio_margin, ratio_pad_size, ratio_pad_spacing, zero_threshold,
                 blind)

        opt.Read("legend", legend_opt);
        std::string text_boxes_str;
        opt.Read("text_boxes", text_boxes_str);
        text_boxes_opt = ::analysis::SplitValueList(text_boxes_str, false, ", \t", true);
    }

    Box GetRatioPadBox() const
    {
        const float left_bottom_x = main_pad.left_bottom_x();
        const float right_top_x = main_pad.right_top_x();
        const float right_top_y = main_pad.left_bottom_y() - ratio_pad_spacing;
        const float left_bottom_y = right_top_y - ratio_pad_size;
        return Box(left_bottom_x, left_bottom_y, right_top_x, right_top_y);
    }
};

struct PositionedElement {
    Point pos{.5f, .5f};
    std::string pos_ref;

    PositionedElement() {}
    PositionedElement(const PositionedElement&) = default;
    PositionedElement& operator=(const PositionedElement&) = default;
    virtual ~PositionedElement() {}

    PositionedElement(const Item& opt)
    {
        READ_ALL(pos, pos_ref);
    }
};

struct Legend : PositionedElement {
    Size size{.25f, .25f};
    Color fill_color{kWhite};
    short fill_style{0};
    int border_size{0};
    float text_size{.026f};
    Font font{42};

    Legend() {}
    explicit Legend(const Item& opt) : PositionedElement(opt)
    {
        READ_ALL(size, fill_color, fill_style, border_size, text_size, font)
    }
};

struct Text : PositionedElement {
    std::vector<std::string> text;
    float text_size{.2f};
    float line_spacing{0.3f};
    Angle angle{0};
    Font font;
    TextAlign align{TextAlign::LeftTop};
    Color color{kBlack};

    Text() {}
    explicit Text(const Item& opt) : PositionedElement(opt)
    {
        READ_ALL(text_size, line_spacing, angle, font, align, color);
        std::string text_str;
        opt.Read("text", text_str);
        SetText(text_str);
    }

    void SetText(std::string text_str)
    {
        boost::replace_all(text_str, "\\n", "\n");
        text = ::analysis::SplitValueList(text_str, true, "\n", false);
    }
};

struct Histogram {
    short fill_style{0}, line_style{2};
    std::string legend_style{"f"};

    bool draw_unc{false};
    std::string unc_legend_title;
    std::string unc_legend_style;
    int unc_fill_style{3013};
    Color fill_color{kWhite}, line_color{kBlack}, unc_fill_color{kBlack};
    std::string draw_opt, unc_draw_opt{"e2"};
    short line_width{2};

    Histogram() {}
    explicit Histogram(const Item& opt)
    {
        READ_ALL(fill_style, line_style, legend_style, draw_unc, unc_legend_title, unc_legend_style, unc_fill_style,
                 fill_color, line_color, unc_fill_color, draw_opt, unc_draw_opt, line_width);
    }
};


#undef READ_ALL
#undef READ_VAR
#undef READ

} // namespace draw_options
} // namespace root_ext
