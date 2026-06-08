#include "mod_loader.hpp"
#include "mod_enabled.hpp"
#include "localization.hpp"
#include "registry.hpp"
#include "settings.hpp"
#include "virtual_filesystem.hpp"

#include <leaf/logging/logging.hpp>

#include <sol/sol.hpp>
#include <yaml-cpp/yaml.h>

#include <cmath>
#include <algorithm>
#include <fstream>
#include <functional>
#include <limits>
#include <unordered_map>
#include <utility>

namespace lf {
	namespace detail {
		struct ModSettings {
			std::unordered_map<string, object> values;
			std::unordered_map<string, string> input;
		};

		object loaded_options;
		vector<ModInfo> loaded_mods;
		string loaded_main_scene_path;
		dict loaded_startup_settings;
		std::unordered_map<string, ModSettings> loaded_settings;

		fs::path settings_path(string_view mod_name) {
			return fs::folder::appdata / "settings" / (string(mod_name.empty() ? "core" : mod_name) + ".yaml");
		}

		error write_mod_settings(string_view mod_name) {
			const fs::path path = settings_path(mod_name);
			std::error_code ec;
			fs::create_directories(path.parent_path(), ec);
			if (ec) {
				return error(generic_errc::input_error, format("failed to create '{}': {}", path.parent_path().string(), ec.message()));
			}

			const ModSettings& settings = loaded_settings[string(mod_name)];
			YAML::Emitter out;
			out << YAML::BeginMap;
			out << YAML::Key << "settings" << YAML::Value << YAML::BeginMap;
			for (const auto& [name, value] : settings.values) {
				out << YAML::Key << name << YAML::Value;
				EmitYaml(out, value);
			}
			out << YAML::EndMap;
			out << YAML::Key << "input" << YAML::Value << YAML::BeginMap;
			for (const auto& [action, key] : settings.input) {
				out << YAML::Key << action << YAML::Value << key;
			}
			out << YAML::EndMap;
			out << YAML::EndMap;
			std::ofstream file(path, std::ios::binary);
			if (!file) {
				return error(generic_errc::input_error, format("failed to write '{}'", path.string()));
			}
			file << out.c_str();
			return file ? error::no_error : error(generic_errc::input_error, format("failed to write '{}'", path.string()));
		}

		report<ModSettings> read_mod_settings(string_view mod_name) {
			ModSettings settings;
			const fs::path path = settings_path(mod_name);
			if (!fs::exists(path)) {
				return settings;
			}

			try {
				YAML::Node root = YAML::LoadFile(path.string());
				if (YAML::Node values = root["settings"]) {
					for (const auto& entry : values) {
						settings.values[entry.first.as<string>()] = ObjectFromYaml(entry.second);
					}
				}
				if (YAML::Node input = root["input"]) {
					for (const auto& entry : input) {
						settings.input[entry.first.as<string>()] = entry.second.as<string>();
					}
				}
			} catch (const YAML::Exception& e) {
				return unexpected(error(generic_errc::parse_error, format("loading '{}': {}", path.string(), e.what())));
			}
			return settings;
		}

		error load_mod_settings_cache(string_view mod_name) {
			auto settings = read_mod_settings(mod_name);
			if (!settings) {
				return settings.error();
			}
			loaded_settings[string(mod_name)] = std::move(*settings);
			return error::no_error;
		}
	}

	class DataScriptRunner {
	  public:
		explicit DataScriptRunner(sol::state& lua) : lua(lua) {}

		void run_file(string logical_path, fs::path absolute_path, fs::path relative_root) {
			absolute_path = absolute_path.lexically_normal();
			relative_root = relative_root.lexically_normal();
			report<string> source = expanded_source(std::move(logical_path), absolute_path, relative_root);
			if (!source) {
				throw runtime_exception(source.error().message);
			}

			sol::protected_function_result result = lua.safe_script(*source, sol::script_pass_on_error);
			if (!result.valid()) {
				sol::error script_error = result;
				throw runtime_exception(script_error.what());
			}
		}

	  private:
		sol::state& lua;

		static bool include_line(string_view line, string& path) {
			size_t cursor = 0;
			while (cursor < line.size() && (line[cursor] == ' ' || line[cursor] == '\t')) {
				++cursor;
			}
			constexpr string_view prefix = "include";
			if (line.substr(cursor, prefix.size()) != prefix) {
				return false;
			}
			cursor += prefix.size();
			while (cursor < line.size() && (line[cursor] == ' ' || line[cursor] == '\t')) {
				++cursor;
			}
			if (cursor >= line.size() || line[cursor++] != '(') {
				return false;
			}
			while (cursor < line.size() && (line[cursor] == ' ' || line[cursor] == '\t')) {
				++cursor;
			}
			if (cursor >= line.size() || (line[cursor] != '"' && line[cursor] != '\'')) {
				return false;
			}
			char quote = line[cursor++];
			size_t end = line.find(quote, cursor);
			if (end == string_view::npos) {
				return false;
			}
			path = string(line.substr(cursor, end - cursor));
			return true;
		}

