#pragma once

#include <plotter.h>

namespace plotIt {
  class TH1Plotter: public plotter {
    public:
      TH1Plotter(plotIt& plotIt):
        plotter(plotIt) {
        }

      virtual boost::optional<Summary> plot(TCanvas& c, Plot& plot);
      virtual bool supports(TObject& object);

    private:
      void setHistogramStyle(const File& file);
      void addOverflow(TH1* h, Type type, const Plot& plot);
  };
}
