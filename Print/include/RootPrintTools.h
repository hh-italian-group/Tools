/*! Contains useful functions to print histograms with ROOT.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#pragma once

#include <limits>
#include <vector>
#include <map>

#include <Rtypes.h>
#include <TPaveStats.h>
#include <TPaveLabel.h>
#include <TPad.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TColor.h>
#include <TRatioPlot.h>

#include "AnalysisTools/Core/include/RootExt.h"
#include "AnalysisTools/Core/include/NumericPrimitives.h"
#include "PlotPrimitives.h"

namespace root_ext {

template<typename Histogram, typename ValueType=Double_t>
class HistogramRangeTuner {
public:
    using Range = ::analysis::Range<ValueType>;
    using OptValue = ::boost::optional<ValueType>;

    static std::pair<Int_t, Int_t> GetBinRangeX(const Histogram& h, bool consider_overflow_and_underflow)
    {
        return consider_overflow_and_underflow
                ? std::make_pair(0, h.GetNbinsX() + 1) : std::make_pair(1, h.GetNbinsX());
    }

    static ValueType FindMinLimitX(const Histogram& h)
    {
        for(Int_t i = 1; i <= h.GetNbinsX(); ++i) {
            if(h.GetBinContent(i) != ValueType(0))
                return h.GetBinLowEdge(i);
        }
        return std::numeric_limits<ValueType>::max();
    }

    static ValueType FindMaxLimitX(const Histogram& h)
    {
        for(Int_t i = h.GetNbinsX(); i > 0; --i) {
            if(h.GetBinContent(i) != ValueType(0))
                return h.GetBinLowEdge(i) + h.GetBinWidth(i);
        }
        return std::numeric_limits<ValueType>::lowest();
    }

    static ValueType FindMinLimitY(const Histogram& h, bool consider_overflow_and_underflow = false)
    {
        ValueType min = std::numeric_limits<ValueType>::max();
        const auto bin_range = GetBinRangeX(h, consider_overflow_and_underflow);
        for(Int_t i = bin_range.first; i <= bin_range.second; ++i) {
            if(h.GetBinContent(i) != ValueType(0))
                min = std::min(min, h.GetBinContent(i));
        }
        return min;
    }

    static ValueType FindMaxLimitY(const Histogram& h, bool consider_overflow_and_underflow = false)
    {
        ValueType max = std::numeric_limits<ValueType>::lowest();
        const auto bin_range = GetBinRangeX(h, consider_overflow_and_underflow);
        for(Int_t i = bin_range.first; i <= bin_range.second; ++i) {
            if(h.GetBinContent(i) != ValueType(0))
                max = std::max(max, h.GetBinContent(i));
        }
        return max;
    }

    void Add(const Histogram& hist, bool consider_overflow_and_underflow = false)
    {
        x_min = std::min(x_min, FindMinLimitX(hist));
        x_max = std::max(x_max, FindMinLimitX(hist));
        x_min = std::min(x_min, FindMinLimitX(hist, consider_overflow_and_underflow));
        x_min = std::min(x_min, FindMinLimitX(hist, consider_overflow_and_underflow));
    }

    void SetRangeX(TAxis& x_axis) const
    {
        x_axis.SetRangeUser(x_min, x_max);
    }

    void SetRangeY(TAxis& y_axis, bool log_y = false, ValueType max_y_sf = 1, ValueType min_y_sf = 1) const
    {
        const ValueType y_min_value = log_y ? std::min(y_min * min_y_sf, std::numeric_limits<ValueType>::min())
                                            : y_min * min_y_sf;
        y_axis.SetRangeUser(y_min_value, y_max * max_y_sf);
    }

public:
    ValueType x_min{std::numeric_limits<ValueType>::max()};
    ValueType x_max{std::numeric_limits<ValueType>::lowest()};
    ValueType y_min{std::numeric_limits<ValueType>::max()};
    ValueType y_max{std::numeric_limits<ValueType>::lowest()};
};

namespace plotting {
template<typename T>
std::shared_ptr<TPaveLabel> NewPaveLabel(const Box<T>& box, const std::string& text)
{
    return std::make_shared<TPaveLabel>(box.left_bottom().x(), box.left_bottom().y(),
                                        box.right_top().x(), box.right_top().y(), text.c_str());
}

template<typename T>
std::shared_ptr<TPad> NewPad(const Box<T>& box)
{
    static const char* pad_name = "pad";
    return std::make_shared<TPad>(pad_name, pad_name, box.left_bottom().x(), box.left_bottom().y(),
                                  box.right_top().x(), box.right_top().y());
}

template<typename T>
void SetMargins(TPad& pad, const MarginBox<T>& box)
{
    pad.SetLeftMargin(box.left());
    pad.SetBottomMargin(box.bottom());
    pad.SetRightMargin(box.right());
    pad.SetTopMargin(box.top());
}

template<typename T>
void SetMargins(TRatioPlot& plot, const MarginBox<T>& box)
{
    plot.SetLeftMargin(box.left());
    plot.SetLowBottomMargin(box.bottom());
    plot.SetRightMargin(box.right());
    plot.SetUpTopMargin(box.top());
}

} // namespace plotting

template<typename Histogram, typename ValueType=Double_t>
class HistogramPlotter {
public:
    struct Options {
        Color_t color;
        Width_t line_width;
        Box<double> pave_stats_box;
        Double_t pave_stats_text_size;
        Color_t fit_color;
        Width_t fit_line_width;
        Options() : color(kBlack), line_width(1), pave_stats_text_size(0), fit_color(kBlack), fit_line_width(1) {}
        Options(Color_t _color, Width_t _line_width, const Box<double>& _pave_stats_box, Double_t _pave_stats_text_size,
            Color_t _fit_color, Width_t _fit_line_width)
            : color(_color), line_width(_line_width), pave_stats_box(_pave_stats_box),
              pave_stats_text_size(_pave_stats_text_size), fit_color(_fit_color), fit_line_width(_fit_line_width) {}
    };

    struct Entry {
        Histogram* histogram;
        Options plot_options;
        Entry(Histogram* _histogram, const Options& _plot_options)
            : histogram(_histogram), plot_options(_plot_options) {}
    };

    using HistogramContainer = std::vector<Histogram*>;

public:
    HistogramPlotter(const std::string& _title, const std::string& _axis_titleX, const std::string& _axis_titleY)
        : title(_title), axis_titleX(_axis_titleX),axis_titleY(_axis_titleY) {}

    void Add(Histogram* histogram, const Options& plot_options)
    {
        histograms.push_back(histogram);
        options.push_back(plot_options);
    }

    void Add(const Entry& entry)
    {
        histograms.push_back(entry.histogram);
        options.push_back(entry.plot_options);
    }

    void Superpose(TPad* main_pad, TPad* stat_pad, bool draw_legend, const Box<double>& legend_box,
                   const std::string& draw_options)
    {
        if(!histograms.size() || !main_pad)
            return;

        histograms[0]->SetTitle(title.c_str());
        histograms[0]->GetXaxis()->SetTitle(axis_titleX.c_str());
        histograms[0]->GetYaxis()->SetTitle(axis_titleY.c_str());

        TLegend* legend = 0;
        if(draw_legend) {
            legend = new TLegend(legend_box.left_bottom().x(), legend_box.left_bottom().y(),
                                 legend_box.right_top().x(), legend_box.right_top().y());
        }

        for(unsigned n = 0; n < histograms.size(); ++n) {
            main_pad->cd();
            const Options& o = options[n];
            Histogram* h = histograms[n];
            if(!h)
                continue;

            const char* opt = n ? "sames" : draw_options.c_str();
            h->Draw(opt);
            if(legend) {
                legend->AddEntry(h, h->GetName());
                legend->Draw();
            }

            main_pad->Update();
            if(!stat_pad)
                continue;
            stat_pad->cd();
            TPaveStats *pave_stats = dynamic_cast<TPaveStats*>(h->GetListOfFunctions()->FindObject("stats"));

            TPaveStats *pave_stats_copy = root_ext::CloneObject(*pave_stats);
            h->SetStats(0);

            pave_stats_copy->SetX1NDC(o.pave_stats_box.left_bottom().x());
            pave_stats_copy->SetX2NDC(o.pave_stats_box.right_top().x());
            pave_stats_copy->SetY1NDC(o.pave_stats_box.left_bottom().y());
            pave_stats_copy->SetY2NDC(o.pave_stats_box.right_top().y());
            pave_stats_copy->ResetAttText();
            pave_stats_copy->SetTextColor(o.color);
            pave_stats_copy->SetTextSize(static_cast<float>(o.pave_stats_text_size));
            pave_stats_copy->Draw();
            stat_pad->Update();
        }
    }

    const HistogramContainer& Histograms() const { return histograms; }

private:
    HistogramContainer histograms;
    std::vector<Options> options;
    std::string title;
    std::string axis_titleX, axis_titleY;
};

} // root_ext