		static report<string> expanded_source(string logical_path, fs::path absolute_path, fs::path relative_root) {
			report<string> source = fs::ReadTextFile(absolute_path.string());
			if (!source) {
				return unexpected(source.error());
			}

			string expanded;
			size_t line_begin = 0;
			while (line_begin <= source->size()) {
				size_t line_end = source->find('\n', line_begin);
				if (line_end == string::npos) {
					line_end = source->size();
				}
				string_view line = string_view(*source).substr(line_begin, line_end - line_begin);
				string include_path;
				if (include_line(line, include_path)) {
					report<fs::path> absolute_include = ResolveVirtualPathReport(include_path, absolute_path.parent_path());
					if (!absolute_include) {
						return unexpected(absolute_include.error());
					}

					fs::path target_root = relative_root;
					fs::path logical_root;
					for (const fs::path& part : fs::path(logical_path)) {
						logical_root = part;
						break;
					}
					if (IsVirtualPath(include_path)) {
						size_t end = include_path.find("__", 2);
						if (end == string::npos || end == 2) {
							return unexpected(error(generic_errc::parse_error, format("invalid virtual path '{}'", include_path)));
						}
						string mod_name(include_path.substr(2, end - 2));
						report<fs::path> virtual_root = ResolveVirtualPathReport(string("__") + mod_name + "__/");
						if (!virtual_root) {
							return unexpected(virtual_root.error());
						}
						target_root = *virtual_root;
						logical_root = mod_name;
					}

					fs::path normalized_include = absolute_include->lexically_normal();
					fs::path normalized_root = target_root.lexically_normal();
					fs::path relative_include = normalized_include.lexically_relative(normalized_root);
					bool escapes_root = relative_include.empty();
					for (const fs::path& part : relative_include) {
						if (part == "..") {
							escapes_root = true;
							break;
						}
					}
					if (escapes_root) {
						return unexpected(error(generic_errc::parse_error, format("include '{}' escapes its mod root", include_path)));
					}

					report<string> included = expanded_source((logical_root / relative_include).generic_string(), normalized_include, normalized_root);
					if (!included) {
						return unexpected(included.error());
					}
					expanded += *included;
					if (!expanded.empty() && expanded.back() != '\n') {
						expanded += '\n';
					}
				} else {
					expanded += line;
					if (line_end < source->size()) {
						expanded += '\n';
					}
				}
				if (line_end == source->size()) {
					break;
				}
				line_begin = line_end + 1;
			}
			return expanded;
		}
	};

	string operator_to_string(ModDependency::Operator op) {
		switch (op) {
		case ModDependency::Operator::Equal: return "==";
		case ModDependency::Operator::NotEqual: return "!=";
		case ModDependency::Operator::Greater: return ">";
		case ModDependency::Operator::GreaterEqual: return ">=";
		case ModDependency::Operator::Less: return "<";
		case ModDependency::Operator::LessEqual: return "<=";
		case ModDependency::Operator::Any: return "any";
		default: return "?";
		}
	}
	const char* sol_type_name(sol::type t) {
		switch (t) {
		case sol::type::lua_nil: return "nil";
		case sol::type::boolean: return "boolean";
		case sol::type::number: return "number";
		case sol::type::string: return "string";
		case sol::type::table: return "table";
		case sol::type::function: return "function";
		case sol::type::userdata: return "userdata";
		case sol::type::lightuserdata: return "lightuserdata";
		case sol::type::thread: return "thread";
		default: return "unknown";
		}
	}
	string sol_object_to_string(const sol::object& v) {
		using sol::type;
		switch (v.get_type()) {
		case type::lua_nil: return "nil";
		case type::boolean: return v.as<bool>() ? "true" : "false";
		case type::number: return format("{}", v.as<double>());
		case type::string: return "\"" + v.as<string>() + "\"";
		case type::table: {
			string result;
			for (auto& [key, value] : v.as<sol::table>()) {
				result += "[" + sol_object_to_string(key) + "] = " + sol_object_to_string(value) + ", ";
			}
			return result;
		}
		default: return format("<{}>", sol_type_name(v.get_type()));
		}
	}

	string sol_setting_to_string(const sol::object& v) {
		using sol::type;
		switch (v.get_type()) {
		case type::boolean: return v.as<bool>() ? "true" : "false";
		case type::number: return format("{}", v.as<double>());
		case type::string: return v.as<string>();
		default: return sol_object_to_string(v);
		}
	}

