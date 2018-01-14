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

    void Draw(TPad& main_pad, std::shared_ptr<TLegend> legend, std::vector<std::shared_ptr<TObject>>& plot_items)
    {
        main_pad.SetLogx(page_opt.log_x);
        main_pad.SetLogy(page_opt.log_y);

        if(legend) {
            if (data)
                legend->AddEntry(data_histogram.get(), data_histogram->GetLegendTitle().c_str(), "PLE");
            if (drawBKGerrors && sum_backgound_histogram)
                legend->AddEntry(sum_backgound_histogram.get(),sum_backgound_histogram->GetLegendTitle().c_str(), "f");
            for(const hist_ptr& signal : signal_histograms)
                legend->AddEntry(signal.get(), signal->GetLegendTitle().c_str(), "F");
            for(const hist_ptr& background : background_histograms)
                legend->AddEntry(background.get(), background->GetLegendTitle().c_str(), "F");
        }



        if (background_histograms.size()) {
            const auto& bkg_hist = background_histograms.front();
            stack = std::shared_ptr<THStack>(new THStack(bkg_hist->GetName(), bkg_hist->GetTitle()));

            for (auto iter = background_histograms.rbegin(); iter != background_histograms.rend(); ++iter){
                stack->Add(iter->get());
            }
            stack->Draw("HIST");

            Double_t maxY = stack->GetMaximum();
            if (data_histogram){
                const Int_t maxBin = data_histogram->GetMaximumBin();
                const Double_t maxData = data_histogram->GetBinContent(maxBin) + data_histogram->GetBinError(maxBin);
                maxY = std::max(maxY, maxData);
            }

            for (const hist_ptr& signal : signal_histograms)
                maxY = std::max(maxY,signal->GetMaximum());

            stack->SetMaximum(maxY * bkg_hist->MaxYDrawScaleFactor());

            const Double_t minY = page.side.use_log_scaleY ? 0.01 : 0;
            stack->SetMinimum(minY);


            if (draw_ratio){
                stack->GetXaxis()->SetTitle("");
                stack->GetXaxis()->SetLabelColor(kWhite);

            }
            else {
                stack->GetXaxis()->SetTitle(page.side.axis_titleX.c_str());
                stack->GetXaxis()->SetTitleOffset(1.00); //1.05
//                stack->GetXaxis()->SetTitleSize(0.055); //0.04
                stack->GetXaxis()->SetLabelSize(0.04f);
                stack->GetXaxis()->SetLabelOffset(0.015f);
//                stack->GetXaxis()->SetTitleFont(42); //62
            }

//            stack->GetYaxis()->SetTitleSize(0.055); //0.05
            stack->GetYaxis()->SetTitleOffset(1.4f); //1.45
            stack->GetYaxis()->SetLabelSize(0.04f);
            stack->GetYaxis()->SetTitle(page.side.axis_titleY.c_str());
//            stack->GetYaxis()->SetTitleFont(42); //62

            if (drawBKGerrors){
                sum_backgound_histogram->SetMarkerSize(0);
                //new
                // int new_idx = root_ext::CreateTransparentColor(12,0.5);
                // sum_backgound_histogram->SetFillColor(new_idx);
                // sum_backgound_histogram->SetFillStyle(3001);
                // sum_backgound_histogram->SetLineWidth(0);
                // sum_backgound_histogram->Draw("e2same");
//                end new

                  //old style for bkg uncertainties
               sum_backgound_histogram->SetFillColor(13);
               sum_backgound_histogram->SetFillStyle(3013);
               sum_backgound_histogram->SetLineWidth(0);
               sum_backgound_histogram->Draw("e2same");
            }
        }

        for(const hist_ptr& signal : signal_histograms)
            signal->Draw("SAME HIST");

        if(data_histogram) {
            data_histogram->SetMarkerColor(1);
            data_histogram->SetLineColor(1);
            data_histogram->SetFillColor(1);
            data_histogram->SetFillStyle(0);
            data_histogram->SetLineWidth(2);


            data_histogram->SetMarkerStyle(20);
            data_histogram->SetMarkerSize(1.1f);
            data_histogram->Draw("samepPE0");
//            data_histogram->Draw("pE0same");
        }

        const std::string axis_titleX = page.side.axis_titleX;
        if (data_histogram && draw_ratio){
            ratio_pad = std::shared_ptr<TPad>(root_ext::Adapter::NewPad(page.side.layout.ratio_pad));
            if(page.side.use_log_scaleX)
                ratio_pad->SetLogx();

            ratio_pad->Draw();

            ratio_pad->cd();


            ratio_histogram = hist_ptr(new Histogram(*data_histogram));
            ratio_histogram->Divide(sum_backgound_histogram.get());

            ratio_histogram->GetYaxis()->SetRangeUser(0.5,1.5);
            ratio_histogram->GetYaxis()->SetNdivisions(505);
            ratio_histogram->GetYaxis()->SetLabelSize(0.11f);
            ratio_histogram->GetYaxis()->SetTitleSize(0.14f);
            ratio_histogram->GetYaxis()->SetTitleOffset(0.55f);
            ratio_histogram->GetYaxis()->SetTitle("Obs/Bkg");
            ratio_histogram->GetYaxis()->SetTitleFont(62);
            ratio_histogram->GetXaxis()->SetNdivisions(510);
            ratio_histogram->GetXaxis()->SetTitle(axis_titleX.c_str());
            ratio_histogram->GetXaxis()->SetTitleSize(0.1f);
            ratio_histogram->GetXaxis()->SetTitleOffset(0.98f);
            ratio_histogram->GetXaxis()->SetTitleFont(62);
            //ratio_histogram->GetXaxis()->SetLabelColor(kBlack);
            ratio_histogram->GetXaxis()->SetLabelSize(0.1f);
            ratio_histogram->SetMarkerStyle(20);
            ratio_histogram->SetMarkerColor(1);
            ratio_histogram->SetMarkerSize(1);

            ratio_histogram->Draw("E0P");

            TLine* line = new TLine();
            line->SetLineStyle(3);
            line->DrawLine(ratio_histogram->GetXaxis()->GetXmin(), 1, ratio_histogram->GetXaxis()->GetXmax(), 1);
            TLine* line1 = new TLine();
            line1->SetLineStyle(3);
            line1->DrawLine(ratio_histogram->GetXaxis()->GetXmin(), 1.2, ratio_histogram->GetXaxis()->GetXmax(), 1.2);
            TLine* line2 = new TLine();
            line2->SetLineStyle(3);
            line2->DrawLine(ratio_histogram->GetXaxis()->GetXmin(), 0.8, ratio_histogram->GetXaxis()->GetXmax(), 0.8);
            ratio_pad->SetTopMargin(0.04f);
            ratio_pad->SetBottomMargin(0.3f);
            ratio_pad->Update();
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
