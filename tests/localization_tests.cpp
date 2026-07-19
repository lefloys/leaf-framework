#include <chrono>
#include <filesystem>
#include <fstream>

#include <catch2/catch_test_macros.hpp>

#include "leaf/script/localization.hpp"

namespace {
	struct locale_fixture {
		std::filesystem::path root;
		lf::vector<lf::ModInfo> mods;

		locale_fixture() {
			const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
			root = std::filesystem::temp_directory_path() / lf::format("leaf-localization-test-{}", stamp);
			const std::filesystem::path locale_dir = root / "base" / "locale";
			std::filesystem::create_directories(locale_dir);

			write_file(locale_dir / "en-US.cfg",
				"[language]\n"
				"name=English\n"
				"native-name=English\n"
				"\n"
				"# comment line\n"
				"[greetings]\n"
				"hello=Hello\n"
				"welcome=Welcome, __1__! You have __2__ new messages.\n"
				"english-only=Only in English\n");

			write_file(locale_dir / "de-DE.cfg",
				"[language]\n"
				"name=German\n"
				"native-name=Deutsch\n"
				"\n"
				"[greetings]\n"
				"hello=Hallo\n"
				"welcome=Willkommen, __1__! Du hast __2__ neue Nachrichten.\n");

			lf::ModInfo mod;
			mod.name = "base";
			mod.location = root / "base";
			mods.push_back(std::move(mod));
		}

		~locale_fixture() {
			lf::ClearLocalization();
			std::error_code ec;
			std::filesystem::remove_all(root, ec);
		}

		static void write_file(const std::filesystem::path& path, lf::string_view contents) {
			std::ofstream file(path, std::ios::binary);
			file << contents;
		}

		lf::span<const lf::ModInfo> mod_span() const {
			return lf::span<const lf::ModInfo>(mods.data(), mods.size());
		}
	};
} // namespace

/// Verifies that unloaded localization falls back to the key.
TEST_CASE("localization falls back to key when unloaded", "[localization]") {
	lf::ClearLocalization();
	REQUIRE(lf::Localize("greetings", "hello") == "hello");
	REQUIRE(lf::Localize("greetings", "").empty());
	REQUIRE(lf::LoadedLanguage() == "en-US");
	REQUIRE(lf::AvailableLanguages().empty());
	REQUIRE(lf::AvailableLanguageName("xx-XX") == "xx-XX");
	REQUIRE(lf::AvailableLanguageNativeName("xx-XX") == "xx-XX");
}

/// Verifies that locale files load and localize keys with parameters.
TEST_CASE("localization loads locale files and applies parameters", "[localization]") {
	locale_fixture fixture;

	lf::error err = lf::LoadLocaleFiles(fixture.mod_span(), "en-US");
	REQUIRE_FALSE(static_cast<bool>(err));
	REQUIRE(lf::LoadedLanguage() == "en-US");
	REQUIRE(lf::Localize("greetings", "hello") == "Hello");
	REQUIRE(lf::Localize("greetings", "missing-key") == "missing-key");

	const lf::vector<lf::string> parameters = { "Ada", "3" };
	REQUIRE(lf::Localize("greetings", "welcome", lf::span<const lf::string>(parameters.data(), parameters.size()))
			== "Welcome, Ada! You have 3 new messages.");

	// local_string overloads route through the same lookup.
	lf::local_string text{ "hello" };
	REQUIRE(lf::Localize("greetings", text) == "Hello");
}

/// Verifies that a non-default language falls back to en-US entries.
TEST_CASE("localization selected language with fallback", "[localization]") {
	locale_fixture fixture;

	lf::error err = lf::LoadLocaleFiles(fixture.mod_span(), "de-DE");
	REQUIRE_FALSE(static_cast<bool>(err));
	REQUIRE(lf::LoadedLanguage() == "de-DE");
	REQUIRE(lf::Localize("greetings", "hello") == "Hallo");

	// Key present only in the fallback language resolves through en-US.
	REQUIRE(lf::Localize("greetings", "english-only") == "Only in English");

	// Language metadata is discovered from the locale directory.
	REQUIRE(lf::AvailableLanguages().size() >= 2);
	REQUIRE(lf::AvailableLanguageName("de-DE") == "German");
	REQUIRE(lf::AvailableLanguageNativeName("de-DE") == "Deutsch");
	REQUIRE(lf::AvailableLanguageName("en-US") == "English");
}

/// Verifies that ClearLocalization resets state to defaults.
TEST_CASE("localization clear resets state", "[localization]") {
	locale_fixture fixture;
	REQUIRE_FALSE(static_cast<bool>(lf::LoadLocaleFiles(fixture.mod_span(), "de-DE")));
	REQUIRE(lf::Localize("greetings", "hello") == "Hallo");

	lf::ClearLocalization();
	REQUIRE(lf::LoadedLanguage() == "en-US");
	REQUIRE(lf::Localize("greetings", "hello") == "hello");
	REQUIRE(lf::AvailableLanguages().empty());
}