	object sol_to_object(const sol::object& v) {
		using sol::type;
		switch (v.get_type()) {
		case type::lua_nil: return object();
		case type::boolean: return object(v.as<bool>());
		case type::number: {
			double d = v.as<double>();
			double id;
			if (std::modf(d, &id) == 0.0) {
				if (id >= static_cast<double>(std::numeric_limits<i64>::min()) &&
					id <= static_cast<double>(std::numeric_limits<i64>::max())) {
					return object(static_cast<i64>(id));
				} else if (id >= 0 && id <= static_cast<double>(std::numeric_limits<u64>::max())) {
					return object(static_cast<u64>(id));
				}
			}
			return object(d);
		}
		case type::string: return object(v.as<string>());
		case type::table: {
			sol::table t = v.as<sol::table>();
			bool seen_string_keys = false;
			bool seen_number_keys = false;
			for (const auto& kv : t) {
				sol::type kt = kv.first.get_type();
				if (kt == type::string) {
					seen_string_keys = true;
				} else if (kt == type::number) {
					seen_number_keys = true;
				} else {
					throw runtime_exception(format("invalid table key type '{}' when converting lua table to object", sol_type_name(kt)));
				}
				if (seen_string_keys && seen_number_keys) {
					throw runtime_exception("mixed string and numeric keys in lua table are not supported: " + sol_object_to_string(t));
				}
			}
			if (seen_number_keys) {
				i64 max_index = 0;
				struct Entry {
					i64 idx;
					object val;
				};
				std::vector<Entry> entries;
				entries.reserve(t.size());
				for (const auto& kv : t) {
					double d = kv.first.as<double>();
					double id;
					if (std::modf(d, &id) != 0.0) {
						throw runtime_exception(format("non-integer numeric key '{}' in list", d));
					}
					i64 idx = static_cast<i64>(id);
					if (idx < 1) {
						throw runtime_exception(format("list index '{}' must be >= 1", idx));
					}
					if (idx > max_index) {
						max_index = idx;
					}
					entries.push_back({ idx, sol_to_object(kv.second) });
				}
				lf::list lst;
				lst.resize(static_cast<size_t>(max_index));
				for (const auto& e : entries) {
					lst[static_cast<size_t>(e.idx - 1)] = e.val;
				}
				return object(lst);
			}
			// dict
			lf::dict d;
			for (const auto& kv : t) {
				string k = kv.first.as<string>();
				d.emplace(k, sol_to_object(kv.second));
			}
			return object(d);
		}
		default:
			throw runtime_exception(format("unsupported lua type '{}' when converting to object", sol_type_name(v.get_type())));
		}
	}
	string version_to_string(const version& v) {
		string s =
			std::to_string(v.major) + "." + std::to_string(v.minor) + "." + std::to_string(v.patch);
		if (v.snapshot != 0) {
			s += "-" + std::to_string(v.snapshot);
		}
		return s;
	}
	const char* dependency_type_name(ModDependency::Type type) {
		switch (type) {
		case ModDependency::Type::Required: return "required";
		case ModDependency::Type::Optional: return "optional";
		case ModDependency::Type::Forbidden: return "forbidden";
		default: return "unknown";
		}
	}
	string dependency_to_string(const ModDependency& dependency) {
		return format("{} {} {} {}", dependency_type_name(dependency.type),
					  dependency.name,
					  operator_to_string(dependency.op),
					  version_to_string(dependency.required_version));
	}
	string dependencies_to_string(const vector<ModDependency>& dependencies) {
		if (dependencies.empty()) {
			return "none";
		}
		string result;
		for (size_t i = 0; i < dependencies.size(); ++i) {
			if (i != 0) {
				result += "; ";
			}
			result += dependency_to_string(dependencies[i]);
		}
		return result;
	}
	string prototype_counts_to_string() {
		string result;
		for (const auto& fn : PrototypeTypeRegistry::functions) {
			const size_t count = fn.count();
			if (count == 0) {
				continue;
			}
			if (!result.empty()) {
				result += ", ";
			}
			result += string(fn.type()) + "=" + std::to_string(count);
		}
		return result.empty() ? "none" : result;
	}
	string mod_names_to_string(span<const ModInfo> mods) {
		if (mods.empty()) {
			return "none";
		}
		string result;
		for (size_t i = 0; i < mods.size(); ++i) {
			if (i != 0) {
				result += ", ";
			}
			result += mods[i].name;
		}
		return result;
	}
	void report_mod_progress(ModLoadProgress* progress, string stage, f32 stage_progress, string process, f32 process_progress) {
		if (progress && progress->report) {
			progress->report(std::move(stage), stage_progress, std::move(process), process_progress);
		}
	}
	f32 mod_stage_progress(u32 completed_stages) {
		constexpr f32 stage_count = 6.0f;
		return static_cast<f32>(completed_stages) / stage_count;
	}

	void set_lua_value(sol::table table, string_view name, const object& value) {
		string key(name);
		if (value.is<bool>()) {
			table[key] = value.get<bool>();
		} else if (value.is<i64>()) {
			table[key] = value.get<i64>();
		} else if (value.is<u64>()) {
			table[key] = value.get<u64>();
		} else if (value.is<f64>()) {
			table[key] = value.get<f64>();
		} else if (value.is<string>()) {
			table[key] = value.get<string>();
		}
	}

	void initialize_prototype_lua(sol::state& lua) {
		lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
		lua.script(R"(
data = {}
data["raw"] = {}
function data:extend(prototypes)
	for i = 1, #prototypes do
		local prototype = prototypes[i]
		if prototype.mod == nil then
			prototype.mod = __leaf_current_mod
		end
		local type_table = self.raw[prototype.type]
		if type_table == nil then
			error("data.raw[" .. tostring(prototype.type) .. "] does not exist")
		end
		if type_table[prototype.name] ~= nil then
			error("duplicate prototype '" .. tostring(prototype.type) .. "/" .. tostring(prototype.name) .. "'")
		end
		type_table[prototype.name] = prototype
	end
end
option = {}
option["scene"] = {}
settings = {}
settings["startup"] = {}
)");
		for (auto& type : PrototypeTypeRegistry::functions) {
			lua["data"]["raw"][type.type()] = lua.create_table();
		}
		sol::table startup = lua["settings"]["startup"];
		for (const auto& [name, value] : detail::loaded_startup_settings) {
			sol::table setting = lua.create_table();
			set_lua_value(setting, "value", value);
			startup[name] = setting;
		}
	}

	const ModInfo* find_mod(span<const ModInfo> mods, string_view name) {
		for (const ModInfo& mod : mods) {
			if (mod.name == name) {
				return &mod;
			}
		}
		return nullptr;
	}

