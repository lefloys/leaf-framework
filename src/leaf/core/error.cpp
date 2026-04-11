#include "error.hpp"

namespace lf {
    const error error::no_error = error();
	const error error::unknown_error = error(generic_errc::unknown    );
    const char* generic_error_category::name() const noexcept {
        return "generic";
    }

    std::string generic_error_category::message(i32 ev) const {
        switch (static_cast<generic_errc>(ev)) {
            case generic_errc::unknown: return "Unknown error";
            default: return "Unrecognized error code";
        }
    }

    const error_category& generic_category() {
        static generic_error_category instance;
        return instance;
    }

    error_code make_error_code(generic_errc e) { return { static_cast<i32>(e), generic_category() }; }


    error::operator bool() const noexcept { return code.value() != 0; }
    error& error::add_context(std::string_view context) {
        if (context.empty()) { return *this; }
        message = std::format("{}\n -> {}", context, message);
        return *this;
	}

}