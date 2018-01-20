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
    using MultiRange = Hist::MultiRange;

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

        if(bkg_opt.DrawUnc()) {
            iter = opt_items.find(bkg_opt.unc_hist);
            if(iter == opt_items.end())
                throw exception("Options to draw background uncertainties not found.");
            bkg_unc_opt = HistOptions(iter->second);
        }
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

    void Draw(std::shared_ptr<TPad> main_pad, std::shared_ptr<TPad> ratio_pad, std::shared_ptr<TLegend> legend,
              std::vector<std::shared_ptr<TObject>>& plot_items)
    {
        main_pad->SetLogx(page_opt.log_x);
        main_pad->SetLogy(page_opt.log_y);
        main_pad->cd();

        const bool has_ratio = ratio_pad.get() != nullptr;
        bool first_draw = true;
        HistPtr bkg_sum_hist;
        if(backgrounds.size()) {
            auto stack = std::make_shared<THStack>("", "");
            for (auto iter = backgrounds.rbegin(); iter != backgrounds.rend(); ++iter){
                stack->Add(iter->get());
            }

            DrawItem(*stack, bkg_opt.draw_opt, has_ratio, first_draw);
            plot_items.push_back(stack);

            bkg_sum_hist = CreateSumHistogram(backgrounds);
            if(bkg_unc_opt) {
                ApplyHistOptionsEx(*bkg_sum_hist, *bkg_unc_opt);
                DrawItem(*bkg_sum_hist, bkg_unc_opt->draw_opt, has_ratio, first_draw);
                plot_items.push_back(bkg_sum_hist);
            }
        }

        for(const auto& signal : signals)
            DrawItem(*signal, signal_opt.draw_opt, has_ratio, first_draw);

        std::shared_ptr<TGraphAsymmErrors> data_graph;
        if(data) {
            const auto blind_ranges = data_opt.blind ? data->GetBlindRanges() : MultiRange();
            data_graph = plotting::HistogramToGraph(*data, page_opt.divide_by_bin_width, blind_ranges);
            ApplyHistOptions(*data_graph, data_opt);
            DrawItem(*data_graph, data_opt.draw_opt, has_ratio, first_draw);
            plot_items.push_back(data_graph);
        }

        if(legend) {
            if(data_graph)
                legend->AddEntry(data_graph.get(), data->GetLegendTitle().c_str(), data_opt.legend_style.c_str());
            if(bkg_unc_opt)
                legend->AddEntry(bkg_sum_hist.get(), bkg_unc_opt->legend_title.c_str(),
                                 bkg_unc_opt->legend_style.c_str());
            for(const auto& signal : signals)
                legend->AddEntry(signal.get(), signal->GetLegendTitle().c_str(), signal_opt.legend_style.c_str());
            for(const auto& background : backgrounds)
                legend->AddEntry(background.get(), background->GetLegendTitle().c_str(), bkg_opt.legend_style.c_str());
        }

        if(ratio_pad && bkg_sum_hist && (bkg_unc_opt || data_graph)) {
            ratio_pad->cd();
            bool first_ratio_draw = true;
            if(bkg_unc_opt) {
                auto ratio_unc_hist = plotting::CreateNormalizedUncertaintyHistogram(*bkg_sum_hist);
                DrawRatioItem(*ratio_unc_hist, bkg_unc_opt->draw_opt, first_ratio_draw);
                plot_items.push_back(ratio_unc_hist);
            }
            if(data_graph) {
                auto ratio_graph = plotting::CreateRatioGraph(*data_graph, *bkg_sum_hist);
                DrawRatioItem(*ratio_graph, data_opt.draw_opt, first_ratio_draw);
                plot_items.push_back(ratio_graph);
            }
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

    template<typename Item>
    static void ApplyHistOptions(Item& item, const HistOptions& opt)
    {
        item.SetFillStyle(opt.fill_style);
        item.SetFillColor(opt.fill_color.GetColor_t());
        item.SetLineStyle(opt.line_style);
        item.SetLineWidth(opt.line_width);
        item.SetLineColor(opt.line_color.GetColor_t());
        item.SetMarkerStyle(opt.marker_style);
        item.SetMarkerSize(opt.marker_size);
        item.SetMarkerColor(opt.marker_color.GetColor_t());
    }

    static void ApplyHistOptionsEx(Hist& hist, const HistOptions& opt)
    {
        ApplyHistOptions(hist, opt);
        hist.SetLegendTitle(opt.legend_title);
    }


    HistPtr PrepareHistogram(const Hist& original_histogram, const HistOptions& opt, const std::string& legend_title,
                             const Color& line_color, const Color& fill_color, bool is_data)
    {
        auto hist = std::make_shared<Hist>(original_histogram);
        if(is_data)
            hist->SetBinErrorOption(TH1::kPoisson);
        else if (hist->NeedToDivideByBinWidth())
            DivideByBinWidth(*hist);
        auto opt_copy = opt;
        opt_copy.fill_color = fill_color;
        opt_copy.line_color = line_color;
        opt_copy.legend_title = legend_title;
        ApplyHistOptionsEx(*hist, opt_copy);
        UpdatePageOptions(*hist);
        return hist;
    }

    static HistPtr CreateSumHistogram(const HistPtrVec& hists)
    {
        if(hists.empty())
            throw exception("Unable to create sum histogram.");
        auto iter = hists.begin();
        auto sum_hist = std::make_shared<Hist>(**iter++);
        for(; iter != hists.end(); ++iter)
            sum_hist->Add(iter->get(), 1.);
        return sum_hist;
    }

    template<typename Item>
    void DrawItem(Item& item, const std::string& draw_opt, bool has_ratio, bool& first_draw) const
    {
        if(first_draw) {
            item.Draw(draw_opt.c_str());
            first_draw = false;
            item.GetYaxis()->SetTitle(page_opt.y_title.c_str());
            item.GetYaxis()->SetTitleSize(page_opt.axis_title_sizes.y());
            item.GetYaxis()->SetTitleOffset(page_opt.axis_title_offsets.y());
            item.GetYaxis()->SetLabelSize(page_opt.axis_label_sizes.y());
            item.GetYaxis()->SetLabelOffset(page_opt.axis_label_offsets.y());
            if(has_ratio) {
                item.GetXaxis()->SetTitle("");
                item.GetXaxis()->SetTitleSize(0);
                item.GetXaxis()->SetTitleOffset(0);
                item.GetXaxis()->SetLabelSize(0);
                item.GetXaxis()->SetLabelOffset(0);
            } else {
                item.GetXaxis()->SetTitle(page_opt.x_title.c_str());
                item.GetXaxis()->SetTitleSize(page_opt.axis_title_sizes.x());
                item.GetXaxis()->SetTitleOffset(page_opt.axis_title_offsets.x());
                item.GetXaxis()->SetLabelSize(page_opt.axis_label_sizes.x());
                item.GetXaxis()->SetLabelOffset(page_opt.axis_label_offsets.x());
            }
        } else {
            const auto opt = "SAME" + draw_opt;
            item.Draw(opt.c_str());
        }
    }

    template<typename Item>
    void DrawRatioItem(Item& item, const std::string& draw_opt, bool& first_draw) const
    {
        item.SetTitle("");
        if(first_draw) {
            item.Draw(draw_opt.c_str());
            first_draw = false;
            const float sf = page_opt.GetRatioPadSizeSF();
            item.GetXaxis()->SetTitle(page_opt.x_title.c_str());
            item.GetXaxis()->SetTitleSize(page_opt.axis_title_sizes.x() * sf);
            item.GetXaxis()->SetTitleOffset(page_opt.axis_title_offsets.x());
            item.GetXaxis()->SetLabelSize(page_opt.axis_label_sizes.x() * sf);
            item.GetXaxis()->SetLabelOffset(page_opt.axis_label_offsets.x());
            item.GetYaxis()->SetTitle(page_opt.ratio_y_title.c_str());
            item.GetYaxis()->SetTitleSize(page_opt.ratio_y_title_size * sf);
            item.GetYaxis()->SetTitleOffset(page_opt.ratio_y_title_offset);
            item.GetYaxis()->SetLabelSize(page_opt.ratio_y_label_size * sf);
            item.GetYaxis()->SetLabelOffset(page_opt.ratio_y_label_offset);
            item.GetYaxis()->SetNdivisions(page_opt.ratio_n_div_y);
        } else {
            const auto opt = "SAME" + draw_opt;
            item.Draw(opt.c_str());
        }
    }

private:
    HistPtrVec signals, backgrounds;
    HistPtr data;

    PageOptions page_opt;
    HistOptions signal_opt, bkg_opt, data_opt;
    boost::optional<HistOptions> bkg_unc_opt;
};

} // namespace analysis