	error load_prototype_script(sol::state& lua, const ModInfo& mod, string_view file_name, bool required = false) {
		fs::path path = mod.location / string(file_name);
		if (!fs::exists(path)) {
			if (required) {
				log::Error("{}", format("[mod-loader] missing required script: {}/{} ({})", mod.name, file_name, path.string()));
				return error(generic_errc::input_error, "Missing required prototype script " + mod.name + "/" + string(file_name));
			}
			log::Debug("{}", format("[mod-loader] script skipped: {}/{} (missing optional)", mod.name, file_name));
			return error::no_error;
		}

		try {
			DataScriptRunner runner(lua);
			lua["__leaf_current_mod"] = mod.name;
			log::Debug("{}", format("[mod-loader] script begin: {}/{} ({})", mod.name, file_name, path.string()));
			runner.run_file(mod.name + "/" + string(file_name), path, mod.location);
			log::Info("{}", format("[mod-loader] script: \"{}\"/{}", mod.name, file_name));
			return error::no_error;
		} catch (const lf::exception& e) {
			log::Error("{}", format("[mod-loader] error: {}/{}: {}", mod.name, file_name, e.what()));
			return error(generic_errc::parse_error, "Failed to execute " + mod.name + "/" + string(file_name) + ": " + e.what());
		}
	}

	void initialize_settings_lua(
		sol::state& lua,
		vector<std::pair<string, object>>& setting_defaults,
		vector<std::pair<string, string>>& input_defaults) {
		lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
		lua.set_function("__leaf_set_local_input_binding", [&input_defaults](string_view action, string_view key) {
			input_defaults.emplace_back(string(action), string(key));
		});
		lua.set_function("__leaf_declare_setting", [&setting_defaults, &input_defaults](string_view type, string_view name, string_view setting_type, sol::object default_value) {
			if (name.empty()) {
				throw runtime_exception(format("{} is missing name", type));
			}
			if (default_value.get_type() == sol::type::lua_nil) {
				throw runtime_exception(format("{} '{}' is missing default_value", type, name));
			}

			object value = sol_to_object(default_value);
			if (setting_type == "startup") {
				detail::loaded_startup_settings[string(name)] = value;
			}
			if (type == "input-setting") {
				if (setting_type != "runtime-per-user") {
					throw runtime_exception(format("input-setting '{}' must use setting_type='runtime-per-user'", name));
				}
				input_defaults.emplace_back(string(name), value.as<string>());
			} else if (setting_type == "runtime-per-user") {
				setting_defaults.emplace_back(string(name), sol_to_object(default_value));
			}
		});
		lua.script(R"(
data = {}
local setting_types = {
	["bool-setting"] = true,
	["int-setting"] = true,
	["double-setting"] = true,
	["string-setting"] = true,
	["input-setting"] = true,
}
local setting_scopes = {
	["startup"] = true,
	["runtime-global"] = true,
	["runtime-per-user"] = true,
}

function data:extend(settings)
	for i = 1, #settings do
		local setting = settings[i]
		if setting_types[setting.type] ~= true then
			error("unknown setting type '" .. tostring(setting.type) .. "'")
		end
		if setting.name == nil then
			error(tostring(setting.type) .. " is missing name")
		end
		if setting.setting_type == nil then
			error(tostring(setting.type) .. " '" .. tostring(setting.name) .. "' is missing setting_type")
		end
		if setting_scopes[setting.setting_type] ~= true then
			error(tostring(setting.type) .. " '" .. tostring(setting.name) .. "' has invalid setting_type '" .. tostring(setting.setting_type) .. "'")
		end
		if setting.type == "int-setting" or setting.type == "double-setting" then
			if setting.minimum_value ~= nil and setting.default_value < setting.minimum_value then
				error(tostring(setting.type) .. " '" .. tostring(setting.name) .. "' default_value is below minimum_value")
			end
			if setting.maximum_value ~= nil and setting.default_value > setting.maximum_value then
				error(tostring(setting.type) .. " '" .. tostring(setting.name) .. "' default_value is above maximum_value")
			end
		end
		if setting.type == "string-setting" then
			local default = tostring(setting.default_value)
			if setting.minimum_length ~= nil and #default < setting.minimum_length then
				error("string-setting '" .. tostring(setting.name) .. "' default_value is shorter than minimum_length")
			end
			if setting.maximum_length ~= nil and #default > setting.maximum_length then
				error("string-setting '" .. tostring(setting.name) .. "' default_value is longer than maximum_length")
			end
			if setting.allowed_values ~= nil then
				if type(setting.allowed_values) ~= "table" then
					error("string-setting '" .. tostring(setting.name) .. "' allowed_values must be a table")
				end
				local allowed = false
				for _, value in ipairs(setting.allowed_values) do
					if default == tostring(value) then
						allowed = true
						break
					end
				end
				if not allowed then
					error("string-setting '" .. tostring(setting.name) .. "' default_value is not in allowed_values")
				end
			end
		end
		__leaf_declare_setting(tostring(setting.type), tostring(setting.name), tostring(setting.setting_type), setting.default_value)
	end
end
)");
	}

