/* SPDX-FileCopyrightText: 2023 Blender Foundation
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

// import node with: bpy.data.node_groups['Geometry Nodes'].nodes.new('GeometryNodePython')


#define PY_SSIZE_T_CLEAN

// Somebody will kill me for these include statements.. But it works!
#ifdef _WIN32
#    include "python/311/include/Python.h"
#    include "python/311/include/frameobject.h"
#endif
#ifdef __APPLE__
#    include "python/include/python3.11/Python.h"
#    include "python/include/python3.11/frameobject.h"
#endif

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include "python/numpy/core/include/numpy/arrayobject.h"

#include "BLI_utildefines.h" /* for bool */

#include "../../source/blender/python/generic/python_utildefines.hh"
#include "../../source/blender/python/generic/py_capi_utils.hh"

#ifndef MATH_STANDALONE
#  include "MEM_guardedalloc.h"

#  include "BLI_string.h"

/* Only for #BLI_strncpy_wchar_from_utf8,
 * should replace with Python functions but too late in release now. */
#  include "BLI_string_utf8.h"
#endif

#ifdef _WIN32
    #include "BLI_math_base.h" /* isfinite() */
#endif

#include <fstream>
#include <map>
#include <array>

#include "BLI_array.hh"
#include "BLI_kdtree.h"
#include "BLI_map.hh"
#include "BLI_task.hh"
 
#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"

#include "BKE_material.h"
#include "BKE_mesh.hh"
#include "BKE_attribute.hh"
#include "BKE_attribute_math.hh"

#include "GEO_mesh_primitive_cuboid.hh"
#include "GEO_mesh_primitive_grid.hh"
#include "GEO_mesh_primitive_line.hh"

#include "node_geometry_util.hh"


