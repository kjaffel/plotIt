#include "plotIt.h"

// For fnmatch()
#include <fnmatch.h>

#include <TList.h>
#include <TCollection.h>
#include <TCanvas.h>
#include <TError.h>
#include <TFile.h>
#include <TKey.h>
#include <TLatex.h>
#include <TLegend.h>
#include <TLegendEntry.h>
#include <TPaveText.h>
#include <TColor.h>
#include <TGaxis.h>

#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <set>
#include <iomanip>

#include "tclap/CmdLine.h"

#include <boost/regex.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <plotters.h>
#include <pool.h>
#include <summary.h>
#include <systematics.h>
#include <utilities.h>


namespace fs = boost::filesystem;
using std::setw;

// Load libdpm at startup, on order to be sure that rfio files are working
#include <dlfcn.h>
struct Dummy
{
  Dummy()
  {
    dlopen("libdpm.so", RTLD_NOW|RTLD_GLOBAL);
  }
};
static Dummy foo;

namespace plotIt {

  plotIt::plotIt(const fs::path& outputPath, const std::string& configFile):
    m_outputPath(outputPath) {

      createPlotters(*this);

      gErrorIgnoreLevel = kError;
      m_style.reset(createStyle());

      TH1::AddDirectory(false);

      parseConfigurationFile(configFile);
    }

  // Replace the "include" fields by the content they point to
  void plotIt::parseIncludes(YAML::Node& node) {

    if (node["include"]) {
        std::vector<std::string> files = node["include"].as<std::vector<std::string>>();
        node.remove("include");

        for (std::string& file: files) {
          YAML::Node root = YAML::LoadFile(file);

          for (YAML::const_iterator it = root.begin(); it != root.end(); ++it) {
            node[it->first.as<std::string>()] = it->second;
          }
        }

        if (node["include"])
            parseIncludes(node);
    }

    for(YAML::iterator it = node.begin(); it != node.end(); ++it) {
      if (it->second.Type() == YAML::NodeType::Map)
          parseIncludes(it->second);
    }
  }

  void plotIt::parseSystematicsNode(const YAML::Node& node) {

      std::string type;
      std::string name;
      YAML::Node configuration;

      switch (node.Type()) {
        case YAML::NodeType::Scalar:
            name = node.as<std::string>();
            type = "shape";
            break;

        case YAML::NodeType::Map: {
            const auto& it = *node.begin();

            if (it.second.IsScalar())
                type = "const";
            else if (it.second["type"])
                type = it.second["type"].as<std::string>();
            else
                type = "shape";

            name = it.first.as<std::string>();
            configuration = it.second;
            } break;

        default:
            throw YAML::ParserException(node.Mark(), "Invalid systematics node. Must be either a string or a map");
      }

      m_systematics.push_back(SystematicFactory::create(name, type, configuration));
  }

  void plotIt::parseConfigurationFile(const std::string& file) {
    YAML::Node f = YAML::LoadFile(file);

    parseIncludes(f);

    if (! f["files"]) {
      throw YAML::ParserException(YAML::Mark::null_mark(), "Your configuration file must have a 'files' list");
    }

    const auto& parseLabelsNode = [](YAML::Node& node) -> std::vector<Label> {
      std::vector<Label> labels;

      for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
        const YAML::Node& labelNode = *it;

        Label label;
        label.text = labelNode["text"].as<std::string>();
        label.position = labelNode["position"].as<Point>();

        if (labelNode["size"])
          label.size = labelNode["size"].as<uint32_t>();

        labels.push_back(label);
      }

      return labels;
    };

    // Retrieve legend configuration
    if (f["legend"]) {
      YAML::Node node = f["legend"];

      if (node["position"])
        m_legend.position = node["position"].as<Position>();

      if (node["columns"])
          m_legend.columns = node["columns"].as<size_t>();
    }