	error sync_loaded_mod_settings(span<const ModInfo> mods) {
		for (const ModInfo& mod : mods) {
			if (error err = detail::load_mod_settings_cache(mod.name)) {
				return err.add_context(format("loading settings for '{}'", mod.name));
			}
			log::Debug("{}", format("[mod-loader] settings sync: {}", mod.name));
			vector<std::pair<string, object>> setting_defaults;
			vector<std::pair<string, string>> input_defaults;
			sol::state lua;
			initialize_settings_lua(lua, setting_defaults, input_defaults);
			if (error err = load_prototype_script(lua, mod, "settings.lua")) {
				return err.add_context("loading settings defaults");
			}
			for (const auto& [action, key] : input_defaults) {
				detail::loaded_settings[mod.name].input.try_emplace(action, key);
			}
			for (const auto& [name, value] : setting_defaults) {
				detail::loaded_settings[mod.name].values.try_emplace(name, value);
			}
			if (error err = detail::write_mod_settings(mod.name)) {
				return err.add_context(format("saving settings for '{}'", mod.name));
			}
		}
		return error::no_error;
	}

	bool version_compare(const version& a, const version& b, ModDependency::Operator op) {
		auto cmp = [](const version& lhs, const version& rhs) {
			if (lhs.major != rhs.major) {
				return lhs.major < rhs.major ? -1 : 1;
			}
			if (lhs.minor != rhs.minor) {
				return lhs.minor < rhs.minor ? -1 : 1;
			}
			if (lhs.patch != rhs.patch) {
				return lhs.patch < rhs.patch ? -1 : 1;
			}
			if (lhs.snapshot != rhs.snapshot) {
				return lhs.snapshot < rhs.snapshot ? -1 : 1;
			}
			return 0;
		};
		int res = cmp(a, b);
		switch (op) {
		case ModDependency::Operator::Equal: return res == 0;
		case ModDependency::Operator::NotEqual: return res != 0;
		case ModDependency::Operator::Greater: return res > 0;
		case ModDependency::Operator::GreaterEqual: return res > 0 || res == 0;
		case ModDependency::Operator::Less: return res < 0;
		case ModDependency::Operator::LessEqual: return res < 0 || res == 0;
		case ModDependency::Operator::Any: return true;
		default: return false;
		}
	}
	error sort_mods_by_dependency(vector<ModInfo>& mods) {
		log::Debug("{}", format("[mod-loader] dependency resolution begin: {} enabled mod(s)", mods.size()));
		std::unordered_map<string, size_t> mod_index;
		for (size_t i = 0; i < mods.size(); ++i) {
			mod_index[mods[i].name] = i;
			log::Debug("{}", format("[mod-loader] dependency input: {} v{} deps=[{}]",
									mods[i].name,
									version_to_string(mods[i].mod_version),
									dependencies_to_string(mods[i].dependencies)));
		}

		std::vector<std::vector<size_t>> graph(mods.size());
		std::vector<string> errors;
		for (size_t i = 0; i < mods.size(); ++i) {
			for (const auto& dep : mods[i].dependencies) {
				auto it = mod_index.find(dep.name);
				if (dep.type == ModDependency::Type::Forbidden) {
					if (it != mod_index.end()) {
						// Check forbidden version constraint
						if (version_compare(mods[it->second].mod_version, dep.required_version,
											dep.op)) {
							string msg =
								"Mod '" + mods[i].name + "' forbids mod '" + dep.name +
								"' (version " + version_to_string(mods[it->second].mod_version) +
								") matching forbidden constraint '" + operator_to_string(dep.op) +
								" " + version_to_string(dep.required_version) + "'.";
							errors.push_back(msg);
						}
					}
					continue;
				}
				if (dep.type == ModDependency::Type::Required ||
					dep.type == ModDependency::Type::Optional) {
					if (it != mod_index.end()) {
						// Check required/optional version constraint
						if (!version_compare(mods[it->second].mod_version, dep.required_version,
											 dep.op)) {
							string msg = "Mod '" + mods[i].name + "' requires mod '" + dep.name +
										 " " + operator_to_string(dep.op) + " version " +
										 version_to_string(dep.required_version) + "' but '" +
										 dep.name + "' is '" +
										 version_to_string(mods[it->second].mod_version) + "'.";
							errors.push_back(msg);
						}
						// Reverse edge: dependency -> mod
						graph[it->second].push_back(i);
						log::Debug("{}", format("[mod-loader] dependency edge: {} -> {}", dep.name, mods[i].name));
					} else if (dep.type == ModDependency::Type::Required) {
						// Required dependency missing
						string msg = "Mod '" + mods[i].name + "' requires mod '" + dep.name +
									 "' but it is missing.";
						errors.push_back(msg);
					}
				}
			}
		}

		// If any errors, return all
		if (!errors.empty()) {
			string all_errors;
			for (const auto& e : errors) {
				log::Error("{}", format("[mod-loader] dependency error: {}", e));
				all_errors += e + "\n";
			}
			return error(generic_errc::unknown, all_errors);
		}

		// Topological sort with cycle detection
		std::vector<bool> visited(mods.size(), false);
		std::vector<bool> on_stack(mods.size(), false);
		std::vector<size_t> order;
		std::vector<size_t> cycle;
		std::function<bool(size_t)> dfs = [&](size_t u) {
			visited[u] = true;
			on_stack[u] = true;
			for (size_t v : graph[u]) {
				if (!visited[v]) {
					if (dfs(v)) {
						if (cycle.empty() || cycle.front() != v) {
							cycle.push_back(v);
						}
						return true;
					}
				} else if (on_stack[v]) {
					// Cycle detected
					cycle.push_back(v);
					return true;
				}
			}
			on_stack[u] = false;
			order.push_back(u);
			return false;
		};
		for (size_t i = 0; i < mods.size(); ++i) {
			if (!visited[i]) {
				if (dfs(i)) {
					// Cycle detected, build error message
					string msg = "Circular dependency detected: ";
					for (size_t idx : cycle) {
						msg += mods[idx].name + " -> ";
					}
					msg += mods[cycle.front()].name;
					return error(generic_errc::unknown).add_context(msg);
				}
			}
		}

		// Reorder mods vector in dependency order
		std::reverse(order.begin(), order.end());
		vector<ModInfo> sorted;
		sorted.reserve(mods.size());
		for (size_t idx : order) {
			sorted.push_back(std::move(mods[idx]));
		}
		mods = std::move(sorted);
		log::Debug("{}", "[mod-loader] dependency resolution complete");
		for (size_t i = 0; i < mods.size(); ++i) {
			log::Debug("{}", format("[mod-loader] load-order[{}]: {} v{}", i + 1, mods[i].name, version_to_string(mods[i].mod_version)));
		}
		return error::no_error;
	}

