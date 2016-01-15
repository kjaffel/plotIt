#include <yaml-cpp/yaml.h>

#include <types.h>
#include <utilities.h>

namespace plotIt {
  void PlotStyle::loadFromYAML(YAML::Node& node, Type type) {
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
        drawing_options = "P";
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

    if (node["line-color"])
      line_color = loadColor(node["line-color"]);

    if (node["line-type"])
      line_type = node["line-type"].as<int16_t>();

    if (node["line-width"])
      line_width = node["line-width"].as<float>();

    if (node["marker-color"])
      marker_color = loadColor(node["marker-color"]);

    if (node["marker-type"])
      marker_type = node["marker-type"].as<int16_t>();

    if (node["marker-size"])
      marker_size = node["marker-size"].as<float>();
  }

  Line::Line(const YAML::Node& node, Orientation orientation) {
      auto NaN = std::numeric_limits<float>::quiet_NaN();

      if (orientation == UNSPECIFIED) {
          auto points = node.as<std::vector<Point>>();
          if (points.size() != 2)
              throw nullptr; // FIXME

          start = points[0];
          end = points[1];
      } else {
          float value = node.as<float>();
          if (orientation == HORIZONTAL) {
              start = {NaN, value};
              end = {NaN, value};
          } else {
              start = {value, NaN};
              end = {value, NaN};
          }
      }
  }
}