    // Retrieve plotIt configuration
    if (f["configuration"]) {
      YAML::Node node = f["configuration"];

      if (node["width"])
        m_config.width = node["width"].as<float>();

      if (node["height"])
        m_config.height = node["height"].as<float>();

      if (node["experiment"])
        m_config.experiment = node["experiment"].as<std::string>();

      if (node["extra-label"])
        m_config.extra_label = node["extra-label"].as<std::string>();

      if (node["luminosity-label"])
        m_config.lumi_label = node["luminosity-label"].as<std::string>();

      if (node["root"])
        m_config.root = node["root"].as<std::string>();

      if (node["scale"])
        m_config.scale = node["scale"].as<float>();

      if (node["luminosity"])
        m_config.luminosity = node["luminosity"].as<float>();
      else {
        throw YAML::ParserException(YAML::Mark::null_mark(), "'configuration' block is missing luminosity");
      }

      if (node["luminosity-error"]) {
        float value = node["luminosity-error"].as<float>();

        if (value > 0) {
          // Create a 'luminosity' systematic error
          YAML::Node syst;
          syst["type"] = "const";
          syst["pretty-name"] = "Luminosity";
          syst["value"] = value + 1;

          YAML::Node syst_node;
          syst_node["lumi"] = syst;

          f["systematics"].push_back(syst_node);
        }
      }

      if (node["error-fill-color"])
        m_config.error_fill_color = loadColor(node["error-fill-color"]);

      if (node["error-fill-style"])
        m_config.error_fill_style = loadColor(node["error-fill-style"]);

      if (node["fit-line-style"])
        m_config.fit_line_style = node["fit-line-style"].as<int16_t>();

      if (node["fit-line-width"])
        m_config.fit_line_width = node["fit-line-width"].as<int16_t>();

      if (node["fit-line-color"])
        m_config.fit_line_color = loadColor(node["fit-line-color"]);

      if (node["fit-error-fill-style"])
        m_config.fit_error_fill_style = node["fit-error-fill-style"].as<int16_t>();

      if (node["fit-error-fill-color"])
        m_config.fit_error_fill_color = loadColor(node["fit-error-fill-color"]);

      if (node["fit-n-points"])
        m_config.fit_n_points = node["fit-n-points"].as<uint16_t>();

      if (node["ratio-fit-line-style"])
        m_config.ratio_fit_line_style = node["ratio-fit-line-style"].as<int16_t>();

      if (node["ratio-fit-line-width"])
        m_config.ratio_fit_line_width = node["ratio-fit-line-width"].as<int16_t>();

      if (node["ratio-fit-line-color"])
        m_config.ratio_fit_line_color = loadColor(node["ratio-fit-line-color"]);

      if (node["ratio-fit-error-fill-style"])
        m_config.ratio_fit_error_fill_style = node["ratio-fit-error-fill-style"].as<int16_t>();

      if (node["ratio-fit-error-fill-color"])
        m_config.ratio_fit_error_fill_color = loadColor(node["ratio-fit-error-fill-color"]);

      if (node["ratio-fit-n-points"])
        m_config.ratio_fit_n_points = node["ratio-fit-n-points"].as<uint16_t>();

      if (node["blinded-range-fill-color"])
        m_config.blinded_range_fill_color = loadColor(node["blinded-range-fill-color"]);

      if (node["blinded-range-fill-style"])
        m_config.blinded_range_fill_style = node["blinded-range-fill-style"].as<uint16_t>();

      m_config.line_style.parse(node);

      if (node["labels"]) {
        YAML::Node labels = node["labels"];
        m_config.labels = parseLabelsNode(labels);
      }

      if (node["y-axis-format"])
        m_config.y_axis_format = node["y-axis-format"].as<std::string>();

      if (node["mode"])
          m_config.mode = node["mode"].as<std::string>();

      if (node["tree-name"])
          m_config.tree_name = node["tree-name"].as<std::string>();

      if (node["show-overflow"])
          m_config.show_overflow = node["show-overflow"].as<bool>();

      if (node["errors-type"])
          m_config.errors_type = string_to_errors_type(node["errors-type"].as<std::string>());

      if (node["yields-table-stretch"])
        m_config.yields_table_stretch = node["yields-table-stretch"].as<float>();

      if (node["yields-table-align"])
        m_config.yields_table_align = node["yields-table-align"].as<std::string>();

      if (node["yields-table-text-align"])
        m_config.yields_table_text_align = node["yields-table-text-align"].as<std::string>();

      if (node["yields-table-numerical-precision-yields"])
        m_config.yields_table_num_prec_yields = node["yields-table-numerical-precision-yields"].as<int>();

      if (node["yields-table-numerical-precision-ratio"])
        m_config.yields_table_num_prec_ratio = node["yields-table-numerical-precision-ratio"].as<int>();
    }

    // Retrieve files/processes configuration
    YAML::Node files = f["files"];

    for (YAML::const_iterator it = files.begin(); it != files.end(); ++it) {
      File file;

      file.path = it->first.as<std::string>();
      fs::path root = fs::path(m_config.root);
      fs::path path = fs::path(file.path);
      file.path = (root / path).string();

      YAML::Node node = it->second;

      if (node["pretty-name"]) {
        file.pretty_name = node["pretty-name"].as<std::string>();
      } else {
        file.pretty_name = path.stem().native();
      }

      if (node["type"]) {
        std::string type = node["type"].as<std::string>();
        if (type == "signal")
          file.type = SIGNAL;
        else if (type == "data")
          file.type = DATA;
        else
          file.type = MC;
      }

      if (node["scale"])
        file.scale = node["scale"].as<float>();

      if (node["cross-section"])
        file.cross_section = node["cross-section"].as<float>();

      if (node["branching-ratio"])
        file.branching_ratio = node["branching-ratio"].as<float>();

      if (node["generated-events"])
        file.generated_events = node["generated-events"].as<float>();

      if (node["order"])
        file.order = node["order"].as<int16_t>();

      if (node["group"])
        file.legend_group = node["group"].as<std::string>();

      if (node["yields-group"]) {
        file.yields_group = node["yields-group"].as<std::string>();
      } else if(node["group"]) {
        file.yields_group = node["group"].as<std::string>();
      } else if(node["legend"]) {
        file.yields_group = node["legend"].as<std::string>();
      } else {
        file.yields_group = file.path;
      }

      file.plot_style = std::make_shared<PlotStyle>();
      file.plot_style->loadFromYAML(node, file.type);

      m_files.push_back(file);
    }

    std::sort(m_files.begin(), m_files.end(), [](const File& a, const File& b) {
      return a.order < b.order;
     });

    YAML::Node legend_groups = f["groups"];

    for (YAML::const_iterator it = legend_groups.begin(); it != legend_groups.end(); ++it) {
      Group group;

      group.name = it->first.as<std::string>();

      YAML::Node node = it->second;

      // Find the first file belonging to this group, and use its type to set
      // default style values
      const auto file = std::find_if(m_files.begin(), m_files.end(), [&group](const File& file) {
          return file.legend_group == group.name;
        });

      // Is this group actually used?
      if (file == m_files.end())
          continue;

      group.plot_style = std::make_shared<PlotStyle>();
      group.plot_style->loadFromYAML(node, file->type);

      m_legend_groups[group.name] = group;
    }

    // Remove non-existant groups from files
    for (auto& file: m_files) {
      if (!file.legend_group.empty() && !m_legend_groups.count(file.legend_group)) {
        file.legend_group = "";
      }
    }

