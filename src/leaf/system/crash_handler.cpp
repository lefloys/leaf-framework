#include "crash_handler.hpp"
#include "system.hpp"

#include <csignal>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <typeinfo>

#ifdef _WIN32
#include <windows.h>
#endif

namespace lf {
	static const char* signal_name(int signal) {
		switch (signal) {
		case SIGABRT: return "SIGABRT";
		case SIGFPE: return "SIGFPE";
		case SIGILL: return "SIGILL";
		case SIGINT: return "SIGINT";
		case SIGSEGV: return "SIGSEGV";
		case SIGTERM: return "SIGTERM";
		default: return "unknown signal";
		}
	}

	static void crash_signal_handler(int signal) {
		if (signal == SIGINT || signal == SIGTERM) {
			RequestShutdown();
			return;
		}
		std::cerr << "[crash] fatal signal: " << signal_name(signal) << " (" << signal << ")\n";
		std::cerr.flush();
		std::_Exit(128 + signal);
	}

	static void terminate_handler() {
		std::cerr << "[crash] std::terminate called\n";
		std::exception_ptr exception = std::current_exception();
		if (exception) {
			try {
				std::rethrow_exception(exception);
			} catch (const std::exception& err) {
				std::cerr << "[crash] uncaught exception: " << typeid(err).name() << ": " << err.what() << "\n";
			} catch (...) {
				std::cerr << "[crash] uncaught non-standard exception\n";
			}
		}
		std::cerr.flush();
		std::_Exit(EXIT_FAILURE);
	}

#ifdef _WIN32
	static const char* exception_name(DWORD code) {
		switch (code) {
		case EXCEPTION_ACCESS_VIOLATION: return "EXCEPTION_ACCESS_VIOLATION";
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
		case EXCEPTION_BREAKPOINT: return "EXCEPTION_BREAKPOINT";
		case EXCEPTION_DATATYPE_MISALIGNMENT: return "EXCEPTION_DATATYPE_MISALIGNMENT";
		case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
		case EXCEPTION_FLT_INVALID_OPERATION: return "EXCEPTION_FLT_INVALID_OPERATION";
		case EXCEPTION_ILLEGAL_INSTRUCTION: return "EXCEPTION_ILLEGAL_INSTRUCTION";
		case EXCEPTION_INT_DIVIDE_BY_ZERO: return "EXCEPTION_INT_DIVIDE_BY_ZERO";
		case EXCEPTION_STACK_OVERFLOW: return "EXCEPTION_STACK_OVERFLOW";
		default: return "unknown structured exception";
		}
	}

	static LONG WINAPI structured_exception_handler(EXCEPTION_POINTERS* info) {
		if (!info || !info->ExceptionRecord) {
			std::cerr << "[crash] unhandled structured exception\n";
			std::cerr.flush();
			return EXCEPTION_EXECUTE_HANDLER;
		}

		EXCEPTION_RECORD* record = info->ExceptionRecord;
		const auto module_base = reinterpret_cast<ULONG_PTR>(GetModuleHandleA(nullptr));
		const auto exception_address = reinterpret_cast<ULONG_PTR>(record->ExceptionAddress);
		std::cerr << "[crash] unhandled structured exception: "
				  << exception_name(record->ExceptionCode)
				  << " (0x" << std::hex << record->ExceptionCode << std::dec << ")";
		if (record->ExceptionAddress) {
			std::cerr << " at " << record->ExceptionAddress;
			if (module_base != 0 && exception_address >= module_base) {
				std::cerr << " rva=0x" << std::hex << (exception_address - module_base) << std::dec;
			}
		}
		std::cerr << "\n";
		if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && record->NumberParameters >= 2) {
			const ULONG_PTR operation = record->ExceptionInformation[0];
			const ULONG_PTR address = record->ExceptionInformation[1];
			const char* operation_name = operation == 0 ? "read" : operation == 1 ? "write" : operation == 8 ? "execute" : "access";
			std::cerr << "[crash] access violation while trying to " << operation_name
					  << " address 0x" << std::hex << address << std::dec << "\n";
		}
		std::cerr.flush();
		return EXCEPTION_EXECUTE_HANDLER;
	}
#endif

	void install_crash_handler() {
		std::set_terminate(terminate_handler);
#ifdef _WIN32
		SetUnhandledExceptionFilter(structured_exception_handler);
#endif
		std::signal(SIGABRT, crash_signal_handler);
		std::signal(SIGFPE, crash_signal_handler);
		std::signal(SIGILL, crash_signal_handler);
		std::signal(SIGINT, crash_signal_handler);
		std::signal(SIGSEGV, crash_signal_handler);
		std::signal(SIGTERM, crash_signal_handler);
	}
}
