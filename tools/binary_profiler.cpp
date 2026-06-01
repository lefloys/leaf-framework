#include "leaf/core/binary.hpp"
#include "leaf/core/format.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>

namespace leaf_binary_profiler {
	using clock_type = std::chrono::steady_clock;

	struct ProfilerRecord {
		u32 id = 0;
		i32 x = 0;
		i32 y = 0;
		f32 weight = 0.0f;
	};

	struct ProfilerItem {
		u32 item_id = 0;
		u16 count = 0;
	};

	struct ProfilerEntity {
		u32 id = 0;
		lf::vec2 position{};
		lf::vec2 velocity{};
		i32 health = 0;
		lf::string name;
		lf::vector<ProfilerItem> inventory;
	};

	struct ProfilerPacket {
		u64 tick = 0;
		lf::string shard;
		lf::vector<ProfilerEntity> entities;
	};

	struct SessionPatch {
		u64 tick = 0;
		u32 selected_entity = 0;
		lf::vec2 camera{};
		lf::vector<ProfilerItem> changed_items;
	};

	struct ProfilerSession {
		u64 tick = 0;
		u32 selected_entity = 0;
		lf::vec2 camera{};
		lf::vector<ProfilerItem> changed_items;
	};

	template<typename Value>
	struct TimedResult {
		Value value{};
		double seconds = 0.0;
	};

	template<typename F>
	auto timed(F&& fn) {
		auto begin = clock_type::now();
		auto value = fn();
		auto end = clock_type::now();
		return TimedResult<decltype(value)>{
			.value = std::move(value),
			.seconds = std::chrono::duration<double>(end - begin).count()
		};
	}

	double mib_per_second(size_t bytes, double seconds) {
		if (seconds <= 0.0) {
			return 0.0;
		}
		return (static_cast<double>(bytes) / (1024.0 * 1024.0)) / seconds;
	}

	void print_result(lf::string_view name, size_t bytes, size_t iterations, double seconds) {
		const size_t total_bytes = bytes * iterations;
		std::cout
			<< name << '\n'
			<< "  iterations: " << iterations << '\n'
			<< "  payload:    " << bytes << " bytes\n"
			<< "  total:      " << total_bytes << " bytes\n"
			<< "  time:       " << seconds << " s\n"
			<< "  throughput: " << mib_per_second(total_bytes, seconds) << " MiB/s\n\n";
	}

	ProfilerPacket make_packet(size_t entity_count, size_t inventory_count) {
		ProfilerPacket packet;
		packet.tick = 123456789;
		packet.shard = "binary-profiler";
		packet.entities.reserve(entity_count);
		for (size_t entity_index = 0; entity_index < entity_count; ++entity_index) {
			ProfilerEntity entity;
			entity.id = static_cast<u32>(entity_index + 1u);
			entity.position = lf::vec2{ static_cast<f32>(entity_index) * 0.25f, static_cast<f32>(entity_index) * -0.5f };
			entity.velocity = lf::vec2{ 0.5f, -0.125f };
			entity.health = 100 - static_cast<i32>(entity_index % 73u);
			entity.name = lf::format("entity-{}", entity_index);
			entity.inventory.reserve(inventory_count);
			for (size_t item_index = 0; item_index < inventory_count; ++item_index) {
				entity.inventory.push_back(ProfilerItem{
					.item_id = static_cast<u32>((entity_index * 31u + item_index) % 2048u),
					.count = static_cast<u16>((item_index % 64u) + 1u)
				});
			}
			packet.entities.push_back(std::move(entity));
		}
		return packet;
	}

	lf::vector<ProfilerRecord> make_records(size_t count) {
		lf::vector<ProfilerRecord> records;
		records.reserve(count);
		for (size_t index = 0; index < count; ++index) {
			records.push_back(ProfilerRecord{
				.id = static_cast<u32>(index + 1u),
				.x = static_cast<i32>(index % 4096u),
				.y = -static_cast<i32>((index * 7u) % 4096u),
				.weight = static_cast<f32>(index % 101u) / 10.0f
			});
		}
		return records;
	}

	SessionPatch make_patch(size_t item_count) {
		SessionPatch patch;
		patch.tick = 987654321;
		patch.selected_entity = 42;
		patch.camera = lf::vec2{ 1024.0f, -512.0f };
		patch.changed_items.reserve(item_count);
		for (size_t index = 0; index < item_count; ++index) {
			patch.changed_items.push_back(ProfilerItem{
				.item_id = static_cast<u32>(index + 100u),
				.count = static_cast<u16>((index % 16u) + 1u)
			});
		}
		return patch;
	}

	template<typename T>
	lf::vector<lf::byte> write_or_exit(const T& value, bool fast = false) {
		lf::report<lf::vector<lf::byte>> bytes = fast ? lf::bin::fast_write(value) : lf::bin::write(value);
		if (!bytes) {
			std::cerr << "write failed: " << bytes.error().message << '\n';
			std::exit(1);
		}
		return std::move(*bytes);
	}

