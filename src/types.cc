#include <yaml-cpp/yaml.h>

#include <types.h>
#include <utilities.h>

#include <TLegendEntry.h>

namespace plotIt {
  void PlotStyle::loadFromYAML(const YAML::Node& node, Type type) {
    if (node["legend"])
      legend = node["legend"].as<std::string>();

    if (type == MC)
      legend_style = "lf";
    else if (type == SIGNAL)
      legend_style = "l";
    else if (type == DATA)
      legend_style = "pe";

    if (node["legend-style"])
      legend_style = node["legend-style"].as<std::string>();

    if (node["legend-order"])
      legend_order = node["legend-order"].as<int16_t>();

    if (node["drawing-options"])
      drawing_options = node["drawing-options"].as<std::string>();
    else {
      if (type == MC || type == SIGNAL)
        drawing_options = "hist";
      else if (type == DATA)
        drawing_options = "P E X0";
    }

    marker_size = -1;
    marker_color = -1;
    marker_type = -1;

    fill_color = -1;
    fill_type = -1;

    line_color = -1;
    line_type = -1;

    if (type == MC) {
      fill_color = 1;
      fill_type = 1001;
      line_width = 0;
    } else if (type == SIGNAL) {
      fill_type = 0;
      line_color = 1;
      line_width = 1;
      line_type = 2;
    } else {
      marker_size = 1;
      marker_color = 1;
      marker_type = 20;
      line_color = 1;
      line_width = 1; // For uncertainties
    }

    if (node["fill-color"])
      fill_color = loadColor(node["fill-color"]);

    if (node["fill-type"])
      fill_type = node["fill-type"].as<int16_t>();

    LineStyle::parse(node);

    if (node["marker-color"])
      marker_color = loadColor(node["marker-color"]);

    if (node["marker-type"])
      marker_type = node["marker-type"].as<int16_t>();

    if (node["marker-size"])
      marker_size = node["marker-size"].as<float>();
  }

  LineStyle::LineStyle(const YAML::Node& node) {
    parse(node);
  }

  void LineStyle::parse(const YAML::Node& node) {
    if (node["line-color"])
      line_color = loadColor(node["line-color"]);

    if (node["line-type"])
      line_type = node["line-type"].as<int16_t>();

    if (node["line-width"])
      line_width = node["line-width"].as<float>();
  }

  Line::Line(const YAML::Node& node, Orientation orientation) {
      auto NaN = std::numeric_limits<float>::quiet_NaN();

      YAML::Node configuration = node;
      if (node.Type() == YAML::NodeType::Map) {
          style = LineStyle(node);
          if (node["pad-location"]) {
            std::string l = node["pad-location"].as<std::string>();
            if (l == "bottom")
              pad = BOTTOM;
          }

          configuration = node["value"];
      }

      if (orientation == UNSPECIFIED) {
          auto points = configuration.as<std::vector<Point>>();
          if (points.size() != 2)
              throw YAML::ParserException(YAML::Mark::null_mark(), "A line is defined by exactly two points");

          start = points[0];
          end = points[1];
      } else {
          float value = configuration.as<float>();
          if (orientation == HORIZONTAL) {
              start = {NaN, value};
              end = {NaN, value};
          } else {
              start = {value, NaN};
              end = {value, NaN};
          }
      }
  }

  LegendEntry::LegendEntry(TObject* object, const std::string& legend, const std::string& style, int16_t order):
      object(object), legend(legend), style(style), order(order) {
          // Empty
      }

  LegendEntry::LegendEntry(const std::string& legend, const std::string& style, int16_t fill_style, int16_t fill_color, uint16_t line_width):
      object(nullptr), legend(legend), style(style), order(0), fill_style(fill_style), fill_color(fill_color), line_width(line_width) {
          // Empty
      }

  void LegendEntry::stylize(TLegendEntry* entry) {
      if (object)
          return;

      entry->SetLineWidth(line_width);
      entry->SetLineColor(fill_color);
      entry->SetFillStyle(fill_style);
      entry->SetFillColor(fill_color);
  }
}
