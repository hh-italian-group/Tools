/*! Produce sync plots between two groups for the selected distributions.
This file is part of https://github.com/hh-italian-group/AnalysisTools. */

#include <TKey.h>
#include <TCanvas.h>

#include "AnalysisTools/Run/include/program_main.h"
#include "AnalysisTools/Core/include/RootExt.h"
#include "AnalysisTools/Core/include/TextIO.h"
#include "AnalysisTools/Core/include/Tools.h"
#include "AnalysisTools/Core/include/PropertyConfigReader.h"
#include "AnalysisTools/Print/include/PlotPrimitives.h"

namespace analysis {
struct Arguments {
    REQ_ARG(std::string, cfg);
    REQ_ARG(std::string, output);
    REQ_ARG(std::vector<std::string>, input);
};

struct InputPattern {
    using Item = PropertyConfigReader::Item;
    using ItemCollection = PropertyConfigReader::ItemCollection;
    using RegexVector = std::vector<boost::regex>;

    InputPattern(const ItemCollection& config_items)
    {
        static const std::string targets_item_name = "targets";
        if(!config_items.count(targets_item_name))
            throw exception("Description of input patterns not found.");
        const auto& targets_item = config_items.at(targets_item_name);
        LoadPatterns(targets_item, "dir_names", dir_patterns);
        LoadPatterns(targets_item, "hist_names", hist_patterns);
    }

    bool DirMatch(const std::string& dir_name) const { return HasMatch(dir_name, dir_patterns); }
    bool HistMatch(const std::string& hist_name) const { return HasMatch(hist_name, hist_patterns); }

private:
    static void LoadPatterns(const Item& config_item, const std::string& p_name, RegexVector& patterns)
    {
        const std::string& p_value = config_item.Get<>(p_name);
        const auto& pattern_str_list = SplitValueList(p_value, false, " \t", true);
        for(const auto& pattern_str : pattern_str_list)
            patterns.emplace_back(pattern_str);
    }

    static bool HasMatch(const std::string& name, const RegexVector& patterns)
    {
        const std::string full_name = "^" + name + "$";
        for(const auto& pattern : patterns) {
            if(boost::regex_match(full_name, pattern))
                return true;
        }
        return false;
    }

private:
    RegexVector dir_patterns, hist_patterns;
};

struct Source {
    using ItemCollection = PropertyConfigReader::ItemCollection;
    using Hist = TH1;
    using HistPtr = std::shared_ptr<Hist>;
    using HistMap = std::map<std::string, HistPtr>;
    using DirHistMap = std::map<std::string, HistMap>;
    using ClassInheritance = root_ext::ClassInheritance;

    std::shared_ptr<TFile> file;
    std::string name;
    root_ext::Color color;
    DirHistMap histograms;

    Source(size_t n, const std::vector<std::string>& inputs, const ItemCollection& config_items,
           const InputPattern& pattern) :
        file(root_ext::OpenRootFile(inputs.at(n)))
    {
        const std::string item_name = boost::str(boost::format("input%1%") % n);
        if(!config_items.count(item_name))
            throw exception("Descriptor for input #%1% not found.") % n;
        const auto& desc = config_items.at(item_name);
        name = desc.Get<>("name");
        color = desc.Get<root_ext::Color>("color");
        LoadHistograms(pattern);
    }

private:
    void LoadHistograms(const InputPattern& pattern)
    {
        TIter nextkey(file->GetListOfKeys());
        for(TKey* t_key; (t_key = dynamic_cast<TKey*>(nextkey()));) {
            const std::string dir_name = t_key->GetName();
            const auto inheritance = root_ext::FindClassInheritance(t_key->GetClassName());
            if(inheritance != ClassInheritance::TDirectory || !pattern.DirMatch(dir_name)) continue;
            if(histograms.count(dir_name))
                throw exception("Directory '%1%' has been already processed.") % dir_name;
            auto dir = root_ext::ReadObject<TDirectory>(*file, dir_name);
            LoadHistograms(pattern, dir, histograms[dir_name]);
        }
    }

    static void LoadHistograms(const InputPattern& pattern, TDirectory *dir, HistMap& hists)
    {
        TIter nextkey(dir->GetListOfKeys());
        for(TKey* t_key; (t_key = dynamic_cast<TKey*>(nextkey()));) {
            const std::string hist_name = t_key->GetName();
            const auto inheritance = root_ext::FindClassInheritance(t_key->GetClassName());
            if(inheritance != ClassInheritance::TH1 || !pattern.HistMatch(hist_name)) continue;
            if(hists.count(hist_name))
                throw exception("Histogram '%1%' in directory '%2%' has been already processed.") % hist_name
                    % dir->GetName();
            hists[hist_name] = HistPtr(root_ext::ReadObject<TH1>(*dir, hist_name));
        }
    }
};

struct DrawOptions {
    using ItemCollection = PropertyConfigReader::ItemCollection;
    using Size = root_ext::Size<double, 2>;

