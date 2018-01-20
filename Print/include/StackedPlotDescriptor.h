/*! Code to produce stacked plots.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#pragma once

#include <THStack.h>
#include "AnalysisTools/Core/include/SmartHistogram.h"
#include "DrawOptions.h"
#include "RootPrintTools.h"

namespace root_ext {

class StackedPlotDescriptor {
public:
    using exception = ::analysis::exception;
    using Hist = SmartHistogram<TH1D>;
    using HistPtr = std::shared_ptr<Hist>;
    using HistPtrVec = std::vector<HistPtr>;
    using Graph = TGraphAsymmErrors;
    using GraphPtr = std::shared_ptr<Graph>;
    using PageOptions = draw_options::Page;
    using HistOptions = draw_options::Histogram;

    StackedPlotDescriptor(const PageOptions& _page_opt, const draw_options::ItemCollection& opt_items) :
        page_opt(_page_opt)
    {
        auto iter = opt_items.find("sgn_hist");
        if(iter == opt_items.end())
            throw exception("Options to draw signal histograms not found.");
        signal_opt = HistOptions(iter->second);
        iter = opt_items.find("bkg_hist");
        if(iter == opt_items.end())
            throw exception("Options to draw background histograms not found.");
        bkg_opt = HistOptions(iter->second);
        iter = opt_items.find("data_hist");
        if(iter == opt_items.end())
            throw exception("Options to draw data histogram not found.");
        data_opt = HistOptions(iter->second);
    }

    void AddSignalHistogram(const Hist& original_hist, const std::string& legend_title, const Color& color,
                            double scale_factor)
    {
        auto hist = PrepareHistogram(original_hist, signal_opt, legend_title, color, signal_opt.fill_color, false);
        hist->Scale(scale_factor);
        signals.push_back(hist);
    }

    void AddBackgroundHistogram(const Hist& original_hist, const std::string& legend_title, const Color& color)
    {
        auto hist = PrepareHistogram(original_hist, bkg_opt, legend_title, bkg_opt.line_color, color, false);
        backgrounds.push_back(hist);
    }

    void AddDataHistogram(const Hist& original_hist, const std::string& legend_title)
    {
        if(data)
            throw exception("Only one data histogram per stack is supported.");
        data = PrepareHistogram(original_hist, data_opt, legend_title, data_opt.line_color, data_opt.fill_color, true);
    }

    bool HasPrintableContent() const { return signals.size() || backgrounds.size() || data; }

    void Draw(std::shared_ptr<TPad> main_pad, std::shared_ptr<TPad> /*ratio_pad*/, std::shared_ptr<TLegend> legend,
              std::vector<std::shared_ptr<TObject>>& plot_items)
    {
        main_pad->SetLogx(page_opt.log_x);
        main_pad->SetLogy(page_opt.log_y);

        main_pad->cd();
        if(backgrounds.size()) {
            auto stack = std::make_shared<THStack>("", "");
            for (auto iter = backgrounds.rbegin(); iter != backgrounds.rend(); ++iter){
                stack->Add(iter->get());
            }

            stack->Draw("HIST");
            plot_items.push_back(stack);
        }

        for(const auto& signal : signals)
            signal->Draw("SAME HIST");

        std::shared_ptr<TGraphAsymmErrors> data_graph;

        if(data) {
            data_graph = plotting::HistogramToGraph(*data, page_opt.divide_by_bin_width, data->GetBlindRanges());
            data_graph->Draw(data_opt.draw_opt.c_str());
            plot_items.push_back(data_graph);
        }

        if(legend) {
            if(data_graph)
                legend->AddEntry(data_graph.get(), data->GetLegendTitle().c_str(), data_opt.legend_style.c_str());
            for(const auto& signal : signals)
                legend->AddEntry(signal.get(), signal->GetLegendTitle().c_str(), signal_opt.legend_style.c_str());
            for(const auto& background : backgrounds)
                legend->AddEntry(background.get(), background->GetLegendTitle().c_str(), bkg_opt.legend_style.c_str());
        }
    }

private:
    void UpdatePageOptions(const Hist& hist)
    {
        page_opt.log_x = hist.UseLogX();
        page_opt.log_y = hist.UseLogY();
        page_opt.y_max_sf = hist.MaxYDrawScaleFactor();
        page_opt.x_title = hist.GetXTitle();
        page_opt.y_title = hist.GetYTitle();
        page_opt.divide_by_bin_width = hist.NeedToDivideByBinWidth();
    }

    HistPtr PrepareHistogram(const Hist& original_histogram, const HistOptions& opt, const std::string& legend_title,
                             const Color& line_color, const Color& fill_color, bool is_data)
    {
        auto hist = std::make_shared<Hist>(original_histogram);
        if(is_data)
            hist->SetBinErrorOption(TH1::kPoisson);
        else if (hist->NeedToDivideByBinWidth())
            DivideByBinWidth(*hist);
        hist->SetFillStyle(opt.fill_style);
        hist->SetFillColor(fill_color.GetColor_t());
        hist->SetLineStyle(opt.line_style);
        hist->SetLineWidth(opt.line_width);
        hist->SetLineColor(line_color.GetColor_t());
        hist->SetLegendTitle(legend_title);
        UpdatePageOptions(*hist);
        return hist;
    }

private:
    HistPtrVec signals, backgrounds;
    HistPtr data;

    PageOptions page_opt;
    HistOptions signal_opt, bkg_opt, data_opt;
};

} // namespace analysis