	error LoadDataRaw(const sol::state& lua) {
		object data_raw_obj = sol_to_object(lua["data"]["raw"]);
		if (!data_raw_obj.is<dict>()) {
			return error(generic_errc::type_mismatch, "data.raw must be a table/dict");
		}
		dict& data_raw = data_raw_obj.get<dict>();

		for (auto& fn : lf::PrototypeTypeRegistry::functions) {
			auto it = data_raw.find(fn.type());
			if (it != data_raw.end()) {
				if (!it->second.is<dict>()) {
					return error(generic_errc::type_mismatch, "data.raw." + string(fn.type()) + " must be a table/dict");
				}
				dict& type_table = it->second.get<dict>();
				fn.reserve(type_table.size());
				for (const auto& [name, data] : type_table) {
					if (!data.is<dict>()) {
						return error(generic_errc::type_mismatch, lf::format("data.raw[{}][{}] must be a table/dict", fn.type(), name));
					}
					try {
						fn.create(name, data.get<dict>());
					} catch (const lf::exception& e) {
						return error(generic_errc::parse_error, e.what()).add_context(lf::format("creating prototype '{}' (type:{})", name, fn.type()));
					}
				}
			}
		}
		return error::no_error;
	}

	error LoadOptions(const sol::state& lua) {
		sol::object option_obj = lua["option"];
		if (!option_obj.is<sol::table>()) {
			return error(generic_errc::missing_field, "missing required global 'option'");
		}

		detail::loaded_options = sol_to_object(option_obj);

		sol::table option = option_obj.as<sol::table>();
		sol::object scene_obj = option["scene"];
		if (!scene_obj.is<sol::table>()) {
			return error(generic_errc::missing_field, "missing required option.scene table");
		}
		sol::table scene = scene_obj.as<sol::table>();
		sol::object main_obj = scene["main"];
		if (!main_obj.is<sol::table>()) {
			return error(generic_errc::missing_field, "missing required option.scene.main table");
		}
		sol::table main = main_obj.as<sol::table>();
		sol::object path_obj = main["path"];
		if (!path_obj.is<string>()) {
			return error(generic_errc::type_mismatch, "option.scene.main.path must be a string");
		}
		detail::loaded_main_scene_path = path_obj.as<string>();
		if (detail::loaded_main_scene_path.empty()) {
			return error(generic_errc::input_error, "option.scene.main.path cannot be empty");
		}
		return error::no_error;
	}

