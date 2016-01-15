#pragma once

#include <boost/filesystem.hpp>
#include <boost/algorithm/string/join.hpp>
#include <iostream>

#include <defines.h>
#include <uuid.h>

#include <TObject.h>
#include <TFile.h>
#include <TChain.h>

namespace plotIt {

  enum Type {
    MC,
    SIGNAL,
    DATA
  };

  enum ErrorsType {
      Normal = 0,
      Poisson = 1,
      Poisson2 = 2
  };

  inline ErrorsType string_to_errors_type(const std::string& s) {
      ErrorsType errors_type;

      if (s == "normal")
          errors_type = Normal;
      else if (s == "poisson2")
          errors_type = Poisson2;
      else 
          errors_type = Poisson;

      return errors_type;
  }

  enum Log {
    False = 0,
    True,
    Both
  };

  inline Log parse_log(const YAML::Node& node) {
    if (node.as<std::string>() == "both")
      return Both;
    else
      return node.as<bool>() ? True : False;
  }

  struct Summary {
    float n_events = 0;
    float n_events_error = 0;

    float efficiency = 0;
    float efficiency_error = 0;

    void clear() {
      n_events = n_events_error = efficiency = efficiency_error = 0;
    }
  };

  struct Systematic {
    std::string path;
    TObject* object = nullptr;
    std::map<std::string, TObject*> objects;
    std::shared_ptr<TFile> handle;

    Summary summary;
  };

  struct PlotStyle {

    // Style
    float marker_size;
    int16_t marker_color;
    int16_t marker_type;
    int16_t fill_color;
    int16_t fill_type;
    float line_width;
    int16_t line_color;
    int16_t line_type;
    std::string drawing_options;

    // Legend
    std::string legend;
    std::string legend_style;
    int16_t legend_order = 0;

    void loadFromYAML(YAML::Node& node, Type type);
  };

  struct File {
    std::string path;

    // For MC and Signal
    float cross_section = 1.;
    float branching_ratio = 1.;
    float generated_events = 1.;
    float scale = 1.;

    std::shared_ptr<PlotStyle> plot_style;
    std::string legend_group;
    std::string yields_group;

    Type type = MC;

    TObject* object = nullptr;
    std::map<std::string, TObject*> objects;

    std::vector<Systematic> systematics;

    int16_t order = std::numeric_limits<int16_t>::min();
    Summary summary;

    std::shared_ptr<TChain> chain;
    std::shared_ptr<TFile> handle;
  };

  struct Group {
    std::string name;
    std::shared_ptr<PlotStyle> plot_style;

    bool added = false;
  };

  struct Point {
    float x = std::numeric_limits<float>::quiet_NaN();
    float y = std::numeric_limits<float>::quiet_NaN();

    bool operator==(const Point& other) {
      return
        (fabs(x - other.x) < 1e-6) &&
        (fabs(y - other.y) < 1e-6);
    }

    bool valid() const {
      return !std::isnan(x) && !std::isnan(y);
    }

    Point() = default;
    Point(std::initializer_list<float> c) {
      assert(c.size() == 2);
      x = *c.begin();
      y = *(c.begin() + 1);
    }
  };

  struct Range {
    float start = std::numeric_limits<float>::quiet_NaN();
    float end = std::numeric_limits<float>::quiet_NaN();

    bool operator==(const Range& other) {
      return
        (std::abs(start - other.start) < 1e-6) &&
        (std::abs(end - other.end) < 1e-6);
    }

    bool valid() const {
      return !std::isnan(start) && !std::isnan(end);
    }

    Range() = default;
    Range(std::initializer_list<float> c) {
      assert(c.size() == 2);
      start = *c.begin();
      end = *(c.begin() + 1);
    }
  };

  struct Position {
    float x1 = 0;
    float y1 = 0;

    float x2 = 0;
    float y2 = 0;

    Position(float x1, float y1, float x2, float y2):
        x1(x1), y1(y1), x2(x2), y2(y2) {
        // Empty
    }

    Position() = default;

    bool operator==(const Position& other) {
      return
        (fabs(x1 - other.x1) < 1e-6) &&
        (fabs(y1 - other.y1) < 1e-6) &&
        (fabs(x2 - other.x2) < 1e-6) &&
        (fabs(y2 - other.y2) < 1e-6);
    }
  };

  struct Label {
    std::string text;
    uint32_t size = LABEL_FONTSIZE;
    Point position;
  };

  struct Plot {
    std::string name;
    std::string output_suffix;
    std::string uid = get_uuid();
    std::string exclude;

    bool no_data = false;
    bool override = false; // flag to plot only those which have it true (if at least one plot has it true)
    bool normalized = false;
    bool log_y = false;
    bool log_x = false;

    std::string x_axis;
    std::string y_axis = "Events";
    std::string y_axis_format;
    bool y_axis_show_zero = false;

    // Axis range
    Range x_axis_range;
    Range y_axis_range;
    Range ratio_y_axis_range = {0.5, 1.5};

    // Blind range
    Range blinded_range;

    uint16_t binning_x;  // Only used in tree mode
    uint16_t binning_y;  // Only used in tree mode