namespace blender::nodes::node_geo_python_cc {


bool is_internal_attribute(std::string name) {
  if (name == ".edge_verts"  || name == ".corner_vert" ||
      name == ".corner_edge" || name == ".hide_vert"   ||
      name == ".hide_edge"   || name == ".hide_poly"   ||
      name == ".uv_seam"     || name == ".select_vert" ||
      name == ".select_edge" || name == ".select_poly") {
    return true;
  } else { return false; }
}


int get_numpy() {
  import_array();

  if (PyErr_Occurred()) {
      std::cerr << "Failed to import numpy Python module(s)." << std::endl;
  }
  assert(PyArray_API);
  return 0;
}

void PyC_RunString(const char *python_string, const int string_buf_size,
 char * util_strings, const int util_strings_size,
 char * input_strings, const int input_strings_size,
 int * input_integers, const int input_integers_size,
 float * input_floats, const int input_floats_size,
 bool * input_bools, const int input_bools_size,
 std::vector<std::vector<void *>> geoset_pointer_vecs,
 std::vector<std::vector<int>> geoset_size_vecs,
 std::vector<std::vector<int>> geoset_type_vecs,
 std::vector<std::vector<std::string>> geoset_name_vecs,
 std::vector<std::vector<AttrDomain>> geoset_domain_vecs
 ) {

  const PyGILState_STATE gilstate = PyGILState_Ensure();

  get_numpy();

  PyObject *py_dict = PyC_DefaultNameSpace(&util_strings[0]);
  PyObject *py_dict_node = PyDict_New();

  npy_intp dims[1] = {input_integers_size};
  PyObject * py_input_integers = PyArray_SimpleNewFromData(1, dims, NPY_INT, (void *)input_integers);
  dims[0] = {input_floats_size};
  PyObject * py_input_floats = PyArray_SimpleNewFromData(1, dims, NPY_FLOAT, (void *)input_floats);
  dims[0] = {input_bools_size};
  PyObject * py_input_bools = PyArray_SimpleNewFromData(1, dims, NPY_BOOL, (void *)input_bools);
  dims[0] = {input_strings_size};
  PyObject * py_input_strings = PyArray_New(&PyArray_Type, 1, dims, NPY_STRING, NULL, (void *)input_strings, string_buf_size, NPY_ARRAY_WRITEABLE, NULL);
  dims[0] = {util_strings_size};
  PyObject * py_input_utils = PyArray_New(&PyArray_Type, 1, dims, NPY_STRING, NULL, (void *)util_strings, string_buf_size, NULL, NULL);


  PyObject * py_geometry = PyList_New(geoset_pointer_vecs.size());

  for (int index = 0; index < geoset_pointer_vecs.size(); index++) {
    std::vector<void *> attribute_pointers = geoset_pointer_vecs[index];
    std::vector<int> attribute_sizes = geoset_size_vecs[index];
    std::vector<int> attribute_types = geoset_type_vecs[index];
    std::vector<std::string> attribute_names = geoset_name_vecs[index];
    std::vector<AttrDomain> attribute_domains = geoset_domain_vecs[index];
  
    const int attr_vector_size = attribute_pointers.size();
    PyObject * py_attributes = PyDict_New();
    
    for (int i = 0; i < attr_vector_size; i++) {
      PyObject * py_value = nullptr;
      switch (attribute_types[i]) {
        case (1): { 
          npy_intp attr_dims[1] = {attribute_sizes[i]};
          py_value = PyArray_SimpleNewFromData(1, attr_dims, NPY_FLOAT, attribute_pointers[i]);
          break;
        }
         case (2): {
          npy_intp attr_dims[2] = {attribute_sizes[i] / 2, 2};
          py_value = PyArray_SimpleNewFromData(2, attr_dims, NPY_FLOAT, attribute_pointers[i]);
          break;
        } 
        case (3): {
          npy_intp attr_dims[2] = {attribute_sizes[i] / 3, 3};
          py_value = PyArray_SimpleNewFromData(2, attr_dims, NPY_FLOAT, attribute_pointers[i]);
          break;
        }
        case (4):
        case (5): {
          npy_intp attr_dims[2] = {attribute_sizes[i] / 4, 4};
          py_value = PyArray_SimpleNewFromData(2, attr_dims, NPY_FLOAT, attribute_pointers[i]);
          break;
        } 
        case (6): {
          npy_intp attr_dims[3] = {attribute_sizes[i] / 16, 4, 4};
          py_value = PyArray_SimpleNewFromData(3, attr_dims, NPY_FLOAT, attribute_pointers[i]);
          break;
        } 
        case (7): {
          npy_intp attr_dims[1] = {attribute_sizes[i]};
          py_value = PyArray_SimpleNewFromData(1, attr_dims, NPY_INT, attribute_pointers[i]);
          break;
        } 
        case (8): {
          npy_intp attr_dims[2] = {attribute_sizes[i] / 2, 2};
          py_value = PyArray_SimpleNewFromData(2, attr_dims, NPY_INT, attribute_pointers[i]);
          break;
        } 
        case (9): {
          npy_intp attr_dims[1] = {attribute_sizes[i]};
          py_value = PyArray_SimpleNewFromData(1, attr_dims, NPY_BYTE, attribute_pointers[i]);
          break;
        } 
        case (10): { 
          npy_intp attr_dims[1] = {attribute_sizes[i]};
          py_value = PyArray_SimpleNewFromData(1, attr_dims, NPY_BYTE, attribute_pointers[i]);
          break;
        } 
        // Strings per vertex and such read random memeory and cause crashes. Maybe re-enable once Blender devs fix it.
        /*
        case (11): { 
          npy_intp attr_dims[1] = {attribute_sizes[i]};
          py_value = PyArray_New(&PyArray_Type, 1, attr_dims, NPY_STRING, NULL, attribute_pointers[i], string_buf_size, NPY_ARRAY_WRITEABLE, NULL);
          break;
        } 
        */
        default: {
          break;
        }
      }
      if (py_value != nullptr) {
          PyDict_SetItemString(py_attributes, attribute_names[i].c_str(), py_value);
        Py_DECREF(py_value);
      }
    }
    PyList_SET_ITEM(py_geometry, index, py_attributes);
  }

  PyObject *py_result;

  //PyC_ObSpit("Geometry", py_geometry);
  PyDict_SetItemString(py_dict_node, "geometry", py_geometry);
  Py_DECREF(py_geometry);
  PyDict_SetItemString(py_dict_node, "integers", py_input_integers);
  Py_DECREF(py_input_integers);
  PyDict_SetItemString(py_dict_node, "floats", py_input_floats);
  Py_DECREF(py_input_floats);
  PyDict_SetItemString(py_dict_node, "bools", py_input_bools);
  Py_DECREF(py_input_bools);
  PyDict_SetItemString(py_dict_node, "strings", py_input_strings);
  Py_DECREF(py_input_strings);
  PyDict_SetItemString(py_dict_node, "utils", py_input_utils);
  Py_DECREF(py_input_utils);
  PyDict_SetItemString(py_dict, "node", py_dict_node);
  Py_DECREF(py_dict_node);

  // RUN
  py_result = PyRun_String(python_string, Py_file_input, py_dict, py_dict);

  if (py_result) {
    Py_DECREF(py_result);
    py_result = nullptr;

  }
  else {
    //printf("%s error line:%d\n", __func__, __LINE__);
    PyErr_Print();
    PyErr_Clear();
  }

  PyGILState_Release(gilstate);
}



//NODE_STORAGE_FUNCS(NodeGeometryPythonScript)

static void node_declare(NodeDeclarationBuilder &b)
{

  b.add_input<decl::String>("Python");
  b.add_input<decl::Geometry>("Geometry").multi_input();
  b.add_input<decl::String>("Strings").multi_input().hide_value();
  b.add_input<decl::Int>("Integers").multi_input().hide_value();
  b.add_input<decl::Float>("Floats").multi_input().hide_value();
  b.add_input<decl::Bool>("Bools").multi_input().hide_value();

  b.add_output<decl::Geometry>("Geometry 1");
  b.add_output<decl::Geometry>("Geometry 2");
  b.add_output<decl::Geometry>("Geometry 3");
  b.add_output<decl::Geometry>("Geometry 4");
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

  // Use a fixed buffer for the string size because of we will memcpy the new values back in later. 
  // This will cut of larger strings but prevent buffer overflows.
  const int buffer_size = 1024;

  Vector<GeometrySet> geometry_sets = params.extract_input<Vector<GeometrySet>>("Geometry");
  Vector<SocketValueVariant> multi_input_strings = params.extract_input<Vector<SocketValueVariant>>("Strings");
  Vector<SocketValueVariant> multi_input_ints = params.extract_input<Vector<SocketValueVariant>>("Integers");
  Vector<SocketValueVariant> multi_input_floats = params.extract_input<Vector<SocketValueVariant>>("Floats");
  Vector<SocketValueVariant> multi_input_bools = params.extract_input<Vector<SocketValueVariant>>("Bools");

  std::vector<std::string> multi_string_vector;

  char * input_strings = new char[buffer_size * multi_input_strings.size()]();
  for (const int i : multi_input_strings.index_range()) {
    std::string socket_input_string = multi_input_strings[i].extract<std::string>();
    strncpy(&(input_strings[i * buffer_size]), socket_input_string.data(), buffer_size);
  }


  const int util_strings_size = 3;
  char * util_strings = new char[buffer_size * util_strings_size]();
  strncpy(&(util_strings[0]), params.self_object()->id.name, buffer_size);
  strncpy(&(util_strings[1 * buffer_size]), params.node().name, buffer_size);
  strncpy(&(util_strings[2 * buffer_size]), params.node().owner_tree().idname, buffer_size);
  

  int * input_integers = new int[multi_input_ints.size()]();
  for (const int i : multi_input_ints.index_range()) {
    const int input_int = multi_input_ints[i].extract<int>();
    memcpy(&(input_integers[i]), &input_int, sizeof(int));
  }

  float * input_floats = new float[multi_input_floats.size()]();
  for (const int i : multi_input_floats.index_range()) {
    const float input_float = multi_input_floats[i].extract<float>();
    memcpy(&(input_floats[i]), &input_float, sizeof(float));
  }

  bool * input_bools = new bool[multi_input_bools.size()]();
  for (const int i : multi_input_bools.index_range()) {
    const bool input_bool = multi_input_bools[i].extract<bool>();
    memcpy(&(input_bools[i]), &input_bool, sizeof(bool));
  }

  std::vector<std::vector<void *>> geoset_pointer_vecs;
  std::vector<std::vector<int>> geoset_size_vecs;
  std::vector<std::vector<std::string>> geoset_name_vecs;
  std::vector<std::vector<int>> geoset_type_vecs;
  std::vector<std::vector<AttrDomain>> geoset_domain_vecs;

  for (int index = 0; index < geometry_sets.size(); index++) {
    std::vector<void *> attribute_pointers_vector;
    std::vector<int> attribute_sizes;
    std::vector<std::string> attribute_names;
    std::vector<int> attribute_types;
    std::vector<AttrDomain> attribute_domains;

    GeometrySet geometry = geometry_sets[index];
    // Read mesh attributes
    if (geometry.has_instances()) { continue; }

    std::optional<AttributeAccessor> attributes;

    if (geometry.has_mesh()) {
      attributes = geometry.get_component(GeometryComponent::Type::Mesh)->attributes();
    } else if (geometry.has_curves()) {
      attributes = geometry.get_component(GeometryComponent::Type::Curve)->attributes();
    } else if (geometry.has_pointcloud()) {
      attributes = geometry.get_component(GeometryComponent::Type::PointCloud)->attributes();
    } else if (geometry.has_volume()) {
      attributes = geometry.get_component(GeometryComponent::Type::Volume)->attributes();
    } else if (geometry.has_grease_pencil()) {
      attributes = geometry.get_component(GeometryComponent::Type::GreasePencil)->attributes();
    } else { continue; }
    
    if (!attributes) { continue; }

    if (attributes) {
        Set<StringRefNull> attribute_ids =  (*attributes).all_ids();
        for (StringRefNull attribute_id : attribute_ids) {
          GAttributeReader attribute_id_read = attributes->lookup(attribute_id);
          const CPPType &type = attribute_id_read.varray.type();
          
          if (type.is<float>()) {
            const int size = attribute_id_read.varray.size();
            float * attribute_floats = new float[size]();  
            for (int i = 0; i < attribute_id_read.varray.size(); i++) {
              attribute_floats[i] = attribute_id_read.varray.get<float>(i);
            }
            attribute_pointers_vector.push_back(attribute_floats);
            attribute_types.push_back(1);
            attribute_sizes.push_back(size);
          }
          else if (type.is<float2>()) {
            const int size = 2 * attribute_id_read.varray.size();
            float * attribute_float2s = new float[size]();  
            for (int i = 0; i < attribute_id_read.varray.size(); i++) {
              float2 value = attribute_id_read.varray.get<float2>(i);
              attribute_float2s[i * 2] = value.x;
              attribute_float2s[i * 2 + 1] = value.y;
            }
            attribute_pointers_vector.push_back(attribute_float2s);
            attribute_types.push_back(2);
            attribute_sizes.push_back(size);
          }
          else if (type.is<float3>()) {
            const int size = 3 * attribute_id_read.varray.size();
            float * attribute_float3s = new float[size]();  
            for (int i = 0; i < attribute_id_read.varray.size(); i++) {
              float3 value = attribute_id_read.varray.get<float3>(i);
              attribute_float3s[i * 3] = value.x;
              attribute_float3s[i * 3 + 1] = value.y;
              attribute_float3s[i * 3 + 2] = value.z;
            }
            attribute_pointers_vector.push_back(attribute_float3s);
            attribute_types.push_back(3);
            attribute_sizes.push_back(size);
          }
          else if (type.is<ColorGeometry4f>()) {
            const int size = 4 * attribute_id_read.varray.size();
            float * attribute_CG4fs = new float[size]();  
            for (int i = 0; i < attribute_id_read.varray.size(); i++) {
              ColorGeometry4f value = attribute_id_read.varray.get<ColorGeometry4f>(i);
              attribute_CG4fs[i * 4] = value.r;
              attribute_CG4fs[i * 4 + 1] = value.g;
              attribute_CG4fs[i * 4 + 2] = value.b;
              attribute_CG4fs[i * 4 + 3] = value.a;
            }
            attribute_pointers_vector.push_back(attribute_CG4fs);
            attribute_types.push_back(4);
            attribute_sizes.push_back(size);
          }
          else if (type.is<math::Quaternion>()) {
            const int size = 4 * attribute_id_read.varray.size();
            float * attribute_Quats = new float[size]();  
            for (int i = 0; i < attribute_id_read.varray.size(); i++) {
              math::Quaternion value = attribute_id_read.varray.get<math::Quaternion>(i);
              attribute_Quats[i * 4] = value.w;
              attribute_Quats[i * 4 + 1] = value.x;
              attribute_Quats[i * 4 + 2] = value.y;
              attribute_Quats[i * 4 + 3] = value.z;
            }
            attribute_pointers_vector.push_back(attribute_Quats);
            attribute_types.push_back(5);
            attribute_sizes.push_back(size);
          }
          else if (type.is<float4x4>()) {
            const int size = 16 * attribute_id_read.varray.size();
            float * attribute_f4x4s = new float[size]();  
            for (int i = 0; i < attribute_id_read.varray.size(); i++) {
              float4x4 value = attribute_id_read.varray.get<float4x4>(i);
              attribute_f4x4s[i * 16 + 0] = value[0][0];
              attribute_f4x4s[i * 16 + 1] = value[0][1];
              attribute_f4x4s[i * 16 + 2] = value[0][2];
              attribute_f4x4s[i * 16 + 3] = value[0][3];
              
              attribute_f4x4s[i * 16 + 4] = value[1][0];
              attribute_f4x4s[i * 16 + 5] = value[1][1];
              attribute_f4x4s[i * 16 + 6] = value[1][2];
              attribute_f4x4s[i * 16 + 7] = value[1][3];

              attribute_f4x4s[i * 16 + 8] = value[2][0];
              attribute_f4x4s[i * 16 + 9] = value[2][1];
              attribute_f4x4s[i * 16 + 10] = value[2][2];
              attribute_f4x4s[i * 16 + 11] = value[2][3];

              attribute_f4x4s[i * 16 + 12] = value[3][0];
              attribute_f4x4s[i * 16 + 13] = value[3][1];
              attribute_f4x4s[i * 16 + 14] = value[3][2];
              attribute_f4x4s[i * 16 + 15] = value[3][3];
            }
            attribute_pointers_vector.push_back(attribute_f4x4s);
            attribute_types.push_back(6);
            attribute_sizes.push_back(size);
          }
          else if (type.is<int>()) {
            const int size = attribute_id_read.varray.size();
            int * attribute_ints = new int[size]();  
            for (int i = 0; i < attribute_id_read.varray.size(); i++) {
              attribute_ints[i] = attribute_id_read.varray.get<int>(i);
            }
            attribute_pointers_vector.push_back(attribute_ints);
            attribute_types.push_back(7);
            attribute_sizes.push_back(size);
          }
          else if (type.is<int2>()) {
            const int size = 2 * attribute_id_read.varray.size();
            int * attribute_int2s = new int[size]();  
            for (int i = 0; i < attribute_id_read.varray.size(); i++) {
              int2 value = attribute_id_read.varray.get<int2>(i);
              attribute_int2s[i * 2] = value.x;
              attribute_int2s[i * 2 + 1] = value.y;
            }
            attribute_pointers_vector.push_back(attribute_int2s);
            attribute_types.push_back(8);
            attribute_sizes.push_back(size);
          }
          else if (type.is<int8_t>()) {
            const int size = attribute_id_read.varray.size();
            int8_t * attribute_int8_ts = new int8_t[size]();  
            for (int i = 0; i < attribute_id_read.varray.size(); i++) {
              attribute_int8_ts[i] = attribute_id_read.varray.get<int8_t>(i);
            }
            attribute_pointers_vector.push_back(attribute_int8_ts);
            attribute_types.push_back(9);
            attribute_sizes.push_back(size);
          }
          else if (type.is<bool>()) {
            const int size = attribute_id_read.varray.size();
            bool * attribute_bools = new bool[size]();  
            for (int i = 0; i < attribute_id_read.varray.size(); i++) {
              attribute_bools[i] = attribute_id_read.varray.get<bool>(i);
            }
            attribute_pointers_vector.push_back(attribute_bools);
            attribute_types.push_back(10);
            attribute_sizes.push_back(size);
          }
          // Disable string reading of attributes for now because of crashes and instability. Blender needs to implement it first.
          /*
          else if (type.is<MStringProperty>()) {
            const int size = buffer_size * attribute_id_read.varray.size();
            char * attribute_strings = new char[size]();  
            for (int i = 0; i < attribute_id_read.varray.size(); i++) {
              strncpy(&(attribute_strings[i * buffer_size]), attribute_id_read.varray.get<MStringProperty>(i).s, buffer_size);
            }
            attribute_pointers_vector.push_back(attribute_strings);
            attribute_types.push_back(11);
            attribute_sizes.push_back(size);
          }
          */
          else { printf("Error: Unknown type found in geometry attributes!\n Fix this!"); continue; }
          
          attribute_names.push_back(attribute_id);
          attribute_domains.push_back(attribute_id_read.domain);
        }
        geoset_pointer_vecs.push_back(attribute_pointers_vector);
        geoset_size_vecs.push_back(attribute_sizes);
        geoset_name_vecs.push_back(attribute_names);
        geoset_type_vecs.push_back(attribute_types);
        geoset_domain_vecs.push_back(attribute_domains);
      }
  }

  if (python_string.length() > 0) {
    PyC_RunString(python_string.c_str(), buffer_size,
    util_strings, util_strings_size,
    input_strings, buffer_size * multi_input_strings.size(),
    input_integers, multi_input_ints.size(),
    input_floats, multi_input_floats.size(),
    input_bools, multi_input_bools.size(),
    geoset_pointer_vecs, geoset_size_vecs,
    geoset_type_vecs, geoset_name_vecs,
    geoset_domain_vecs);
  }
  else {
    fprintf(stderr, "%s: Missing input Python string!\n", params.node().label_or_name().c_str());
    params.error_message_add(NodeWarningType::Error,
                                TIP_("No input found!"));
  }



// Writing attributes costs more time than I am willing to spend on this right now
// This code doesn't work

  for (int index = 0; index < geoset_pointer_vecs.size(); index++) {

    GeometrySet input_geo = geometry_sets[index];

    // Prevent instance crash by ignoring them
    if (input_geo.has_instances()) { continue;}

    std::vector<void *> modified_geo_attributes = geoset_pointer_vecs[index];
    std::vector<int> modified_geo_attribute_sizes = geoset_size_vecs[index];
    std::vector<std::string> modified_geo_attribute_names = geoset_name_vecs[index];
    std::vector<int> modified_geo_attribute_types = geoset_type_vecs[index];
    std::vector<AttrDomain> modified_geo_attribute_domains = geoset_domain_vecs[index];

    std::optional<MutableAttributeAccessor> attributes;
    
    if (input_geo.has_mesh()) {
      attributes = input_geo.get_component_for_write(GeometryComponent::Type::Mesh).attributes_for_write();
    } else if (input_geo.has_curves()) {
      attributes = input_geo.get_component_for_write(GeometryComponent::Type::Curve).attributes_for_write();
    } else if (input_geo.has_pointcloud()) {
      attributes = input_geo.get_component_for_write(GeometryComponent::Type::PointCloud).attributes_for_write();
    } else if (input_geo.has_volume()) {
      attributes = input_geo.get_component_for_write(GeometryComponent::Type::Volume).attributes_for_write();
    } else if (input_geo.has_grease_pencil()) {
      attributes = input_geo.get_component_for_write(GeometryComponent::Type::GreasePencil).attributes_for_write();
    } else { continue; }
    
    if (!attributes) { continue; }
    
    Set<StringRefNull> attribute_ids = attributes->all_ids();
    int attr_index = -1;
    for (StringRefNull attribute_id : attribute_ids) {
      attr_index += 1;
      if (is_internal_attribute(modified_geo_attribute_names[attr_index])) { continue; }
      
      GAttributeWriter writer = attributes->lookup_for_write(attribute_id);
      const CPPType &type = writer.varray.type();
      if (type.is<float>()) {
        for (int varr_index = 0; varr_index < writer.varray.size(); varr_index++) {
          float * modified_array = ((float *)modified_geo_attributes[attr_index]);
          float value = modified_array[varr_index];
          writer.varray.set_by_copy(varr_index, &value);
        }
      } else if (type.is<float2>()) {
        for (int varr_index = 0; varr_index < writer.varray.size(); varr_index++) {
          float * modified_array = ((float *)modified_geo_attributes[attr_index]);
          float2 value;
          value.x = modified_array[varr_index * 2 + 0];
          value.y = modified_array[varr_index * 2 + 1];
          writer.varray.set_by_copy(varr_index, value);
        }
      } else if (type.is<float3>()) {
        for (int varr_index = 0; varr_index < writer.varray.size(); varr_index++) {
          float * modified_array = ((float *)modified_geo_attributes[attr_index]);
          float3 value;
          value.x = modified_array[varr_index * 3 + 0];
          value.y = modified_array[varr_index * 3 + 1];
          value.z = modified_array[varr_index * 3 + 2];
          writer.varray.set_by_copy(varr_index, value);
        }
      } else if (type.is<ColorGeometry4f>()) {
        for (int varr_index = 0; varr_index < writer.varray.size(); varr_index++) {
          float * modified_array = ((float *)modified_geo_attributes[attr_index]);
          ColorGeometry4f value;
          value.r = modified_array[varr_index * 4 + 0];
          value.g = modified_array[varr_index * 4 + 1];
          value.b = modified_array[varr_index * 4 + 2];
          value.a = modified_array[varr_index * 4 + 3];
          writer.varray.set_by_copy(varr_index, value);
        }
      } else if (type.is<math::Quaternion>()) {
        for (int varr_index = 0; varr_index < writer.varray.size(); varr_index++) {
          float * modified_array = ((float *)modified_geo_attributes[attr_index]);
          math::Quaternion value;
          value.w = modified_array[varr_index * 4 + 0];
          value.x = modified_array[varr_index * 4 + 1];
          value.y = modified_array[varr_index * 4 + 2];
          value.z = modified_array[varr_index * 4 + 3];
          writer.varray.set_by_copy(varr_index, &value);
        }
      } else if (type.is<float4x4>()) {
        for (int varr_index = 0; varr_index < writer.varray.size(); varr_index++) {
          float * modified_array = ((float *)modified_geo_attributes[attr_index]);
          float4x4 value;
          value[0][0] = modified_array[varr_index * 16 + 0];
          value[0][1] = modified_array[varr_index * 16 + 1];
          value[0][2] = modified_array[varr_index * 16 + 2];
          value[0][3] = modified_array[varr_index * 16 + 3];

          value[1][0] = modified_array[varr_index * 16 + 4];
          value[1][1] = modified_array[varr_index * 16 + 5];
          value[1][2] = modified_array[varr_index * 16 + 6];
          value[1][3] = modified_array[varr_index * 16 + 7];

          value[2][0] = modified_array[varr_index * 16 + 8];
          value[2][1] = modified_array[varr_index * 16 + 9];
          value[2][2] = modified_array[varr_index * 16 + 10];
          value[2][3] = modified_array[varr_index * 16 + 11];

          value[3][0] = modified_array[varr_index * 16 + 12];
          value[3][1] = modified_array[varr_index * 16 + 13];
          value[3][2] = modified_array[varr_index * 16 + 14];
          value[3][3] = modified_array[varr_index * 16 + 15];
          writer.varray.set_by_copy(varr_index, &value);
        }
      } else if (type.is<int>()) {
        for (int varr_index = 0; varr_index < writer.varray.size(); varr_index++) {
          int * modified_array = ((int *)modified_geo_attributes[attr_index]);
          int value = modified_array[varr_index];
          writer.varray.set_by_copy(varr_index, &value);
        }
      } else if (type.is<int2>()) {
        for (int varr_index = 0; varr_index < writer.varray.size(); varr_index++) {
          int * modified_array = ((int *)modified_geo_attributes[attr_index]);
          int2 value;
          value.x = modified_array[varr_index * 2 + 0];
          value.y = modified_array[varr_index * 2 + 1];
          writer.varray.set_by_copy(varr_index, value);
        }
      } else if (type.is<int8_t>()) {
        for (int varr_index = 0; varr_index < writer.varray.size(); varr_index++) {
          int8_t * modified_array = ((int8_t *)modified_geo_attributes[attr_index]);
          int8_t value = modified_array[varr_index];
          writer.varray.set_by_copy(varr_index, &value);
        }
      } else if (type.is<bool>()) {
        for (int varr_index = 0; varr_index < writer.varray.size(); varr_index++) {
          bool * modified_array = ((bool *)modified_geo_attributes[attr_index]);
          bool value = modified_array[varr_index];
          writer.varray.set_by_copy(varr_index, &value);
        }
      } else {
        continue;
      }
      writer.finish();
    }
    geometry_sets[index] = input_geo;
  }


  Mesh *mesh = geometry::create_cuboid_mesh(float3{2.0f, 2.0f, 2.0f}, 2, 2, 2);
  BKE_id_material_eval_ensure_default_slot(&mesh->id);
  GeometrySet cube = GeometrySet::from_mesh(mesh);
  
  for (int i = 0; i < 4; i++) {
    std::string identifier = std::string("Geometry ").append(std::to_string(i + 1));
    if (i >= geometry_sets.size()) { params.set_output(identifier, cube); }
    else {
      params.set_output(identifier, geometry_sets[i]);
    }
  }

  // Set first 4 string outputs to the first 4 strings in the list.
  for (int i = 0; i < 4; i++) {
    std::string identifier = std::string("String ").append(std::to_string(i + 1));
    if (i >= multi_input_strings.size()) { params.set_output(identifier, std::string()); }
    else { 
      // Prevent Unicode on the end of the string from messing stuff up by supplying a 4 null byte slide where 4 byte unicode characters can land in
      // Very rarely string output with unicode on the end with string to curves gives extra characters (OOB mem read?) Can't be bothered to fix the edge case
      memset(&(input_strings[(i * buffer_size) + buffer_size - 4]), '\x00', 4);
      std::string new_str = std::string(&(input_strings[i * buffer_size]));
      params.set_output(identifier, new_str);
    }
  }

  for (int i = 0; i < 4; i++) {
    std::string identifier = std::string("Integer ").append(std::to_string(i + 1));
    if (i >= multi_input_ints.size()) { params.set_output(identifier, 0); }
    else {
      params.set_output(identifier, input_integers[i]);
    }
  }

  for (int i = 0; i < 4; i++) {
    std::string identifier = std::string("Float ").append(std::to_string(i + 1));
    if (i >= multi_input_floats.size()) { params.set_output(identifier, 0.0f); }
    else {
      params.set_output(identifier, input_floats[i]);
    }
  }

  for (int i = 0; i < 4; i++) {
    std::string identifier = std::string("Bool ").append(std::to_string(i + 1));
    if (i >= multi_input_bools.size()) { params.set_output(identifier, false); }
    else {
      params.set_output(identifier, input_bools[i]);
    }
  }




  delete[] util_strings;
  delete[] input_strings;
  delete[] input_integers;
  delete[] input_floats;
  delete[] input_bools;


for (int index = 0; index < geoset_pointer_vecs.size(); index++) {
  for (int i = 0; i < geoset_pointer_vecs[index].size(); i++) {
    void * array = geoset_pointer_vecs[index][i];
    const int type = geoset_type_vecs[index][i];
    if (type <= 6) { delete[] (float*) array;}
    else if (type <= 8) { delete[] (int*) array;} 
    else if (type == 9) { delete[] (int8_t*) array;} 
    else if (type == 10) { delete[] (bool*) array;} 
    else if (type == 11) { delete[] (char*) array;} 
    }
  }
}


static void node_register()
{
  static blender::bke::bNodeType ntype;
  geo_node_type_base(&ntype, GEO_NODE_PYTHON, "Python", NODE_CLASS_GEOMETRY);
  ntype.declare = node_declare;
  ntype.geometry_node_execute = node_geo_exec;
  blender::bke::node_register_type(&ntype);

}
NOD_REGISTER_NODE(node_register)

}  // namespace blender::nodes::node_geo_python_cc