	error LoadMods(ModCollection& mod_tree, ModLoadProgress* progress) {
		log::Info("{}", "[mod-loader] loading mods");
		for (const auto& func : PrototypeTypeRegistry::functions) {
			func.clear();
		}
		ClearVirtualFileSpace();
		ClearLocalization();
		detail::loaded_options = {};
		detail::loaded_mods.clear();
		detail::loaded_main_scene_path.clear();
		detail::loaded_startup_settings.clear();
		log::Debug("{}", format("[mod-loader] reset runtime state: prototype_types={}", PrototypeTypeRegistry::functions.size()));

		report_mod_progress(progress, "collecting-sorting-mods", mod_stage_progress(0), "scanning-mod-directories", 0.0f);
		vector<ModInfo> mods;
		auto add_mods_from_dir = [&](const fs::path& dir, bool privileged) {
			log::Debug("{}", format("[mod-loader] scan root: kind={} path={}", privileged ? "privileged" : "unprivileged", dir.string()));
			if (!fs::exists(dir)) {
				fs::create_directories(dir);
				log::Debug("{}", format("[mod-loader] created missing mod directory: {}", dir.string()));
			}
			vector<fs::path> mod_dirs;
			for (const auto& entry : fs::directory_iterator(dir)) {
				if (entry.is_directory()) {
					auto info_path = entry.path() / "info.yaml";
					if (fs::exists(info_path)) {
						mod_dirs.push_back(entry.path());
					} else {
						log::Debug("{}", format("[mod-loader] ignored non-mod directory: {}", entry.path().string()));
					}
				}
			}
			std::sort(mod_dirs.begin(), mod_dirs.end(), [](const fs::path& a, const fs::path& b) {
				return a.filename().generic_string() < b.filename().generic_string();
			});
			log::Debug("{}", format("[mod-loader] candidates in root: count={} path={}", mod_dirs.size(), dir.string()));
			for (const fs::path& mod_dir : mod_dirs) {
				ModInfo info = parse_mod_info((mod_dir / "info.yaml").string(), privileged);
				log::Debug("{}", format("[mod-loader] discovered: {} v{} title='{}' path={} privileged={} deps=[{}]",
										info.name,
										version_to_string(info.mod_version),
										info.title,
										info.location.string(),
										info.privileged ? "true" : "false",
										dependencies_to_string(info.dependencies)));
				mods.emplace_back(std::move(info));
			}
		};
		for (const auto& dir : mod_tree.privileged_dirs) {
			add_mods_from_dir(dir, true);
		}
		for (const auto& dir : mod_tree.unprivileged_dirs) {
			add_mods_from_dir(dir, false);
		}
		log::Info("{}", format("[mod-loader] discovered {} mod(s): {}", mods.size(), mod_names_to_string(mods)));

		report_mod_progress(progress, "collecting-sorting-mods", mod_stage_progress(0), "reading-enabled-mods", 0.35f);
		fs::path enabled_mods_path = fs::folder::appdata / "enabled_mods.yaml";
		log::Debug("{}", format("[mod-loader] enabled file: {}", enabled_mods_path.string()));
		sync_enabled_mods(enabled_mods_path, mods);
		auto enabled_mods = load_enabled_mods(enabled_mods_path);
		log::Debug("{}", format("[mod-loader] enabled entries: {}", enabled_mods.size()));

		vector<ModInfo> enabled_mod_list;
		for (const auto& mod : mods) {
			auto it = enabled_mods.find(mod.name);
			if (it != enabled_mods.end() && it->second.enabled) {
				log::Debug("{}", format("[mod-loader] selected: {} v{} enabled=true", mod.name, version_to_string(mod.mod_version)));
				enabled_mod_list.push_back(mod);
			} else {
				log::Debug("{}", format("[mod-loader] selected: {} v{} enabled=false", mod.name, version_to_string(mod.mod_version)));
			}
		}
		log::Info("{}", format("[mod-loader] enabled {} mod(s): {}", enabled_mod_list.size(), mod_names_to_string(enabled_mod_list)));

		report_mod_progress(progress, "collecting-sorting-mods", mod_stage_progress(0), "resolving-dependencies", 0.7f);
		error sort_result = sort_mods_by_dependency(enabled_mod_list);
		if (sort_result) {
			return sort_result;
		}
		report_mod_progress(progress, "collecting-sorting-mods", mod_stage_progress(1), "load-order-ready", 1.0f);

		log::Info("{}", "[mod-loader] load order:");
		for (size_t i = 0; i < enabled_mod_list.size(); ++i) {
			const auto& mod = enabled_mod_list[i];
			RegisterVirtualRoot(mod.name, mod.location);
			log::Info("{}", format("[mod-loader]   {}. \"{}\" v{}", i + 1, mod.name, version_to_string(mod.mod_version)));
		}

		if (error err = detail::load_mod_settings_cache("core")) {
			return err.add_context("loading settings/core.yaml");
		}
		auto language_result = LoadSetting("core", "language", object(default_language));
		if (!language_result) {
			return language_result.error().add_context("loading settings/core.yaml");
		}
		string selected_language = language_result->as<string>();
		log::Info("{}", format("[mod-loader] language: {}", selected_language));

		report_mod_progress(progress, "loading-settings", mod_stage_progress(1), "syncing-mod-settings", 0.0f);
		log::Debug("{}", "[mod-loader] loading mod settings");
		if (error settings_err = sync_loaded_mod_settings(span<const ModInfo>(enabled_mod_list.data(), enabled_mod_list.size()))) {
			return settings_err.add_context("syncing mod settings");
		}
		report_mod_progress(progress, "loading-settings", mod_stage_progress(2), "mod-settings-loaded", 1.0f);
		log::Info("{}", format("[mod-loader] settings: {} startup setting(s)", detail::loaded_startup_settings.size()));

		report_mod_progress(progress, "loading-localization", mod_stage_progress(0), selected_language, 0.0f);
		log::Info("{}", format("[mod-loader] loading locale: {}", selected_language));
		error locale_error = LoadLocaleFiles(span<const ModInfo>(enabled_mod_list.data(), enabled_mod_list.size()), selected_language);
		if (locale_error) {
			return locale_error.add_context("loading locale files");
		}

		{
			log::Info("{}", "[mod-loader] loading built-in prototype defaults");
			report_mod_progress(progress, "loading-mod-prototypes", mod_stage_progress(2), "core/null.lua", 0.0f);
			sol::state lua;
			initialize_prototype_lua(lua);

			const ModInfo* core = find_mod(span<const ModInfo>(enabled_mod_list.data(), enabled_mod_list.size()), "core");
			if (!core) {
				return error(generic_errc::input_error, "required core mod was not found");
			}
			if (error err = load_prototype_script(lua, *core, "null.lua", true)) {
				return err;
			}
			report_mod_progress(progress, "loading-mod-prototypes", mod_stage_progress(2), "core/null.lua", 0.2f);
			auto err = LoadDataRaw(lua);
			if (err) {
				return err.add_context("loading null prototypes from core/null.lua");
			}
			report_mod_progress(progress, "loading-mod-prototypes", mod_stage_progress(2), "registered-null-prototypes", 0.35f);
			log::Debug("{}", format("[mod-loader] built-in prototypes: {}", prototype_counts_to_string()));
		}

		sol::state lua;
		initialize_prototype_lua(lua);

		log::Info("{}", "[mod-loader] loading data scripts");
		for (size_t i = 0; i < enabled_mod_list.size(); ++i) {
			const auto& mod = enabled_mod_list[i];
			f32 progress_base = enabled_mod_list.empty() ? 1.0f : static_cast<f32>(i) / static_cast<f32>(enabled_mod_list.size());
			report_mod_progress(progress, "loading-mod-prototypes", mod_stage_progress(2), mod.name + "/data.lua", 0.35f + progress_base * 0.45f);
			log::Debug("{}", format("[mod-loader] data.lua {}/{}: {}", i + 1, enabled_mod_list.size(), mod.name));
			if (error err = load_prototype_script(lua, mod, "data.lua")) {
				return err;
			}
		}

		report_mod_progress(progress, "loading-mod-prototypes", mod_stage_progress(2), "data-lua-files-loaded", 0.8f);
		report_mod_progress(progress, "loading-mod-prototypes", mod_stage_progress(2), "loading-options", 0.85f);
		log::Debug("{}", "[mod-loader] loading global mod options");
		auto option_err = LoadOptions(lua);
		if (option_err) {
			return option_err.add_context("loading options from mods' data.lua");
		}
		detail::loaded_mods = enabled_mod_list;
		log::Info("{}", format("[mod-loader] main scene: {}", detail::loaded_main_scene_path));

		report_mod_progress(progress, "loading-mod-prototypes", mod_stage_progress(2), "creating-prototypes", 0.9f);
		log::Info("{}", "[mod-loader] creating prototypes from data.raw");
		auto err = LoadDataRaw(lua);
		if (err) {
			return err.add_context("loading prototypes from mods' data.lua");
		}
		log::Info("{}", "[mod-loader] prototypes created");
		report_mod_progress(progress, "loading-mod-prototypes", mod_stage_progress(3), "prototypes-loaded", 1.0f);
		log::Debug("{}", format("[mod-loader] prototypes: {}", prototype_counts_to_string()));

		log::Info("{}", "[mod-loader] linking prototype references");
		for (size_t i = 0; i < PrototypeTypeRegistry::functions.size(); ++i) {
			const auto& fn = PrototypeTypeRegistry::functions[i];
			try {
				f32 process_progress = PrototypeTypeRegistry::functions.empty() ? 1.0f : static_cast<f32>(i) / static_cast<f32>(PrototypeTypeRegistry::functions.size());
				report_mod_progress(progress, "linking-prototypes", mod_stage_progress(3), string(fn.type()), process_progress);
				fn.resolve();
			} catch (const lf::exception& e) {
				return error(generic_errc::parse_error, e.what()).add_context("linking prototypes");
			}
		}
		log::Info("{}", "[mod-loader] prototype references linked");

		report_mod_progress(progress, "linking-prototypes", mod_stage_progress(4), "prototype-references-linked", 1.0f);
		report_mod_progress(progress, "loading-assets", mod_stage_progress(4), "checking-asset-work", 0.0f);
		report_mod_progress(progress, "loading-assets", mod_stage_progress(5), "assets-loaded", 1.0f);
		report_mod_progress(progress, "finish", mod_stage_progress(5), "finalizing-startup", 0.0f);
		report_mod_progress(progress, "finish", mod_stage_progress(6), "mods-loaded", 1.0f);
		return error::no_error;
	}

