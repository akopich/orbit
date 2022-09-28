#include <absl/algorithm/container.h>
#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>

#include "../../third_party/json/json.hpp"
#include "OrbitBase/Logging.h"

using ::nlohmann::json;
namespace fs = ::std::filesystem;

constexpr std::string_view kTargetArchitecture = "X64";

struct Identifier {
  std::string name;
  std::string api;

  [[nodiscard]] friend bool operator==(const Identifier& lhs, const Identifier& rhs) {
    return lhs.name == rhs.name && lhs.api == rhs.api;
  }

  template <typename H>
  friend H AbslHashValue(H h, const Identifier& identifier) {
    return H::combine(std::move(h), identifier.api, identifier.name);
  }
};

struct Type {
  Identifier identifier;
  std::vector<std::string> method_names;
  std::vector<std::string> architectures;
  std::optional<Identifier> base_name = std::nullopt;
};

template <typename JsonIt>
bool NeitherEndNorNull(const JsonIt& it, const json& j) {
  return it != j.end() && !it->is_null();
}

bool IsRelevantArchitecture(const Type& type) {
  const auto& architectures = type.architectures;
  return architectures.empty() ||
         absl::c_find(architectures, std::string(kTargetArchitecture)) != architectures.end();
}

bool AreMethodsOrdered(const Type& type) {
  absl::flat_hash_map<std::string, size_t> method_to_last_position;
  for (size_t i = 0; i < type.method_names.size(); ++i) {
    const std::string& method = type.method_names[i];

    if (const auto it = method_to_last_position.find(method); it == method_to_last_position.end()) {
      method_to_last_position.try_emplace(method, i);
    } else {
      if (it->second + 1 != i) {
        return false;
      }
      it->second++;
    }
  }
  return true;
}

void from_json(const json& j, Type& type) {
  type.identifier.name = j["Name"].get<std::string>();

  if (const auto it = j.find("Interface"); NeitherEndNorNull(it, j)) {
    std::string base_name = ((*it)["Name"].get<std::string>());
    std::string api = ((*it)["Api"].get<std::string>());
    type.base_name = Identifier{std::move(base_name), std::move(api)};
  }

  if (const auto it = j.find("Methods"); NeitherEndNorNull(it, j)) {
    absl::c_transform(*it, std::back_inserter(type.method_names),
                      [](const json& method) { return method["Name"].get<std::string>(); });
  }

  if (const auto it = j.find("Architectures"); NeitherEndNorNull(it, j)) {
    absl::c_transform(*it, std::back_inserter(type.architectures),
                      [](const json& architecture) { return architecture.get<std::string>(); });
  }

  ORBIT_CHECK(AreMethodsOrdered(type));
}

static Type ParseType(const json& j, const std::string& path) {
  Type type = j.get<Type>();
  type.identifier.api = fs::path(path).stem();
  return type;
}

struct ApiData {
  absl::flat_hash_map<Identifier, Type> types;
  absl::flat_hash_map<std::string, std::vector<Identifier>> name_to_identifiers;
};

static void ParseFile(const std::string& path, ApiData& api_data) {
  std::ifstream f(path);
  const json data = json::parse(f);

  for (const auto& type_as_json : data["Types"]) {
    Type type = ParseType(type_as_json, path);
    api_data.name_to_identifiers[type.identifier.name].push_back(type.identifier);
    if (IsRelevantArchitecture(type)) {
      api_data.types.try_emplace(type.identifier, std::move(type));
    }
  }
}

void GetVtable(const Identifier& identifier, const ApiData& api_data,
               std::vector<std::string>& vtable) {
  const Type type = api_data.types.at(identifier);
  if (type.base_name.has_value()) {
    GetVtable(type.base_name.value(), api_data, vtable);
  }
  absl::c_copy(type.method_names, std::back_inserter(vtable));
}

std::vector<std::string> GetVtable(const std::string name, const ApiData& api_data) {
  const auto& identifiers = api_data.name_to_identifiers.at(name);
  ORBIT_CHECK(identifiers.size() == 1);
  const Identifier& identifier = identifiers.front();
  std::vector<std::string> result;
  GetVtable(identifier, api_data, result);
  return result;
}

int main(int argc, char* argv[]) {
  ORBIT_CHECK(argc > 1);
  std::string api_folder = argv[1];

  ApiData api_data;

  for (const auto& file : fs::directory_iterator(api_folder)) {
    ParseFile(file.path(), api_data);
  }

  ORBIT_LOG("%d", GetVtable("ID3D11DeviceContext", api_data).size());
  ORBIT_LOG("%d", api_data.types.size());
}