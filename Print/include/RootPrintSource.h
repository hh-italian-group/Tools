/*! Definition of histogram source classes and page layot options that are used to print ROOT histograms.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#pragma once

#include <string>

#include "RootPrintTools.h"
#include "AnalysisTools/Core/include/RootExt.h"
#include "AnalysisTools/Core/include/PropertyConfigReader.h"

namespace root_ext {

struct PageDrawOptions {
    using Item = ::analysis::PropertyConfigReader::Item;
    using Size = ::root_ext::Size<double, 2>;
    using Point = ::root_ext::Point<float, 2, false>;
    using MarginBox = ::root_ext::MarginBox<float>;
    using Box = ::root_ext::Box<double>;

    std::string title, x_title, y_title;
    bool draw_title{false};
    bool divide_by_bin_width{false};
    bool log_x{false}, log_y{false};
    Size canvas_size{600, 600};
    MarginBox margins{.1f, .1f, .1f, .1f};
    Point axes_title_offsets{1.f, 1.f};
    double zero_threshold{-std::numeric_limits<double>::infinity()};
    float y_ratio_label_size{.04f};
    double y_min_sf{1}, y_max_sf{1.2};
    bool draw_ratio{true};
    double max_ratio{-1}, allowed_ratio_margin{0.2};
    bool draw_legend{true};
    Box legend_box{0.6, 0.64, 0.85, 0.89};
    bool blind{false};

    PageDrawOptions(const Item& opt)
    {
        opt.Read("title", title);
        opt.Read("x_title", x_title);
        opt.Read("y_title", y_title);
        opt.Read("draw_title", draw_title);
        opt.Read("div_bw", divide_by_bin_width);
        opt.Read("log_x", log_x);
        opt.Read("log_y", log_y);
        opt.Read("canvas_size", canvas_size);
        opt.Read("margins", margins);
        opt.Read("axes_title_offsets", axes_title_offsets);
        opt.Read("zero_threshold", zero_threshold);
        opt.Read("y_ratio_label_size", y_ratio_label_size);
        opt.Read("y_min_sf", y_max_sf);
        opt.Read("y_max_sf", y_max_sf);
        opt.Read("draw_ratio", draw_ratio);
        opt.Read("max_ratio", max_ratio);
        opt.Read("allowed_ratio_margin", allowed_ratio_margin);
        opt.Read("draw_legend", draw_legend);
        opt.Read("legend_box", legend_box);
        opt.Read("blind", blind);
    }
};

struct PageSideLayout {
    Box<double> main_pad;
    bool has_stat_pad;
    Box<double> stat_pad;
    bool has_legend;
    bool has_legend_pad;
    Box<double> legend_pad;
    bool has_ratio_pad;
    Box<double> ratio_pad;
};

struct PageSide {
    std::string histogram_name;
    std::string histogram_title;
    std::string axis_titleX;
    std::string axis_titleY;
    std::string draw_options;
    bool use_log_scaleX;
    bool use_log_scaleY;
    bool fit_range_x;
    bool fit_range_y;
    analysis::Range<double> xRange;
    analysis::Range<double> yRange;
    PageSideLayout layout;
};

struct PageLayout {
    bool has_title;
    Box<double> title_box;
    Font_t title_font;
    std::string global_style;
    Int_t stat_options;
    Int_t fit_options;
};

struct Page {
    using RegionCollection = std::vector<const PageSide*>;
    std::string title;
    PageLayout layout;

    virtual RegionCollection Regions() const = 0;
    virtual ~Page() {}
};

struct SingleSidedPage : public Page {
    PageSide side;

    explicit SingleSidedPage(bool has_title = true, bool has_stat_pad = true, bool has_legend = true)
    {
        layout.has_title = has_title;
        layout.title_box = Box<double>(0.1, 0.94, 0.9, 0.98);
        layout.title_font = 52;
        layout.global_style = "Plain";
        if(has_stat_pad) {
            layout.stat_options = 1111;
            layout.fit_options = 111;
        } else {
            layout.stat_options = 0;
            layout.fit_options = 0;
        }

        side.layout.has_stat_pad = has_stat_pad;
        side.layout.has_legend = has_legend;
        side.use_log_scaleX = false;
        side.use_log_scaleY = false;
        side.fit_range_x = true;
        side.fit_range_y = true;

        side.layout.main_pad = Box<double>(0.01, 0.01, 0.85, 0.91);
        side.layout.stat_pad = Box<double>(0.86, 0.01, 0.99, 0.91);
        side.layout.legend_pad = Box<double>(0.5, 0.67, 0.88, 0.88);
    }

    virtual RegionCollection Regions() const
    {
        RegionCollection regions;
        regions.push_back(&side);
        return regions;
    }
};

template<typename HistogramType, typename _ValueType=Double_t, typename OriginalHistogramType=HistogramType>
class HistogramSource {
public:
    using Histogram = HistogramType;
    using OriginalHistogram = OriginalHistogramType;
    using ValueType = _ValueType;
    using PlotOptions = typename root_ext::HistogramPlotter<Histogram, ValueType>::Options;
    using Entry = typename root_ext::HistogramPlotter<Histogram, ValueType>::Entry;

    static PlotOptions& GetDefaultPlotOptions(size_t n)
    {
        static std::vector<PlotOptions> options;
        if(!options.size())
        {
            options.push_back( PlotOptions(kGreen, 1, root_ext::Box<double>(0.01, 0.71, 0.99, 0.9), 0.1, kGreen, 2) );
            options.push_back( PlotOptions(kViolet, 1, root_ext::Box<double>(0.01, 0.51, 0.99, 0.7), 0.1, kViolet, 2) );
            options.push_back( PlotOptions(kOrange, 1, root_ext::Box<double>(0.01, 0.31, 0.99, 0.5), 0.1, kOrange, 2) );
            options.push_back( PlotOptions(kRed, 1, root_ext::Box<double>(0.01, 0.11, 0.99, 0.3), 0.1, kRed, 2) );
            options.push_back( PlotOptions(kBlue, 1, root_ext::Box<double>(0.01, 0.11, 0.99, 0.3), 0.1, kBlue, 2) );
            options.push_back( PlotOptions(kBlack, 1, root_ext::Box<double>(0.01, 0.11, 0.99, 0.3), 0.1, kBlack, 2) );
        }
        return n < options.size() ? options[n] : options[options.size() - 1];
    }

public:
    void Add(const std::string& display_name, std::shared_ptr<TFile> source_file, const PlotOptions* plot_options = 0)
    {
        if(!plot_options)
            plot_options = &GetDefaultPlotOptions(display_names.size());

        display_names.push_back(display_name);
        source_files.push_back(source_file);
        plot_options_vector.push_back(*plot_options);
    }

    size_t Size() const { return display_names.size(); }

    Entry Get(unsigned id, const std::string& name) const
    {
        if(!source_files[id])
            return Entry(0, plot_options_vector[id]);
        std::string realName = GenerateName(id, name);
        OriginalHistogram* original_histogram = root_ext::ReadObject<OriginalHistogram>(*source_files[id], realName);
        Histogram* histogram = Convert(original_histogram);
        if(!histogram)
            throw std::runtime_error("source histogram not found.");
        Prepare(histogram, display_names[id], plot_options_vector[id]);
        return Entry(histogram, plot_options_vector[id]);
    }

    virtual ~HistogramSource() {}

protected:
    virtual Histogram* Convert(OriginalHistogram* original_histogram) const = 0;

    virtual std::string GenerateName(unsigned /*id*/, const std::string& name) const
    {
        return name;
    }

    virtual void Prepare(Histogram* histogram, const std::string& display_name,
                         const PlotOptions& plot_options) const
    {
        histogram->SetName(display_name.c_str());
        histogram->SetLineColor(plot_options.color);
        histogram->SetLineWidth(plot_options.line_width);
    }

private:
    std::vector< std::shared_ptr<TFile> > source_files;
    std::vector<std::string> display_names;
    std::vector<PlotOptions> plot_options_vector;
};

template<typename Histogram, typename ValueType=Double_t>
class SimpleHistogramSource : public HistogramSource<Histogram, ValueType, Histogram> {
protected:
    virtual Histogram* Convert(Histogram* original_histogram) const
    {
        return root_ext::CloneObject(*original_histogram);
    }
};

} // namespace root_ext