    std::string x_title, y_title;
    bool divide_by_bin_width{false};
    Size canvas_size{600, 600};

    DrawOptions(const ItemCollection& config_items)
    {
        static const std::string item_name = "draw_opt";
        if(!config_items.count(item_name))
            throw exception("Draw options not found.");
        const auto& opt = config_items.at(item_name);
        if(opt.Has("x_title"))
            x_title = opt.Get<>("x_title");
        if(opt.Has("y_title"))
            y_title = opt.Get<>("y_title");
        if(opt.Has("div_bw"))
            divide_by_bin_width = opt.Get<bool>("div_bw");
        if(opt.Has("canvas_size"))
            canvas_size = opt.Get<Size>("canvas_size");
    }
};

class ShapeSync {
public:
    using InputDesc = PropertyConfigReader::Item;
    using NameSet = std::set<std::string>;
    using SampleItemNamesMap = std::map<std::string, NameSet>;

    ShapeSync(const Arguments& _args) :
        args(_args)
    {
        PropertyConfigReader config;
        config.Parse(args.cfg());
        if(args.input().size() < 2)
            throw exception("At least 2 inputs should be provided.");
        for(size_t n = 0; n < args.input().size(); ++n)
            inputs.emplace_back(n, args.input(), config.GetItems());
        patterns = std::make_shared<InputPattern>(config.GetItems());
        draw_options = std::make_shared<DrawOptions>(config.GetItems());
        canvas = std::make_shared<TCanvas>(draw_options->canvas_size.x(), draw_options->canvas_size.y());
    }

    void Run()
    {
        const auto common_dirs = GetCommonDirs();

        for(auto dir_iter = common_dirs.begin(); dir_iter != common_dirs.end(); ++dir_iter) {
            std::cout << "Processing directory " << *dir_iter << "..." << std::endl;
            const auto common_hists = GetCommonHists(*dir_iter);
            ReportNotCommonHists(*dir_iter, common_hists);
            PrintHistograms(common_hists, *dir_iter, std::next(dir_iter) != common_dirs.end());
        }
    }

private:
    NameSet GetCommonDirs() const
    {
        SampleItemNamesMap dir_names;
        for(const auto& input : inputs)
            dir_names[input.name] = tools::collect_map_keys(input.histograms);
        const NameSet common_dirs = CollectCommonItems(dir_names);
        ReportNotCommonItems(dir_names, common_dirs, "directories");
        return common_dirs;
    }

    static NameSet CollectCommonItems(const SampleItemNamesMap& items)
    {
        NameSet common_items;
        const auto first_input = items.begin();
        for(const auto& item : first_input->second) {
            bool is_common = true;
            for(auto input_iter = std::next(first_input); input_iter != items.end(); ++input_iter) {
                if(!input_iter->second.count(item)) {
                    is_common = false;
                    break;
                }
            }
            if(is_common)
                common_items.insert(item);
        }
        return common_items;
    }

    static void ReportNotCommonItems(const SampleItemNamesMap& items, const NameSet& common_items,
                                     const std::string& items_type_name)
    {
        bool has_not_common = false;
        for(auto input_iter = items.begin(); input_iter != inputs.end(); ++input_iter) {
            bool has_input_not_common = false;
            for(const auto& dir_entry : input_iter->histograms) {
                const std::string& dir_name = dir_entry.first;
                if(common_items.count(dir_name)) continue;
                if(!has_not_common)
                    std::cout << "Not common " << items_type_name << ":\n";
                has_not_common = true;
                if(!has_input_not_common)
                    std::cout << input_iter->name << ": ";
                else
                    std::cout << ", ";
                has_input_not_common = true;
                std::cout << dir_name;
            }
            if(has_input_not_common)
                std::cout << "\n";
        }
        if(has_not_common)
            std::cout << std::endl;
    }

    static void ReportNotCommonObjects(const NameSet& common_objects) const
    {
        bool has_not_common = false;
        for(auto input_iter = inputs.begin(); input_iter != inputs.end(); ++input_iter) {
            bool has_input_not_common = false;
            for(const auto& dir_entry : input_iter->histograms) {
                const std::string& dir_name = dir_entry.first;
                if(common_objects.count(dir_name)) continue;
                if(!has_not_common)
                    std::cout << "Not common directories:\n";
                has_not_common = true;
                if(!has_input_not_common)
                    std::cout << input_iter->name << ": ";
                else
                    std::cout << ", ";
                has_input_not_common = true;
                std::cout << dir_name;
            }
            if(has_input_not_common)
                std::cout << "\n";
        }
        if(has_not_common)
            std::cout << std::endl;
    }