	template<typename T>
	T read_or_exit(lf::span<const lf::byte> bytes, bool fast = false) {
		lf::report<T> value = fast ? lf::bin::fast_read<T>(bytes) : lf::bin::read<T>(bytes);
		if (!value) {
			std::cerr << "read failed: " << value.error().message << '\n';
			std::exit(1);
		}
		return std::move(*value);
	}
template<lf::bin::byte_stream Stream, lf::bin::data<ProfilerItem> Item>
lf::error process(Stream& stream, Item& item) {
	return stream(
		lf::bin::field("item_id", item.item_id),
		lf::bin::field("count", item.count)
	);
}

template<lf::bin::byte_stream Stream, lf::bin::data<ProfilerEntity> Entity>
lf::error process(Stream& stream, Entity& entity) {
	return stream(
		lf::bin::field("id", entity.id),
		lf::bin::field("position", entity.position),
		lf::bin::field("velocity", entity.velocity),
		lf::bin::field("health", entity.health),
		lf::bin::field("name", entity.name),
		lf::bin::field("inventory", entity.inventory)
	);
}

template<lf::bin::byte_stream Stream, lf::bin::data<ProfilerPacket> Packet>
lf::error process(Stream& stream, Packet& packet) {
	return stream(
		lf::bin::field("tick", packet.tick),
		lf::bin::field("shard", packet.shard),
		lf::bin::field("entities", packet.entities)
	);
}

template<lf::bin::byte_stream Stream, lf::bin::data<SessionPatch> Patch>
lf::error process(Stream& stream, Patch& patch) {
	return stream(
		lf::bin::field("tick", patch.tick),
		lf::bin::field("selected_entity", patch.selected_entity),
		lf::bin::field("camera", patch.camera),
		lf::bin::field("changed_items", patch.changed_items)
	);
}

template<lf::bin::byte_stream Stream, lf::bin::data<ProfilerSession> Session>
lf::error process(Stream& stream, Session& session) {
	return stream(
		lf::bin::field("tick", session.tick),
		lf::bin::field("selected_entity", session.selected_entity),
		lf::bin::field("camera", session.camera),
		lf::bin::field("changed_items", session.changed_items)
	);
}
} // namespace leaf_binary_profiler

template<>
inline constexpr bool lf::bin::trivially_binary_serializable_v<leaf_binary_profiler::ProfilerRecord> = true;

int main(int argc, char** argv) {
	using namespace leaf_binary_profiler;

	size_t iterations = 1000;
	if (argc > 1) {
		iterations = std::max<size_t>(1, static_cast<size_t>(std::strtoull(argv[1], nullptr, 10)));
	}

	const ProfilerPacket packet = make_packet(1024, 8);
	const lf::vector<ProfilerRecord> records = make_records(1u << 20u);
	const SessionPatch patch = make_patch(128);

	const lf::vector<lf::byte> packet_bytes = write_or_exit(packet);
	const lf::vector<lf::byte> record_bytes = write_or_exit(records, true);
	const lf::vector<lf::byte> patch_bytes = write_or_exit(patch);

	ProfilerPacket packet_check = read_or_exit<ProfilerPacket>(packet_bytes);
	lf::vector<ProfilerRecord> record_check = read_or_exit<lf::vector<ProfilerRecord>>(record_bytes, true);
	SessionPatch patch_check = read_or_exit<SessionPatch>(patch_bytes);
	if (packet_check.entities.size() != packet.entities.size()
		|| record_check.size() != records.size()
		|| patch_check.changed_items.size() != patch.changed_items.size()) {
		std::cerr << "sanity check failed\n";
		return 1;
	}

	std::cout << "Leaf binary profiler\n\n";

	auto structured_write = timed([&] {
		size_t bytes_written = 0;
		for (size_t i = 0; i < iterations; ++i) {
			bytes_written += write_or_exit(packet).size();
		}
		return bytes_written;
	});
	print_result("structured write", packet_bytes.size(), iterations, structured_write.seconds);

	auto structured_read = timed([&] {
		u64 tick_sum = 0;
		for (size_t i = 0; i < iterations; ++i) {
			ProfilerPacket parsed = read_or_exit<ProfilerPacket>(packet_bytes);
			tick_sum += parsed.tick;
		}
		return tick_sum;
	});
	print_result("structured read", packet_bytes.size(), iterations, structured_read.seconds);

	const size_t bulk_iterations = std::max<size_t>(1, iterations / 50u);
	auto bulk_write = timed([&] {
		size_t bytes_written = 0;
		for (size_t i = 0; i < bulk_iterations; ++i) {
			bytes_written += write_or_exit(records, true).size();
		}
		return bytes_written;
	});
	print_result("bulk trivial fast write", record_bytes.size(), bulk_iterations, bulk_write.seconds);

	auto bulk_read = timed([&] {
		u64 id_sum = 0;
		for (size_t i = 0; i < bulk_iterations; ++i) {
			lf::vector<ProfilerRecord> parsed = read_or_exit<lf::vector<ProfilerRecord>>(record_bytes, true);
			id_sum += parsed.empty() ? 0u : parsed.back().id;
		}
		return id_sum;
	});
	print_result("bulk trivial fast read", record_bytes.size(), bulk_iterations, bulk_read.seconds);

	auto patch_apply = timed([&] {
		u64 tick_sum = 0;
		ProfilerSession session;
		for (size_t i = 0; i < iterations * 100u; ++i) {
			lf::bin::read_stream stream(patch_bytes);
			if (lf::error err = stream(lf::bin::field("session", session)); err) {
				std::cerr << "patch apply failed: " << err.message << '\n';
				std::exit(1);
			}
			tick_sum += session.tick;
		}
		return tick_sum;
	});
	print_result("in-place session patch read", patch_bytes.size(), iterations * 100u, patch_apply.seconds);

	return 0;
}
