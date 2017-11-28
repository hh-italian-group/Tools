/*! Print ROOT histograms to PDF.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#pragma once

#include <sstream>

#include <TROOT.h>
#include <TStyle.h>
#include <Rtypes.h>
#include <TError.h>

#include "DrawOptions.h"
#include "RootPrintTools.h"

namespace root_ext {
class PdfPrinter {
    using PageOptions = draw_options::Page;

public:
    PdfPrinter(const std::string& _output_file_name, const PageOptions& page_opt) :
        canvas(plotting::NewCanvas(page_opt.canvas_size)), output_file_name(_output_file_name)
    {

        gStyle->SetPaperSize(page_opt.paper_size.x(), page_opt.paper_size.y());
        gStyle->SetPalette(page_opt.palette);
        gStyle->SetEndErrorSize(page_opt.end_error_size);

        canvas->SetFillColor(page_opt.canvas_color.GetColor_t());
        canvas->SetBorderSize(page_opt.canvas_border_size);
        canvas->SetBorderMode(page_opt.canvas_border_mode);

        if(page_opt.HasMainPad()) {
            main_pad = plotting::NewPad(page_opt.main_pad);
            plotting::SetMargins(*main_pad, page_opt.margins);
        } else {
            main_pad = canvas;
        }

        canvas->SetBorderMode(0);
        canvas->SetFrameFillStyle(0);
        canvas->SetFrameLineColor(kWhite);
        canvas->SetFrameBorderMode(0);
//        canvas->SetLeftMargin( L/W );
//        canvas->SetRightMargin( R/W );
//        canvas->SetTopMargin( T/H );
//        canvas->SetBottomMargin( B/H );
//        canvas->SetTickx(0);
//        canvas->SetTicky(0);

        const Int_t old_gErrorIgnoreLevel = gErrorIgnoreLevel;
        if(!verbose)
            gErrorIgnoreLevel = kWarning;
        canvas->Print((output_file_name + "[").c_str());
        gErrorIgnoreLevel = old_gErrorIgnoreLevel;
    }

    template<typename Source>
    void Print(const Page& page, const Source& source)
    {
        gROOT->SetStyle(page.layout.global_style.c_str());
        gStyle->SetOptStat(page.layout.stat_options);
        gStyle->SetOptFit(page.layout.fit_options);
        canvas->cd();

        canvas->SetTitle(page.title.c_str());
        if(page.layout.has_title) {
            TPaveLabel *title = Adapter::NewPaveLabel(page.layout.title_box, page.title);
            title->SetTextFont(page.layout.title_font);
            title->Draw();
        }

        Page::RegionCollection page_regions = page.Regions();
        for(Page::RegionCollection::const_iterator iter = page_regions.begin(); iter != page_regions.end(); ++iter)
        {
            canvas->cd();
            DrawHistograms(*(*iter), source);
        }

        canvas->Draw();
        std::ostringstream print_options;
        print_options << "Title: " << page.title;
        canvas->Print(output_file_name.c_str(), print_options.str().c_str());
        ++n_pages;
    }

    void PrintStack(analysis::StackedPlotDescriptor& stackDescriptor, bool is_last)
    {
        if(!stackDescriptor.NeedDraw())
            return;
        canvas->cd();
        canvas->SetTitle(stackDescriptor.GetTitle().c_str());
        canvas->Clear();
        stackDescriptor.Draw(*canvas);
        canvas->Draw();
        std::ostringstream print_options;
        print_options << "Title: " << stackDescriptor.GetTitle();
        const Int_t old_gErrorIgnoreLevel = gErrorIgnoreLevel;
        gErrorIgnoreLevel = kWarning;
        canvas->Print(output_file_name.c_str(), print_options.str().c_str());
        gErrorIgnoreLevel = old_gErrorIgnoreLevel;
        ++n_pages;
    }

    ~PdfPrinter()
    {
        const Int_t old_gErrorIgnoreLevel = gErrorIgnoreLevel;
        gErrorIgnoreLevel = kWarning;
        if (n_pages > 1){
            canvas->Clear();
            canvas->Print(output_file_name.c_str());
        }
        canvas->Print((output_file_name+"]").c_str());
        gErrorIgnoreLevel = old_gErrorIgnoreLevel;
        if(verbose)
            std::cout << "Info in <TCanvas::Print>: pdf file " << output_file_name << " has been closed" << std::endl;
    }

private:
    template<typename Source>
    void DrawHistograms(const PageSide& page_side, const Source& source)
    {
        typedef root_ext::HistogramPlotter<typename Source::Histogram, typename Source::ValueType> Plotter;
        typedef root_ext::HistogramFitter<typename Source::Histogram, typename Source::ValueType> Fitter;

        TPad* stat_pad = 0;
        if(page_side.layout.has_stat_pad) {
            stat_pad = Adapter::NewPad(page_side.layout.stat_pad);
            stat_pad->Draw();
        }

        TPad *pad = Adapter::NewPad(page_side.layout.main_pad);
        if(page_side.use_log_scaleX)
            pad->SetLogx();
        if(page_side.use_log_scaleY)
            pad->SetLogy();
        pad->Draw();
        pad->cd();

        Plotter plotter(page_side.histogram_title, page_side.axis_titleX, page_side.axis_titleY);
        for(unsigned n = 0; n < source.Size(); ++n)
        {
            const typename Plotter::Entry entry = source.Get(n, page_side.histogram_name);
            plotter.Add(entry);
        }

        Fitter::SetRanges(plotter.Histograms(), page_side.fit_range_x, page_side.fit_range_y, page_side.xRange,
                          page_side.yRange, page_side.use_log_scaleY);
        plotter.Superpose(pad, stat_pad, page_side.layout.has_legend, page_side.layout.legend_pad,
                          page_side.draw_options);
    }

private:
    std::shared_ptr<TCanvas> canvas;
    std::shared_ptr<TPad> main_pad;
    std::string output_file_name;
    bool has_first_page{false}, has_last_page{false};
};

} // namespace root_ext
