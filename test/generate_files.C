// This is a ROOT macro

#include <TFile.h>
#include <TH1.h>
#include <TFormula.h>
#include <TF1.h>

#include <iostream>

void generate_files() {
    const float luminosity = 100;  // 100 /pb

    const float mc1_gen_events = 21675970;
    const float mc1_xsection = 245.8;

    const float mc2_gen_events = 24045248;
    const float mc2_xsection = 666.3;

    const uint32_t n_data = luminosity * ( mc1_xsection + mc2_xsection);
    std::cout << n_data << std::endl;

    auto sqroot = new TFormula("sqroot", "x*gaus(0) + [3]*abs(sin(x)/x)");
    auto sqroot_tf = new TF1("sqroot_tf", "sqroot", 0, 10);
    sqroot_tf->SetParameters(10,4,1,20);

    // MC1 file
    auto f_mc1 = TFile::Open("files/MC_sample1.root", "recreate");

    auto h1_mc1 = new TH1F("histo1", "histo1", 200, 0, 10);
    h1_mc1->FillRandom("sqroot_tf", mc1_gen_events);
    
    auto h2_mc1 = new TH1F("histo2", "histo2", 200, -3, 3);
    h2_mc1->FillRandom("gaus", mc1_gen_events);

    f_mc1->Write();


    // MC2 file
    auto sqroot_tf2 = new TF1("sqroot_tf2", "sqroot", 0, 10);
    sqroot_tf2->SetParameters(10, 8, 1.3, 20);

    auto f_mc2 = TFile::Open("files/MC_sample2.root", "recreate");

    auto h1_mc2 = new TH1F("histo1", "histo1", 200, 0, 10);
    h1_mc2->FillRandom("sqroot_tf2", mc2_gen_events);
    
    auto h2_mc2 = new TH1F("histo2", "histo2", 200, -3, 3);
    h2_mc2->FillRandom("gaus", mc2_gen_events);

    f_mc2->Write();


    // Data
    auto h1_sum = new TH1F("histo1_temp", "histo1", 200, 0, 10);
    h1_sum->Add(h1_mc1, luminosity * mc1_xsection / mc1_gen_events);
    h1_sum->Add(h1_mc2, luminosity * mc2_xsection / mc2_gen_events);

    auto h2_sum = new TH1F("histo2_temp", "histo1", 200, -3, 3);
    h2_sum->Add(h2_mc1, luminosity * mc1_xsection / mc1_gen_events);
    h2_sum->Add(h2_mc2, luminosity * mc2_xsection / mc2_gen_events);

    auto f_data = TFile::Open("files/data.root", "recreate");

    auto h1_data = new TH1F("histo1", "histo1", 200, 0, 10);
    h1_data->FillRandom(h1_sum, n_data);

    auto h2_data = new TH1F("histo2", "histo2", 200, -3, 3);
    h2_data->FillRandom(h2_sum, n_data);

    f_data->Write();
}