	const object& LoadedModOptions() {
		return detail::loaded_options;
	}

	string_view LoadedMainScenePath() {
		return detail::loaded_main_scene_path;
	}

	const vector<ModInfo>& LoadedMods() {
		return detail::loaded_mods;
	}

	report<object> LoadSetting(string_view mod_name, string_view name, object fallback) {
		const string mod_key(mod_name.empty() ? "core" : mod_name);
		auto mod = detail::loaded_settings.find(mod_key);
		if (mod == detail::loaded_settings.end()) {
			return fallback;
		}
		if (auto value = mod->second.values.find(string(name)); value != mod->second.values.end()) {
			return value->second;
		}
		return fallback;
	}

	error SaveSetting(string_view mod_name, string_view name, object value) {
		const string mod_key(mod_name.empty() ? "core" : mod_name);
		detail::loaded_settings[mod_key].values[string(name)] = std::move(value);
		return detail::write_mod_settings(mod_key);
	}

	error EnsureSetting(string_view mod_name, string_view name, object value) {
		const string mod_key(mod_name.empty() ? "core" : mod_name);
		detail::loaded_settings[mod_key].values.try_emplace(string(name), std::move(value));
		return error::no_error;
	}

	report<string> LoadInputSetting(string_view mod_name, string_view action) {
		const string mod_key(mod_name.empty() ? "core" : mod_name);
		auto mod = detail::loaded_settings.find(mod_key);
		if (mod == detail::loaded_settings.end()) {
			return string();
		}
		if (auto value = mod->second.input.find(string(action)); value != mod->second.input.end()) {
			return value->second;
		}
		return string();
	}

	error EnsureInputSetting(string_view mod_name, string_view action, string_view key) {
		const string mod_key(mod_name.empty() ? "core" : mod_name);
		detail::loaded_settings[mod_key].input.try_emplace(string(action), string(key));
		return error::no_error;
	}

	void UnloadMods() {
		for (const auto& func : PrototypeTypeRegistry::functions) {
			func.clear();
		}
		ClearVirtualFileSpace();
		ClearLocalization();
		detail::loaded_options = {};
		detail::loaded_mods.clear();
		detail::loaded_main_scene_path.clear();
		detail::loaded_startup_settings.clear();
		detail::loaded_settings.clear();
	}
} // namespace lf
