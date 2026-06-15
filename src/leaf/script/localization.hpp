#pragma once

#include "local_string.hpp"
#include "mod_info.hpp"

#include <leaf/core/error.hpp>
#include <leaf/core/filesystem.hpp>
#include <leaf/core/span.hpp>
#include <leaf/core/string.hpp>
#include <leaf/core/vector.hpp>

namespace lf {
    struct LanguageInfo {
        string id;
        string name;
        string native_name;
    };

    error LoadLocaleFiles(span<const ModInfo> mods, string_view language);
    error ReloadLocaleFiles(string_view language);
    error SetLanguage(string_view language);
    void ClearLocalization();
    string_view LoadedLanguage();
    span<const LanguageInfo> AvailableLanguages();
    string_view AvailableLanguageName(string_view language);
    string_view AvailableLanguageNativeName(string_view language);

    string Localize(string_view section, string_view key);
	string Localize(string_view section, string_view key, span<const string> parameters);
	string Localize(string_view section, const local_string& text);
	string Localize(string_view section, const local_string& text, span<const string> parameters);
} // namespace lf