    // List systematics
    if (f["systematics"]) {
        YAML::Node systs = f["systematics"];

        for (YAML::const_iterator it = systs.begin(); it != systs.end(); ++it) {
            parseSystematicsNode(*it);
        }
    }

    // Retrieve plots configuration
    if (! f["plots"]) {
      throw YAML::ParserException(YAML::Mark::null_mark(), "You must specify at least one plot in your configuration file");
    }

    YAML::Node plots = f["plots"];

    for (YAML::const_iterator it = plots.begin(); it != plots.end(); ++it) {
      Plot plot;

      plot.name = it->first.as<std::string>();

      YAML::Node node = it->second;
      if (node["exclude"])
        plot.exclude = node["exclude"].as<std::string>();

      if (node["x-axis"])
        plot.x_axis = node["x-axis"].as<std::string>();

      if (node["y-axis"])
        plot.y_axis = node["y-axis"].as<std::string>();

      plot.y_axis_format = m_config.y_axis_format;
      if (node["y-axis-format"])
        plot.y_axis_format = node["y-axis-format"].as<std::string>();

      if (node["normalized"])
        plot.normalized = node["normalized"].as<bool>();

      if (node["no-data"])
        plot.no_data = node["no-data"].as<bool>();

      if (node["override"])
        plot.override = node["override"].as<bool>();

      Log log_y = False;
      if (node["log-y"]) {
        log_y = parse_log(node["log-y"]);
      }
      if (log_y != Both)
        plot.log_y = (bool) log_y;

      Log log_x = False;
      if (node["log-x"]) {
        log_x = parse_log(node["log-x"]);
      }
      if (log_x != Both)
        plot.log_x = (bool) log_x;

      if (node["save-extensions"])
        plot.save_extensions = node["save-extensions"].as<std::vector<std::string>>();

      if (node["show-ratio"])
        plot.show_ratio = node["show-ratio"].as<bool>();

      if (node["fit-ratio"])
        plot.fit_ratio = node["fit-ratio"].as<bool>();

      if (node["fit"])
        plot.fit = node["fit"].as<bool>();

      if (node["fit-function"])
        plot.fit_function = node["fit-function"].as<std::string>();

      if (node["fit-legend"])
        plot.fit_legend = node["fit-legend"].as<std::string>();

      if (node["fit-legend-position"])
        plot.fit_legend_position = node["fit-legend-position"].as<Point>();

      if (node["fit-range"])
        plot.fit_range = node["fit-range"].as<Range>();

      if (node["ratio-fit-function"])
        plot.ratio_fit_function = node["ratio-fit-function"].as<std::string>();

      if (node["ratio-fit-legend"])
        plot.ratio_fit_legend = node["ratio-fit-legend"].as<std::string>();

      if (node["ratio-fit-legend-position"])
        plot.ratio_fit_legend_position = node["ratio-fit-legend-position"].as<Point>();

      if (node["ratio-fit-range"])
        plot.ratio_fit_range = node["ratio-fit-range"].as<Range>();

      if (node["show-errors"])
        plot.show_errors = node["show-errors"].as<bool>();

      if (node["x-axis-range"])
        plot.x_axis_range = node["x-axis-range"].as<Range>();

      if (node["y-axis-range"])
        plot.y_axis_range = node["y-axis-range"].as<Range>();

      if (node["ratio-y-axis-range"])
        plot.ratio_y_axis_range = node["ratio-y-axis-range"].as<Range>();

      if (node["blinded-range"])
        plot.blinded_range = node["blinded-range"].as<Range>();

      if (node["y-axis-show-zero"])
        plot.y_axis_show_zero = node["y-axis-show-zero"].as<bool>();

      if (node["inherits-from"])
        plot.inherits_from = node["inherits-from"].as<std::string>();

      if (node["rebin"])
        plot.rebin = node["rebin"].as<uint16_t>();

      if (node["labels"]) {
        YAML::Node labels = node["labels"];
        plot.labels = parseLabelsNode(labels);
      }

      if (node["extra-label"])
        plot.extra_label = node["extra-label"].as<std::string>();

      if (node["legend-position"])
        plot.legend_position = node["legend-position"].as<Position>();
      else
        plot.legend_position = m_legend.position;

      if (node["legend-columns"])
        plot.legend_columns = node["legend-columns"].as<size_t>();
      else
        plot.legend_columns = m_legend.columns;

      if (node["show-overflow"])
        plot.show_overflow = node["show-overflow"].as<bool>();
      else
        plot.show_overflow = m_config.show_overflow;

      if (node["errors-type"])
        plot.errors_type = string_to_errors_type(node["errors-type"].as<std::string>());
      else
        plot.errors_type = m_config.errors_type;

      if (node["binning-x"])
        plot.binning_x = node["binning-x"].as<uint16_t>();

      if (node["binning-y"])
        plot.binning_y = node["binning-y"].as<uint16_t>();

      if (node["draw-string"])
        plot.draw_string = node["draw-string"].as<std::string>();

      if (node["selection-string"])
        plot.selection_string = node["selection-string"].as<std::string>();

      if (node["for-yields"])
        plot.use_for_yields = node["for-yields"].as<bool>();

      if (node["yields-title"])
        plot.yields_title = node["yields-title"].as<std::string>();
      else
        plot.yields_title = plot.name;

      if (node["yields-table-order"])
        plot.yields_table_order = node["yields-table-order"].as<int>();

      if (node["vertical-lines"]) {
        for (const auto& line: node["vertical-lines"]) {
          plot.lines.push_back(Line(line, VERTICAL));
        }
      }

      if (node["horizontal-lines"]) {
        for (const auto& line: node["horizontal-lines"]) {
          plot.lines.push_back(Line(line, HORIZONTAL));
        }
      }

      if (node["lines"]) {
        for (const auto& line: node["lines"]) {
          plot.lines.push_back(Line(line, UNSPECIFIED));
        }
      }

      for (auto& line: plot.lines) {
        if (! line.style)
          line.style = m_config.line_style;
      }

      // Handle log
      std::vector<bool> logs_x;
      std::vector<bool> logs_y;

      if (log_x == Both) {
        logs_x.insert(logs_x.end(), {false, true});
      } else {
        logs_x.push_back(plot.log_x);
      }

      if (log_y == Both) {
        logs_y.insert(logs_y.end(), {false, true});
      } else {
        logs_y.push_back(plot.log_y);
      }

      int log_counter(0);
      for (auto x: logs_x) {
        for (auto y: logs_y) {
          Plot p = plot;
          p.log_x = x;
          p.log_y = y;
          // If the plot is used for yields, they should be output only once
          if(log_counter && plot.use_for_yields)
            p.use_for_yields = false;

          if (p.log_x)
            p.output_suffix += "_logx";

          if (p.log_y)
            p.output_suffix += "_logy";

          m_plots.push_back(p);
          ++log_counter;
        }
      }
    }

