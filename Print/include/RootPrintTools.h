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
std::shared_ptr<TCanvas> NewCanvas(const Size<T, 2>& size)
{
    static const char* canvas_name = "canvas";
    return std::make_shared<TCanvas>(canvas_name, canvas_name, size.x(), size.y());
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

template<typename Range = ::analysis::Range<double>>
std::shared_ptr<TGraphAsymmErrors> HistogramToGraph(const TH1& hist, bool divide_by_bin_width = false,
                                                    const ::analysis::MultiRange<Range>& blind_ranges = {})
{
    std::vector<double> x, y, exl, exh, eyl, eyh;
    size_t n = 0;
    for(int bin = 1; bin <= hist.GetNbinsX(); ++bin) {
        const Range bin_range(hist.GetBinLowEdge(bin), hist.GetBinLowEdge(bin + 1),
                              ::analysis::RangeBoundaries::MinIncluded);
        if(blind_ranges.Overlaps(bin_range))
            continue;
        x.push_back(hist.GetBinCenter(bin));
        exl.push_back(x[n] - hist.GetBinLowEdge(bin));
        exh.push_back(hist.GetBinLowEdge(bin + 1) - x[n]);
        y.push_back(hist.GetBinContent(bin));
        eyl.push_back(hist.GetBinErrorLow(bin));
        eyh.push_back(hist.GetBinErrorUp(bin));
        if(divide_by_bin_width) {
            const double bin_width = hist.GetBinWidth(bin);
            y[n] /= bin_width;
            eyl[n] /= bin_width;
            eyh[n] /= bin_width;
        }
        ++n;
    }
    return std::make_shared<TGraphAsymmErrors>(static_cast<int>(n), x.data(), y.data(), exl.data(), exh.data(),
                                               eyl.data(), eyh.data());
}

} // namespace plotting
} // root_ext