    NameSet GetCommonHists(const std::string& dir_name) const
    {
        NameSet common_hists;
        const auto first_input = inputs.begin();
        for(const auto& hist_entry : first_input->histograms.at(dir_name)) {
            const std::string& hist_name = hist_entry.first;
            bool is_common = true;
            for(auto input_iter = std::next(first_input); input_iter != inputs.end(); ++input_iter) {
                if(!input_iter->histograms.at(dir_name).count(hist_name)) {
                    is_common = false;
                    break;
                }
            }
            if(is_common)
                common_hists.insert(hist_name);
        }
        return common_hists;

    }

    void PrintCanvas(const std::string& page_name, bool is_last_page)
    {
        std::ostringstream print_options, output_name;
        print_options << "Title:" << page_name;
        output_name << args.output();
        if(is_first_page && !is_last_page)
            output_name << "(";
        else if(is_last_page && !is_first_page)
            output_name << ")";
        is_first_page = false;
        canvas.Print(output_name.str().c_str(), print_options.str().c_str());
    }

    void DrawSuperimposedHistograms(std::shared_ptr<TH1F> Hmine, std::shared_ptr<TH1F> Hother,
                                    const std::string& selection_label, const std::string& mine_var,
                                    const std::string& other_var, const std::string& event_subset)
    {
        const std::string title = MakeTitle(mine_var, other_var, event_subset, selection_label);
        Hmine->SetTitle(title.c_str());
        Hmine->GetYaxis()->SetTitle("Events");
        Hmine->GetXaxis()->SetTitle(mine_var.c_str());
        Hmine->SetLineColor(1);
        Hmine->SetMarkerColor(1);
        Hmine->SetStats(0);

//        Hother->GetYaxis()->SetTitle(selection_label.c_str());
//        Hother->GetXaxis()->SetTitle(var.c_str());
        Hother->SetLineColor(2);
        Hother->SetMarkerColor(2);
        Hother->SetStats(0);

        TPad pad1("pad1","",0,0.2,1,1);
        TPad pad2("pad2","",0,0,1,0.2);

        pad1.cd();

        // Draw one histogram on top of the other
        if(Hmine->GetMaximum()>Hother->GetMaximum())
            Hmine->GetYaxis()->SetRangeUser(0,Hmine->GetMaximum()*1.1);
        else
            Hmine->GetYaxis()->SetRangeUser(0,Hother->GetMaximum()*1.1);
        Hmine->Draw("hist");
        Hother->Draw("histsame");
        DrawTextLabels(static_cast<size_t>(Hmine->Integral(0,Hmine->GetNbinsX()+1)),
                       static_cast<size_t>(Hother->Integral(0,Hother->GetNbinsX()+1)));

        pad2.cd();

        // Draw the ratio of the historgrams
        std::unique_ptr<TH1F> HDiff(dynamic_cast<TH1F*>(Hother->Clone("HDiff")));
        HDiff->Divide(Hmine.get());
        ///HDiff->GetYaxis()->SetRangeUser(0.9,1.1);
        HDiff->GetYaxis()->SetRangeUser(0.9,1.1);
        //HDiff->GetYaxis()->SetRangeUser(0.98,1.02);
        //HDiff->GetYaxis()->SetRangeUser(0.,2.0);
        HDiff->GetYaxis()->SetNdivisions(3);
        HDiff->GetYaxis()->SetLabelSize(0.1f);
        HDiff->GetYaxis()->SetTitleSize(0.1f);
        HDiff->GetYaxis()->SetTitleOffset(0.5);
        //HDiff->GetYaxis()->SetTitle(myGroup + " / " + group);
        HDiff->GetYaxis()->SetTitle("Ratio");
        HDiff->GetXaxis()->SetNdivisions(-1);
        HDiff->GetXaxis()->SetTitle("");
        HDiff->GetXaxis()->SetLabelSize(0.0001f);
        HDiff->SetMarkerStyle(7);
        HDiff->SetMarkerColor(2);
        HDiff->Draw("histp");
        TLine line;
        line.DrawLine(HDiff->GetXaxis()->GetXmin(),1,HDiff->GetXaxis()->GetXmax(),1);

        canvas.Clear();
        pad1.Draw();
        pad2.Draw();

        PrintCanvas(title);
    }

private:
    Arguments args;
    std::vector<Source> inputs;
    std::shared_ptr<InputPattern> patterns;
    std::shared_ptr<DrawOptions> draw_options;
    std::shared_ptr<TCanvas> canvas;
    bool is_first_page{true};
};

} // namespace analysis

PROGRAM_MAIN(analysis::ShapeSync, analysis::Arguments)