    // If at least one plot has 'override' set to true, keep only plots which do
    if( std::find_if(m_plots.begin(), m_plots.end(), [](Plot &plot){ return plot.override; }) != m_plots.end() ){
      auto new_end = std::remove_if(m_plots.begin(), m_plots.end(), [](Plot &plot){ return !plot.override; });
      m_plots.erase(new_end, m_plots.end());
    }

    parseLumiLabel();
  }

  void plotIt::parseLumiLabel() {

    m_config.lumi_label_parsed = m_config.lumi_label;

    float lumi = m_config.luminosity / 1000.;

    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << lumi;
    std::string lumiStr = out.str();

    boost::algorithm::replace_all(m_config.lumi_label_parsed, "%lumi%", lumiStr);
  }

  void plotIt::fillLegend(TLegend& legend, const Plot& plot, bool with_uncertainties) {
      struct Entry {
          TObject* object = nullptr;
          std::string legend;
          std::string style;
          int16_t order;

          std::string name;
          int16_t fill_style;
          int16_t fill_color;
          uint16_t line_width;

          Entry() = default;
          Entry(TObject* object, const std::string& legend, const std::string& style, int16_t order):
              object(object), legend(legend), style(style), order(order) {
              // Empty
          }

          Entry(const std::string& name, const std::string& legend, const std::string& style, int16_t fill_style, int16_t fill_color, uint16_t line_width):
              object(nullptr), legend(legend), style(style), order(0), name(name), fill_style(fill_style), fill_color(fill_color), line_width(line_width) {
              // Empty
          }

          void stylize(TLegendEntry* entry) {
              if (object)
                  return;

              entry->SetLineWidth(line_width);
              entry->SetLineColor(fill_color);
              entry->SetFillStyle(fill_style);
              entry->SetFillColor(fill_color);
          }
      };

      std::vector<Entry> legend_entries[plot.legend_columns];

      auto getEntryFromFile = [&](File& file, Entry& entry) {
          if (file.legend_group.length() > 0 && m_legend_groups.count(file.legend_group) && m_legend_groups[file.legend_group].plot_style->legend.length() > 0) {
              if (m_legend_groups[file.legend_group].added)
                  return false;
              m_legend_groups[file.legend_group].added = true;

              const auto& plot_style = m_legend_groups[file.legend_group].plot_style;
              entry = {file.object, plot_style->legend, plot_style->legend_style, plot_style->legend_order};
          } else if (file.plot_style.get() && file.plot_style->legend.length() > 0) {
              entry = {file.object, file.plot_style->legend, file.plot_style->legend_style, file.plot_style->legend_order};
          }

          return true;
      };

      auto getEntries = [&](Type type) {
          std::vector<Entry> entries;
          for (File& file: m_files) {
              if (file.type == type) {
                  Entry entry;
                  if (getEntryFromFile(file, entry)) {
                      entries.push_back(entry);
                  }
              }
          }

          std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) { return a.order > b.order; });

          return entries;
      };

      // First, add data, always on first column
      if (!plot.no_data) {
          std::vector<Entry> entries = getEntries(DATA);
          for (const auto& entry: entries)
              legend_entries[0].push_back(entry);
      }

      // Then MC, spanning on the remaining columns
      size_t index = 0;
      std::vector<Entry> entries = getEntries(MC);
      for (const Entry& entry: entries) {
          size_t column_index = (plot.legend_columns == 1) ? 0 : ((index % (plot.legend_columns - 1)) + 1);
          legend_entries[column_index].push_back(entry);
          index++;
      }

      // Signal, also on the first column
      entries = getEntries(SIGNAL);
      for (const Entry& entry: entries) {
          legend_entries[0].push_back(entry);
      }

      // Finally, if requested, the uncertainties entry
      if (with_uncertainties)
          legend_entries[0].push_back({"errors", "Uncertainties", "f", m_config.error_fill_style, m_config.error_fill_color, 0});

      // Ensure all columns have the same size
      size_t max_size = 0;
      for (size_t i = 0; i < plot.legend_columns; i++) {
          max_size = std::max(max_size, legend_entries[i].size());
      }

      for (size_t i = 0; i < plot.legend_columns; i++) {
          legend_entries[i].resize(max_size, Entry());
      }

      // Add entries to the legend
      for (size_t i = 0; i < (plot.legend_columns * max_size); i++) {
          size_t column_index = (i % plot.legend_columns);
          size_t row_index = static_cast<size_t>(i / static_cast<float>(plot.legend_columns));
          Entry& entry = legend_entries[column_index][row_index];
          TLegendEntry* e = legend.AddEntry(entry.object, entry.legend.c_str(), entry.style.c_str());
          entry.stylize(e);
      }
  }

  bool plotIt::plot(Plot& plot) {
    std::cout << "Plotting '" << plot.name << "'" << std::endl;

    bool hasMC = false;
    bool hasData = false;
    bool hasSignal = false;
    bool hasLegend = false;
    // Open all files, and find histogram in each
    for (File& file: m_files) {
      if (! loadObject(file, plot)) {
        return false;
      }

      hasLegend |= getPlotStyle(file)->legend.length() > 0;
      hasData |= file.type == DATA;
      hasMC |= file.type == MC;
      hasSignal |= file.type == SIGNAL;
    }

    // Create canvas
    TCanvas c("canvas", "canvas", m_config.width, m_config.height);

    boost::optional<Summary> summary = ::plotIt::plot(m_files[0], c, plot);

    if (! summary)
      return false;

    if (m_config.verbose) {
      ConsoleSummaryPrinter printer;
      printer.print(*summary);
    }

    if (plot.log_y)
      c.SetLogy();

    if (plot.log_x)
      c.SetLogx();

    Position legend_position = plot.legend_position;

    // Build legend
    TLegend legend(legend_position.x1, legend_position.y1, legend_position.x2, legend_position.y2);
    legend.SetTextFont(43);
    legend.SetFillStyle(0);
    legend.SetBorderSize(0);
    legend.SetNColumns(plot.legend_columns);

    fillLegend(legend, plot, hasMC && plot.show_errors);

    legend.Draw();

    float topMargin = TOP_MARGIN;
    if (plot.show_ratio)
      topMargin /= .6666;

    // Move exponent label if shown
    TGaxis::SetExponentOffset(-0.06, 0, "y");

    // Luminosity label
    if (m_config.lumi_label_parsed.length() > 0) {
      std::shared_ptr<TPaveText> pt = std::make_shared<TPaveText>(LEFT_MARGIN, 1 - 0.5 * topMargin, 1 - RIGHT_MARGIN, 1, "brNDC");
      TemporaryPool::get().add(pt);

      pt->SetFillStyle(0);
      pt->SetBorderSize(0);
      pt->SetMargin(0);
      pt->SetTextFont(42);
      pt->SetTextSize(0.6 * topMargin);
      pt->SetTextAlign(33);

      pt->AddText(m_config.lumi_label_parsed.c_str());
      pt->Draw();
    }

    // Experiment
    if (m_config.experiment.length() > 0) {
      std::shared_ptr<TPaveText> pt = std::make_shared<TPaveText>(LEFT_MARGIN, 1 - 0.5 * topMargin, 1 - RIGHT_MARGIN, 1, "brNDC");
      TemporaryPool::get().add(pt);

      pt->SetFillStyle(0);
      pt->SetBorderSize(0);
      pt->SetMargin(0);
      pt->SetTextFont(62);
      pt->SetTextSize(0.75 * topMargin);
      pt->SetTextAlign(13);

      std::string text = m_config.experiment;
      if (m_config.extra_label.length() || plot.extra_label.length()) {
        std::string extra_label = plot.extra_label;
        if (extra_label.length() == 0) {
          extra_label = m_config.extra_label;
        }

        boost::format fmt("%s #font[52]{#scale[0.76]{%s}}");
        fmt % m_config.experiment % extra_label;

        text = fmt.str();
      }

      pt->AddText(text.c_str());
      pt->Draw();
    }

    c.cd();

    const auto& labels = mergeLabels(plot.labels);

    // Labels
    for (auto& label: labels) {

      std::shared_ptr<TLatex> t(new TLatex(label.position.x, label.position.y, label.text.c_str()));
      t->SetNDC(true);
      t->SetTextFont(43);
      t->SetTextSize(label.size);
      t->Draw();

      TemporaryPool::get().add(t);
    }

    std::string plot_name = plot.name + plot.output_suffix;
    boost::replace_all(plot_name, "/", "_");
    fs::path outputName = m_outputPath / plot_name;

    for (const std::string& extension: plot.save_extensions) {
      fs::path outputNameWithExtension = outputName.replace_extension(extension);

      c.SaveAs(outputNameWithExtension.string().c_str());
    }

    // Clean all temporary resources
    TemporaryPool::get().clear();

    // Reset groups
    for (auto& group: m_legend_groups) {
      group.second.added = false;
    }

    return true;
  }

  bool plotIt::yields(std::vector<Plot>& plots){
    std::cout << "Producing LaTeX yield table.\n";

    std::map<std::string, double> data_yields;

    std::map< std::string, std::map<std::string, std::pair<double, double> > > mc_yields;
    std::map< std::string, double > mc_total;
    std::map< std::string, double > mc_total_sqerrs;
    std::set<std::string> mc_processes;

    std::map< std::string, std::map<std::string, std::pair<double, double> > > signal_yields;
    std::set<std::string> signal_processes;

    std::map<
        std::tuple<Type, std::string>,
        double
    > process_systematics;

    std::map<
        std::string,
        std::map<Type, double>
    > total_systematics_squared;

    std::vector< std::pair<int, std::string> > categories;

    bool has_data(false);

    for(Plot& plot: plots){
      if (!plot.use_for_yields)
        continue;

      replace_substr(plot.yields_title, "_", "\\_");
      if( std::find_if(categories.begin(), categories.end(), [&](const std::pair<int, std::string> &x){ return x.second == plot.yields_title; }) != categories.end() )
        return false;
      categories.push_back( std::make_pair(plot.yields_table_order, plot.yields_title) );

      std::map<std::tuple<Type, std::string>, double> plot_total_systematics;

      // Open all files, and find histogram in each
      for (File& file: m_files) {
        if (! loadObject(file, plot)) {
          std::cout << "Could not retrieve plot from " << file.path << std::endl;
          return false;
        }

        if ( file.type == DATA ){
          data_yields[plot.yields_title] += dynamic_cast<TH1*>(file.object)->Integral();
          has_data = true;
          continue;
        }

        std::string process_name = file.yields_group;
        replace_substr(process_name, "_", "\\_");
        replace_substr(process_name, "\\", "\\\\");
        std::pair<double, double> yield_sqerror;
        TH1* hist( dynamic_cast<TH1*>(file.object) );

        double factor = m_config.luminosity * file.cross_section * file.branching_ratio / file.generated_events;
        if (!m_config.ignore_scales)
          factor *= m_config.scale * file.scale;

        if (!plot.is_rescaled)
          hist->Scale(factor);

        for (auto& syst: *file.systematics) {
          syst.update();
          syst.scale(factor);
        }

        // Retrieve yield and stat. error
        yield_sqerror.first = hist->IntegralAndError(1, hist->GetNbinsX(), yield_sqerror.second);
        yield_sqerror.second = std::pow(yield_sqerror.second,2);

        // Add systematics
        double file_total_systematics = 0;
        for (auto& syst: *file.systematics) {

          TH1* nominal_shape = static_cast<TH1*>(syst.nominal_shape.get());
          TH1* up_shape = static_cast<TH1*>(syst.up_shape.get());
          TH1* down_shape = static_cast<TH1*>(syst.down_shape.get());

          if (! nominal_shape || ! up_shape || ! down_shape)
              continue;

          double total_syst_error = 0;
          for (size_t i = 1; i <= (size_t) nominal_shape->GetNbinsX(); i++) {
            float syst_error_up = std::abs(up_shape->GetBinContent(i) - nominal_shape->GetBinContent(i));
            float syst_error_down = std::abs(nominal_shape->GetBinContent(i) - down_shape->GetBinContent(i));

            // FIXME: Add support for asymetric errors
            float syst_error = std::max(syst_error_up, syst_error_down);

            total_syst_error += syst_error;
          }

          file_total_systematics += total_syst_error * total_syst_error;

          auto key = std::make_tuple(file.type, syst.name());
          plot_total_systematics[key] += total_syst_error;
        }

        process_systematics[std::make_tuple(file.type, process_name)] += std::sqrt(file_total_systematics);

        if ( file.type == MC ){
          ADD_PAIRS(mc_yields[plot.yields_title][process_name], yield_sqerror);
          mc_total[plot.yields_title] += yield_sqerror.first;
          mc_total_sqerrs[plot.yields_title] += yield_sqerror.second;
          mc_processes.emplace(process_name);
        }
        if ( file.type == SIGNAL ){
          ADD_PAIRS(signal_yields[plot.yields_title][process_name], yield_sqerror);
          signal_processes.emplace(process_name);
        }
      }

      // Get the total systematics for this category
      for (auto& syst: plot_total_systematics) {
        total_systematics_squared[plot.yields_title][std::get<0>(syst.first)] += syst.second * syst.second;
      }
    }

    if( ( !(mc_processes.size()+signal_processes.size()) && !has_data ) || !categories.size() ){
      std::cout << "No processes/data/categories defined\n";
      return false;
    }

    // Sort according to user-defined order
    std::sort(categories.begin(), categories.end(), [](const std::pair<int, std::string>& cat1, const std::pair<int, std::string>& cat2){  return cat1.first < cat2.first; });

    std::ostringstream latexString;
    latexString << "\\renewcommand{\\arraystretch}{" << m_config.yields_table_stretch << "}\n";
    std::string tab("    ");

    latexString << std::setiosflags(std::ios_base::fixed);

    if( m_config.yields_table_align.find("h") != std::string::npos ){

      latexString << "\\begin{tabular}{ |l||";

      // tabular config.
      for(size_t i = 0; i < signal_processes.size(); ++i)
        latexString << m_config.yields_table_text_align << "|";
      if(signal_processes.size())
        latexString << "|";
      for(size_t i = 0; i < mc_processes.size(); ++i)
        latexString << m_config.yields_table_text_align << "|";
      if(mc_processes.size())
        latexString << "|" + m_config.yields_table_text_align << "||";
      if(has_data)
        latexString << m_config.yields_table_text_align << "||";
      if(has_data && mc_processes.size())
        latexString << m_config.yields_table_text_align << "||";
      latexString.seekp(latexString.tellp() - 2l);
      latexString << "| }\n" << tab << tab << "\\hline\n";

      // title line
      latexString << "    Cat. & ";
      for(auto &proc: signal_processes)
        latexString << proc << " & ";
      for(auto &proc: mc_processes)
        latexString << proc << " & ";
      if( mc_processes.size() )
        latexString << "Tot. MC & ";
      if( has_data )
        latexString << "Data & ";
      if( has_data && mc_processes.size() )
        latexString << "Data/MC & ";
      latexString.seekp(latexString.tellp() - 2l);
      latexString << "\\\\\n" << tab << tab << "\\hline\n";

      // loop over each category
      for(auto& cat_pair: categories){

        std::string categ(cat_pair.second);
        latexString << tab << categ << " & ";
        latexString << std::setprecision(m_config.yields_table_num_prec_yields);

        for(auto &proc: signal_processes)
          latexString << "$" << signal_yields[categ][proc].first << " \\pm " << std::sqrt(signal_yields[categ][proc].second + std::pow(process_systematics[std::make_tuple(SIGNAL, proc)], 2)) << "$ & ";

        for(auto &proc: mc_processes)
          latexString << "$" << mc_yields[categ][proc].first << " \\pm " << std::sqrt(mc_yields[categ][proc].second + std::pow(process_systematics[std::make_tuple(MC, proc)], 2)) << "$ & ";
        if( mc_processes.size() )
          latexString << "$" << mc_total[categ] << " \\pm " << std::sqrt(mc_total_sqerrs[categ] + total_systematics_squared[categ][MC]) << "$ & ";

        if( has_data )
          latexString << "$" << std::setprecision(0) << data_yields[categ] << "$ & ";

        if( has_data && mc_processes.size() ){
          double ratio = data_yields[categ] / mc_total[categ];
          double error_data = 0;
          double error_mc = std::sqrt(mc_total_sqerrs[categ] + total_systematics_squared[categ][MC]);

          double error = ratio * std::sqrt(std::pow(error_data / data_yields[categ], 2) +  std::pow(error_mc / mc_total[categ], 2));

          latexString << std::setprecision(m_config.yields_table_num_prec_ratio) << "$" << ratio << " \\pm " << error << "$ & ";
        }

        latexString.seekp(latexString.tellp() - 2l);
        latexString << "\\\\\n";
      }

      latexString << tab << tab << "\\hline\n\\end{tabular}\n";

    } else {
      std::cerr << "Error: yields table alignment " << m_config.yields_table_align << " is not recognized (for now, only \"h\" is supported)" << std::endl;
      return false;
    }

    if(m_config.verbose)
      std::cout << "LaTeX yields table:\n\n" << latexString.str() << std::endl;

    fs::path outputName(m_outputPath);
    outputName /= "yields.tex";

    std::ofstream out(outputName.string());
    out << latexString.str();
    out.close();

    return true;
  }

  void plotIt::plotAll() {
    // First, explode plots to match all glob patterns

    std::vector<Plot> plots;
    if (m_config.mode == "tree") {
      plots = m_plots;
    } else {
      if (!expandObjects(m_files[0], plots)) {
        return;
      }
    }

    std::cout << "Loading all plots..." << std::endl;
    for (File& file: m_files) {
      loadAllObjects(file, plots);

      file.handle.reset();
      file.friend_handles.clear();
    }
    std::cout << "Done." << std::endl;

    if(m_config.do_plots){
      for (Plot& plot: plots) {
        plotIt::plot(plot);
      }
    }
    if(m_config.do_yields){
      plotIt::yields(plots);
    }
  }

  bool plotIt::loadAllObjects(File& file, const std::vector<Plot>& plots) {

    file.object = nullptr;
    file.objects.clear();

    if (m_config.mode == "tree") {

        if (!file.chain.get()) {
          file.chain.reset(new TChain(m_config.tree_name.c_str()));
          file.chain->Add(file.path.c_str());
        }

        for (const auto& plot: plots) {
          std::shared_ptr<TH1> hist(new TH1F(plot.name.c_str(), "", plot.binning_x, plot.x_axis_range.start, plot.x_axis_range.end));
          hist->GetDirectory()->cd();

          file.chain->Draw((plot.draw_string + ">>" + plot.name).c_str(), plot.selection_string.c_str());

          hist->SetDirectory(nullptr);
          file.objects.emplace(plot.uid, hist.get());

          TemporaryPool::get().add(hist);
        }

        return true;
    }

    file.handle.reset(TFile::Open(file.path.c_str()));
    if (! file.handle.get())
      return false;

    file.systematics_cache.clear();

    for (const auto& plot: plots) {
      TObject* obj = file.handle->Get(plot.name.c_str());

      if (obj) {
        std::shared_ptr<TObject> cloned_obj(obj->Clone());
        TemporaryPool::get().addRuntime(cloned_obj);

        file.objects.emplace(plot.uid, cloned_obj.get());

        if (file.type != DATA) {
          for (auto& syst: m_systematics) {
              if (std::regex_search(file.path, syst->on))
                  file.systematics_cache[plot.uid].push_back(syst->newSet(cloned_obj.get(), file, plot));
          }
        }

        continue;
      }

      // Should not be possible!
      std::cout << "Error: object '" << plot.name << "' inheriting from '" << plot.inherits_from << "' not found in file '" << file.path << "'" << std::endl;
      return false;
    }

    return true;
  }

  bool plotIt::loadObject(File& file, const Plot& plot) {

    file.object = nullptr;

    auto it = file.objects.find(plot.uid);

    if (it == file.objects.end()) {
      std::cout << "Error: object '" << plot.name << "' inheriting from '" << plot.inherits_from << "' not found in file '" << file.path << "'" << std::endl;
      return false;
    }

    file.object = it->second;

    file.systematics = & file.systematics_cache[plot.uid];

    return true;
  }

  bool plotIt::expandFiles() {
    std::vector<File> files;

    for (File& file: m_files) {
      std::vector<std::string> matchedFiles = glob(file.path);
      for (std::string& matchedFile: matchedFiles) {
        File f = file;
        f.path = matchedFile;
        //std::cout << file.path << " matches to " << f.path << std::endl;

        files.push_back(f);
      }
    }

    m_files = files;

    return true;
  }

  /**
   * Merge the labels of the global configuration and the current plot.
   * If some are duplicated, only keep the plot label
   **/
  std::vector<Label> plotIt::mergeLabels(const std::vector<Label>& plotLabels) {
    std::vector<Label> labels = plotLabels;

    // Add labels from global configuration, and check for duplicates
    for (auto& globalLabel: m_config.labels) {

      bool duplicated = false;
      for (auto& label: plotLabels) {
        if (globalLabel.text == label.text) {
          duplicated = true;
          break;
        }
      }

      if (! duplicated)
        labels.push_back(globalLabel);
    }

    return labels;
  }

  /**
   * Open 'file', and expand all plots
   */
  bool plotIt::expandObjects(File& file, std::vector<Plot>& plots) {
    file.object = nullptr;
    plots.clear();

    std::shared_ptr<TFile> input(TFile::Open(file.path.c_str()));
    if (! input.get())
      return false;

    TIter root_keys(input->GetListOfKeys());

    for (Plot& plot: m_plots) {
      TKey* key;
      TObject* obj;
      bool match = false;

      std::vector<std::string> tokens;
      boost::split(tokens, plot.name, boost::is_any_of("/"));

      bool in_directory = tokens.size() != 1;
      std::string plot_name = tokens.back();
      std::string root_name;

      root_keys.Reset();

      TIter it = root_keys;

      if (in_directory) {
          auto find_folder = [&](const std::string& name, TDirectory* root) -> TDirectory* {
              TIter it(root->GetListOfKeys());
              while ((key = static_cast<TKey*>(it()))) {
                  TObject* obj = key->ReadObj();
                  if (!obj->InheritsFrom("TDirectory"))
                      continue;

                  if (fnmatch(name.c_str(), obj->GetName(), FNM_CASEFOLD) == 0) {
                      return dynamic_cast<TDirectory*>(obj);
                  }
              }

              return nullptr;
          };

          std::string not_found;
          TDirectory* root = input.get();
          for (size_t i = 0; i < tokens.size() - 1; i++) {
              std::string folder = tokens[i];
              root = find_folder(folder, root);

              if (! root) {
                  not_found = folder;
                  break;
              }

              root_name += std::string(root->GetName()) + "/";
          }

          if (! root) {
              std::cout << "Warning: The folder '" << not_found << "' was not found in file '" << file.path << "'" << std::endl;
              continue;
          }

          it = TIter(root->GetListOfKeys());
      }

      std::vector<std::string> matched;
      while ((key = static_cast<TKey*>(it()))) {
        obj = key->ReadObj();
        if (! obj->InheritsFrom(plot.inherits_from.c_str()))
          continue;

        // Reject systematic variation plots
        std::string object_name = obj->GetName();
        if (object_name.find("__") != std::string::npos) {
            // TODO: Maybe we should be a bit less strict and check that the
            // systematics specified is included in the configuration file?
            continue;
        }

        // Check name
        if (fnmatch(plot_name.c_str(), object_name.c_str(), FNM_CASEFOLD) == 0) {

          // Check if this name is excluded
          if ((plot.exclude.length() > 0) && (fnmatch(plot.exclude.c_str(), object_name.c_str(), FNM_CASEFOLD) == 0)) {
            continue;
          }

          std::string expanded_plot_name = root_name + object_name;

          // The same object can be stored multiple time with a different key
          // The iterator returns first the object with the highest key, which is the most recent object
          // Check if we already have a plot with the same exact name
          if (std::find_if(matched.begin(), matched.end(), [&expanded_plot_name](const std::string& p) { return p == expanded_plot_name; }) != matched.end()) {
            continue;
          }

          // Got it!
          match = true;
          matched.push_back(expanded_plot_name);
          plots.push_back(plot.Clone(expanded_plot_name));
        }
      }

      if (! match) {
        std::cout << "Warning: object '" << plot.name << "' inheriting from '" << plot.inherits_from << "' does not match something in file '" << file.path << "'" << std::endl;
      }
    }

    if (!plots.size()) {
      std::cout << "Error: no plots found in file '" << file.path << "'" << std::endl;
      return false;
    }

    return true;
  }

  std::shared_ptr<PlotStyle> plotIt::getPlotStyle(const File& file) {
    if (file.legend_group.length() && m_legend_groups.count(file.legend_group)) {
      return m_legend_groups[file.legend_group].plot_style;
    } else {
      return file.plot_style;
    }
  }
}

