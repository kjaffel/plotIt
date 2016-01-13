#pragma once

#include <plotIt.h>
#include <summary.h>

#include <boost/optional.hpp>

class TCanvas;
class TObject;

namespace plotIt {
  class plotter {

    public:
      plotter(plotIt& plotIt):
        m_plotIt(plotIt) {

        }


      virtual boost::optional<Summary> plot(TCanvas& c, Plot& plot) = 0;
      virtual bool supports(TObject& object) = 0;

    protected:
      plotIt& m_plotIt;

  };
}