    std::string draw_string;  // Only used in tree mode
    std::string selection_string;  // Only used in tree mode


    std::vector<std::string> save_extensions = {"pdf"};

    bool show_ratio = false;

    bool fit = false;
    std::string fit_function = "gaus";
    std::string fit_legend = "#scale[1.6]{#splitline{#mu = %2$.3f}{#sigma = %3$.3f}}";
    Point fit_legend_position = {0.22, 0.87};
    Range fit_range;

    bool fit_ratio = false;
    std::string ratio_fit_function = "pol1";
    std::string ratio_fit_legend;
    Point ratio_fit_legend_position = {0.20, 0.38};
    Range ratio_fit_range;

    bool show_errors = true;
    bool show_overflow = false;

    std::string inherits_from = "TH1";

    uint16_t rebin = 1;

    std::vector<Label> labels;

    std::string extra_label;

    Position legend_position;
    size_t legend_columns;

    ErrorsType errors_type = Poisson;

    bool use_for_yields = false;
    std::string yields_title;
    int yields_table_order = 0;

    bool is_rescaled = false;
    
    void print() {
      std::cout << "Plot '" << name << "'" << std::endl;
      std::cout << "\tx_axis: " << x_axis << std::endl;
      std::cout << "\ty_axis: " << y_axis << std::endl;
      std::cout << "\tshow_ratio: " << show_ratio << std::endl;
      std::cout << "\tinherits_from: " << inherits_from << std::endl;
      std::cout << "\tsave_extensions: " << boost::algorithm::join(save_extensions, ", ") << std::endl;
    }

    Plot Clone(const std::string& new_name) {
      Plot clone = *this;
      clone.name = new_name;
      clone.uid = get_uuid();

      return clone;
    }
  };

  struct Legend {
    Position position = {0.6, 0.6, 0.9, 0.9};
    size_t columns = 1;
  };

  struct Configuration {
    float width = 800;
    float height = 800;
    float luminosity = -1;
    float scale = 1;

    // Systematics
    float luminosity_error_percent = 0;

    std::string y_axis_format = "%1% / %2$.2f";

    int16_t error_fill_color = 42;
    int16_t error_fill_style = 3154;

    uint16_t fit_n_points = 1000;
    int16_t fit_line_color = 46;
    int16_t fit_line_width = 1;
    int16_t fit_line_style = 1;
    int16_t fit_error_fill_color = 42;
    int16_t fit_error_fill_style = 1001;

    uint16_t ratio_fit_n_points = 1000;
    int16_t ratio_fit_line_color = 46;
    int16_t ratio_fit_line_width = 1;
    int16_t ratio_fit_line_style = 1;
    int16_t ratio_fit_error_fill_color = 42;
    int16_t ratio_fit_error_fill_style = 1001;

    std::vector<Label> labels;

    std::string experiment = "CMS";
    std::string extra_label;

    std::string lumi_label;
    std::string lumi_label_parsed;

    std::string root = "./";

    bool ignore_scales = false;
    bool verbose = false;
    bool show_overflow = false;
    bool do_plots = true;
    bool do_yields = false;

    std::string mode = "hist"; // "tree" or "hist"
    std::string tree_name;

    ErrorsType errors_type = Poisson;

    float yields_table_stretch = 1.15;
    std::string yields_table_align = "h";
    std::string yields_table_text_align = "c";
    int yields_table_num_prec_yields = 1;
    int yields_table_num_prec_ratio = 2;

    bool unblind = false;
    int16_t blinded_range_fill_color = 42;
    int16_t blinded_range_fill_style = 1001;
  };
}

namespace YAML {
  template<>
    struct convert<plotIt::Position> {
      static Node encode(const plotIt::Position& rhs) {
        Node node;
        node.push_back(rhs.x1);
        node.push_back(rhs.y1);
        node.push_back(rhs.x1);
        node.push_back(rhs.y1);

        return node;
      }

      static bool decode(const Node& node, plotIt::Position& rhs) {
        if(!node.IsSequence() || node.size() != 4)
          return false;

        rhs.x1 = node[0].as<float>();
        rhs.y1 = node[1].as<float>();
        rhs.x2 = node[2].as<float>();
        rhs.y2 = node[3].as<float>();

        return true;
      }
    };

  template<>
    struct convert<plotIt::Point> {
      static Node encode(const plotIt::Point& rhs) {
        Node node;
        node.push_back(rhs.x);
        node.push_back(rhs.y);

        return node;
      }

      static bool decode(const Node& node, plotIt::Point& rhs) {
        if(!node.IsSequence() || node.size() != 2)
          return false;

        rhs.x = node[0].as<float>();
        rhs.y = node[1].as<float>();

        return true;
      }
    };

  template<>
    struct convert<plotIt::Range> {
      static Node encode(const plotIt::Range& rhs) {
        Node node;
        node.push_back(rhs.start);
        node.push_back(rhs.end);

        return node;
      }

      static bool decode(const Node& node, plotIt::Range& rhs) {
        if(!node.IsSequence() || node.size() != 2)
          return false;

        rhs.start = node[0].as<float>();
        rhs.end = node[1].as<float>();

        return true;
      }
    };
}