int main(int argc, char** argv) {

  try {

    TCLAP::CmdLine cmd("Plot histograms", ' ', "0.1");

    TCLAP::ValueArg<std::string> outputFolderArg("o", "output-folder", "output folder", true, "", "string", cmd);

    TCLAP::SwitchArg ignoreScaleArg("", "ignore-scales", "Ignore any scales present in the configuration file", cmd, false);

    TCLAP::SwitchArg verboseArg("v", "verbose", "Verbose output (print summary)", cmd, false);

    TCLAP::SwitchArg yieldsArg("y", "yields", "Produce LaTeX table of yields", cmd, false);

    TCLAP::SwitchArg plotsArg("p", "plots", "Do not produce the plots - can be useful if only the yields table is needed", cmd, false);

    TCLAP::SwitchArg unblindArg("u", "unblind", "Unblind the plots, ie ignore any blinded-range in the configuration", cmd, false);

    TCLAP::UnlabeledValueArg<std::string> configFileArg("configFile", "configuration file", true, "", "string", cmd);

    cmd.parse(argc, argv);

    //bool isData = dataArg.isSet();

    fs::path outputPath(outputFolderArg.getValue());

    if (! fs::exists(outputPath)) {
      std::cout << "Error: output path " << outputPath << " does not exist" << std::endl;
      return 1;
    }

    if( plotsArg.getValue() && !yieldsArg.getValue() ) {
      std::cerr << "Error: we have nothing to do" << std::endl;
      return 1;
    }

    plotIt::plotIt p(outputPath, configFileArg.getValue());
    p.getConfigurationForEditing().ignore_scales = ignoreScaleArg.getValue();
    p.getConfigurationForEditing().verbose = verboseArg.getValue();
    p.getConfigurationForEditing().do_plots = !plotsArg.getValue();
    p.getConfigurationForEditing().do_yields = yieldsArg.getValue();
    p.getConfigurationForEditing().unblind = unblindArg.getValue();

    p.plotAll();

  } catch (TCLAP::ArgException &e) {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    return 1;
  }

  return 0;
}
