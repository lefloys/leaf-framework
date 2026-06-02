#pragma once

#include "format.hpp"
#include "string.hpp"
#include "vector.hpp"

namespace lf {
	string make_missing_field_error_message(string_view field_name);
	string make_type_mismatch_error_message(string_view expected, string_view actual);

	namespace log {
		struct TextSegment {
			string text;
			string field;
			bool annotated = false;
			bool structured_field = false;
		};

		bool IsStructuredFieldKey(string_view field);
		string EscapeAnnotationText(string_view text);
		string Field(string_view field, string_view display);
		string Field(string_view field, const char* display);
		string Field(string_view field, const string& display);
		vector<TextSegment> ParseAnnotatedText(string_view text);
		string PlainText(string_view text);

		template <typename T>
		string Field(string_view field, const T& value) {
			return Field(field, format("{}", value));
		}
	} // namespace log
} // namespace lf
