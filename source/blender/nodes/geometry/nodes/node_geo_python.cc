/* SPDX-FileCopyrightText: 2023 Blender Foundation
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

// import node with: bpy.data.node_groups['Geometry Nodes'].nodes.new('GeometryNodePython')


#include <fstream>

#include "BLI_array.hh"
#include "BLI_kdtree.h"
#include "BLI_map.hh"
#include "BLI_task.hh"

#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"

#include "BKE_material.h"
#include "BKE_mesh.hh"

#include "node_geometry_util.hh"


namespace blender::nodes::node_geo_python_cc {

extern "C" void PyC_RunString(const char *python_string, std::vector<std::string> * multi_utils_vector, std::vector<std::string> * vector_input_strings_p, std::vector<int> * vector_input_ints_p, std::vector<float> * vector_input_floats_p, std::vector<int> * vector_input_bools_p);

//NODE_STORAGE_FUNCS(NodeGeometryPythonScript)

static void node_declare(NodeDeclarationBuilder &b)
{

  b.add_input<decl::String>("Python");
  b.add_input<decl::String>("Strings").multi_input().hide_value();
  b.add_input<decl::Int>("Integers").multi_input().hide_value();
  b.add_input<decl::Float>("Floats").multi_input().hide_value();
  b.add_input<decl::Bool>("Bools").multi_input().hide_value();
  b.add_output<decl::String>("String 1");
  b.add_output<decl::String>("String 2");
  b.add_output<decl::String>("String 3");
  b.add_output<decl::String>("String 4");
  b.add_output<decl::Int>("Integer 1");
  b.add_output<decl::Int>("Integer 2");
  b.add_output<decl::Int>("Integer 3");
  b.add_output<decl::Int>("Integer 4");
  b.add_output<decl::Float>("Float 1");
  b.add_output<decl::Float>("Float 2");
  b.add_output<decl::Float>("Float 3");
  b.add_output<decl::Float>("Float 4");
  b.add_output<decl::Bool>("Bool 1");
  b.add_output<decl::Bool>("Bool 2");
  b.add_output<decl::Bool>("Bool 3");
  b.add_output<decl::Bool>("Bool 4");

}


static void node_geo_exec(GeoNodeExecParams params)
{
  const std::string python_string = params.extract_input<std::string>("Python");

  const int buffer_size = 1024;

  Vector<SocketValueVariant> multi_input_strings = params.extract_input<Vector<SocketValueVariant>>("Strings");
  Vector<SocketValueVariant> multi_input_ints = params.extract_input<Vector<SocketValueVariant>>("Integers");
  Vector<SocketValueVariant> multi_input_floats = params.extract_input<Vector<SocketValueVariant>>("Floats");
  Vector<SocketValueVariant> multi_input_bools = params.extract_input<Vector<SocketValueVariant>>("Bools");

  std::vector<std::string> multi_string_vector;
  for (const int i : multi_input_strings.index_range()) {
    std::string input_string(buffer_size, ' ');
    input_string.replace(0, multi_input_strings[i].extract<std::string>().length(), multi_input_strings[i].extract<std::string>());
    multi_string_vector.push_back(input_string);
  }

  std::vector<int> multi_int_vector;
  for (const int i : multi_input_ints.index_range()) {
      multi_int_vector.push_back(multi_input_ints[i].extract<int>());
    }

  std::vector<float> multi_float_vector;
  for (const int i : multi_input_floats.index_range()) {
      multi_float_vector.push_back(multi_input_floats[i].extract<float>());
    }

  std::vector<int> multi_bool_vector;
  for (const int i : multi_input_bools.index_range()) {
      multi_bool_vector.push_back(multi_input_bools[i].extract<bool>());
    }

  std::vector<std::string> multi_utils_vector;
  multi_utils_vector.push_back(params.node().name);
  multi_utils_vector.push_back(params.node().owner_tree().idname);

  std::string string_format = std::to_string(buffer_size).append("s");

  if (python_string.length() > 0) {
    PyC_RunString(python_string.c_str(), &multi_utils_vector, &multi_string_vector, &multi_int_vector, &multi_float_vector, &multi_bool_vector);
  }
  else {
    fprintf(stderr, "%s: Missing input Python string!\n", params.node().label_or_name().c_str());
    params.error_message_add(NodeWarningType::Error,
                                TIP_("No input found!"));
  }

  // Set first 4 string outputs to the first 4 strings in the list. Also string trailing spaces
  for (int i = 0; i < 4; i++) {
    std::string identifier = std::string("String ").append(std::to_string(i + 1));
    if (i >= multi_string_vector.size()) { params.set_output(identifier, std::string()); }
    else { params.set_output(identifier, multi_string_vector[i].erase(multi_string_vector[i].find_last_not_of(' ') + 1)); }
  }

  // Set first 4 integer outputs to the first 4 integers in the list
  for (int i = 0; i < 4; i++) {
    std::string identifier = std::string("Integer ").append(std::to_string(i + 1));
    if (i >= multi_int_vector.size()) { params.set_output(identifier, 0); }
    else { params.set_output(identifier, multi_int_vector[i]); }
  }

  // Set first 4 float outputs to the first 4 floats in the list
  for (int i = 0; i < 4; i++) {
    std::string identifier = std::string("Float ").append(std::to_string(i + 1));
    if (i >= multi_float_vector.size()) { params.set_output(identifier, float(0)); }
    else { params.set_output(identifier, multi_float_vector[i]); }
  }

  // Set first 4 bool outputs to the first 4 bools in the list
  for (int i = 0; i < 4; i++) {
    std::string identifier = std::string("Bool ").append(std::to_string(i + 1));
    if (i >= multi_bool_vector.size()) { params.set_output(identifier, false); }
    else { params.set_output(identifier, multi_bool_vector[i]); }
  }
}

static void node_register()
{
  static bNodeType ntype;
  geo_node_type_base(&ntype, GEO_NODE_PYTHON, "Python", NODE_CLASS_GEOMETRY);
  ntype.declare = node_declare;
  ntype.geometry_node_execute = node_geo_exec;
  nodeRegisterType(&ntype);
}
NOD_REGISTER_NODE(node_register)

}  // namespace blender::nodes::node_geo_python_cc