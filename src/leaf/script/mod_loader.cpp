#include "mod_loader.hpp"
#include "mod_enabled.hpp"
#include "localization.hpp"
#include "registry.hpp"
#include "settings.hpp"
#include "virtual_filesystem.hpp"

#include <leaf/core/messages.hpp>
#include <leaf/system/system.hpp>

#include <sol/sol.hpp>

#include <cmath>
#include <functional>
#include <limits>
#include <unordered_map>
#include <utility>

namespace lf {
	namespace {
		ModOptions& loaded_options() {
			static ModOptions options;
			return options;
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
			report<string> source = ReadTextFile(absolute_path.string());
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
	void report_mod_progress(ModLoadProgress* progress, string stage, f32 stage_progress, string process, f32 process_progress) {
		if (progress && progress->report) {
			progress->report(std::move(stage), stage_progress, std::move(process), process_progress);
		}
	}
	f32 mod_stage_progress(u32 completed_stages) {
		constexpr f32 stage_count = 5.0f;
		return static_cast<f32>(completed_stages) / stage_count;
	}

	void initialize_prototype_lua(sol::state& lua) {
		lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
		lua.script(R"(
data = {}
data["raw"] = {}
function data:extend(prototypes)
	for i = 1, #prototypes do
		local prototype = prototypes[i]
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
)");
		for (auto& type : PrototypeTypeRegistry::functions) {
			lua["data"]["raw"][type.type()] = lua.create_table();
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
				return error(generic_errc::input_error, "Missing required prototype script " + mod.name + "/" + string(file_name));
			}
			return error::no_error;
		}

		try {
			DataScriptRunner runner(lua);
			runner.run_file(mod.name + "/" + string(file_name), path, mod.location);
			log_info(format("[mod-loader] loaded: {}/{}", mod.name, file_name));
			return error::no_error;
		} catch (const lf::exception& e) {
			log_error(format("[mod-loader] error: {}/{}: {}", mod.name, file_name, e.what()));
			return error(generic_errc::parse_error, "Failed to execute " + mod.name + "/" + string(file_name) + ": " + e.what());
		}
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
		std::unordered_map<string, size_t> mod_index;
		for (size_t i = 0; i < mods.size(); ++i) {
			mod_index[mods[i].name] = i;
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
		ModOptions options;

		sol::object option_obj = lua["option"];
		if (!option_obj.is<sol::table>()) {
			return error(generic_errc::missing_field, "missing required global 'option'");
		}

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

		options.main_scene.path = path_obj.as<string>();
		if (options.main_scene.path.empty()) {
			return error(generic_errc::input_error, "option.scene.main.path cannot be empty");
		}

		loaded_options() = std::move(options);
		return error::no_error;
	}

	error LoadMods(ModCollection& mod_tree, ModLoadProgress* progress) {
		for (const auto& func : PrototypeTypeRegistry::functions) {
			func.clear();
		}
		ClearVirtualFileSpace();
		ClearLocalization();
		loaded_options() = {};

		report_mod_progress(progress, "collecting-sorting-mods", mod_stage_progress(0), "scanning-mod-directories", 0.0f);
		vector<ModInfo> mods;
		auto add_mods_from_dir = [&](const fs::path& dir, bool privileged) {
			if (!fs::exists(dir)) {
				fs::create_directories(dir);
			}
			for (const auto& entry : fs::directory_iterator(dir)) {
				if (entry.is_directory()) {
					auto info_path = entry.path() / "info.yaml";
					if (fs::exists(info_path)) {
						mods.emplace_back(parse_mod_info(info_path.string(), privileged));
					}
				}
			}
		};
		for (const auto& dir : mod_tree.privileged_dirs) {
			add_mods_from_dir(dir, true);
		}
		for (const auto& dir : mod_tree.unprivileged_dirs) {
			add_mods_from_dir(dir, false);
		}

		report_mod_progress(progress, "collecting-sorting-mods", mod_stage_progress(0), "reading-enabled-mods", 0.35f);
		fs::path enabled_mods_path = fs::folder::appdata / "enabled_mods.yaml";
		sync_enabled_mods(enabled_mods_path, mods);
		auto enabled_mods = load_enabled_mods(enabled_mods_path);

		vector<ModInfo> enabled_mod_list;
		for (const auto& mod : mods) {
			auto it = enabled_mods.find(mod.name);
			if (it != enabled_mods.end() && it->second.enabled) {
				enabled_mod_list.push_back(mod);
			}
		}

		report_mod_progress(progress, "collecting-sorting-mods", mod_stage_progress(0), "resolving-dependencies", 0.7f);
		error sort_result = sort_mods_by_dependency(enabled_mod_list);
		if (sort_result) {
			return sort_result;
		}
		report_mod_progress(progress, "collecting-sorting-mods", mod_stage_progress(1), "load-order-ready", 1.0f);

		log_info("[mod-loader] load order:");
		for (const auto& mod : enabled_mod_list) {
			RegisterVirtualRoot(mod.name, mod.location);
			log_info(format("[mod-loader]   - {} (version {})", mod.name,
							version_to_string(mod.mod_version)));
		}

		auto language_result = LoadSelectedLanguageSetting(AppSettingsPath());
		if (!language_result) {
			return language_result.error().add_context("loading settings.yaml");
		}
		string selected_language = *language_result;
		report_mod_progress(progress, "loading-localization", mod_stage_progress(0), selected_language, 0.0f);
		error locale_error = LoadLocaleFiles(span<const ModInfo>(enabled_mod_list.data(), enabled_mod_list.size()), selected_language);
		if (locale_error) {
			return locale_error.add_context("loading locale files");
		}

		{
			log_info("[mod-loader] loading unknown prototypes");
			report_mod_progress(progress, "loading-mod-prototypes", mod_stage_progress(1), "core/null.lua", 0.0f);
			sol::state lua;
			initialize_prototype_lua(lua);

			const ModInfo* core = find_mod(span<const ModInfo>(enabled_mod_list.data(), enabled_mod_list.size()), "core");
			if (!core) {
				return error(generic_errc::input_error, "required core mod was not found");
			}
			if (error err = load_prototype_script(lua, *core, "null.lua", true)) {
				return err;
			}
			report_mod_progress(progress, "loading-mod-prototypes", mod_stage_progress(1), "core/null.lua", 0.2f);
			auto err = LoadDataRaw(lua);
			if (err) {
				return err.add_context("loading null prototypes from core/null.lua");
			}
			report_mod_progress(progress, "loading-mod-prototypes", mod_stage_progress(1), "registered-null-prototypes", 0.35f);
		}

		sol::state lua;
		initialize_prototype_lua(lua);

		log_info("[mod-loader] loading mods");
		for (size_t i = 0; i < enabled_mod_list.size(); ++i) {
			const auto& mod = enabled_mod_list[i];
			f32 progress_base = enabled_mod_list.empty() ? 1.0f : static_cast<f32>(i) / static_cast<f32>(enabled_mod_list.size());
			report_mod_progress(progress, "loading-mod-prototypes", mod_stage_progress(1), mod.name + "/data.lua", 0.35f + progress_base * 0.45f);
			if (error err = load_prototype_script(lua, mod, "data.lua")) {
				return err;
			}
		}

		report_mod_progress(progress, "loading-mod-prototypes", mod_stage_progress(1), "data-lua-files-loaded", 0.8f);
		report_mod_progress(progress, "loading-mod-prototypes", mod_stage_progress(1), "loading-options", 0.85f);
		auto option_err = LoadOptions(lua);
		if (option_err) {
			return option_err.add_context("loading options from mods' data.lua");
		}
		loaded_options().language = selected_language;
		loaded_options().mods = enabled_mod_list;

		report_mod_progress(progress, "loading-mod-prototypes", mod_stage_progress(1), "creating-prototypes", 0.9f);
		auto err = LoadDataRaw(lua);
		if (err) {
			return err.add_context("loading prototypes from mods' data.lua");
		}
		report_mod_progress(progress, "loading-mod-prototypes", mod_stage_progress(2), "prototypes-loaded", 1.0f);

		for (size_t i = 0; i < PrototypeTypeRegistry::functions.size(); ++i) {
			const auto& fn = PrototypeTypeRegistry::functions[i];
			try {
				f32 process_progress = PrototypeTypeRegistry::functions.empty() ? 1.0f : static_cast<f32>(i) / static_cast<f32>(PrototypeTypeRegistry::functions.size());
				report_mod_progress(progress, "linking-prototypes", mod_stage_progress(2), string(fn.type()), process_progress);
				fn.resolve();
			} catch (const lf::exception& e) {
				return error(generic_errc::parse_error, e.what()).add_context("linking prototypes");
			}
		}

		report_mod_progress(progress, "linking-prototypes", mod_stage_progress(3), "prototype-references-linked", 1.0f);
		report_mod_progress(progress, "loading-assets", mod_stage_progress(3), "checking-asset-work", 0.0f);
		report_mod_progress(progress, "loading-assets", mod_stage_progress(4), "assets-loaded", 1.0f);
		report_mod_progress(progress, "finish", mod_stage_progress(4), "finalizing-startup", 0.0f);
		report_mod_progress(progress, "finish", mod_stage_progress(5), "mods-loaded", 1.0f);
		return error::no_error;
	}

	const ModOptions& LoadedModOptions() {
		return loaded_options();
	}

	const vector<ModInfo>& LoadedMods() {
		return loaded_options().mods;
	}

	void UnloadMods() {
		for (const auto& func : PrototypeTypeRegistry::functions) {
			func.clear();
		}
		ClearVirtualFileSpace();
		ClearLocalization();
		loaded_options() = {};
	}
} // namespace lf
