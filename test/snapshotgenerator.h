#pragma once

#include <string>
#include <memory>
#include <random>

#include "snapshot/snapshotable.h"
#include "test_helper.h"

class EmptySnapshotGenerator {
public:
	snapshot_package_t operator()();
};

class DuplicateRegionGenerator {
public:
	snapshot_package_t operator()();
};

class DuplicatePayloadGenerator {
public:
	snapshot_package_t operator()();
};

class DuplicateServiceGenerator {
public:
	snapshot_package_t operator()();
};

class UnmatchedRegionGenerator {
public:
	snapshot_package_t operator()();
};

class UncompletedRegionGenerator {
public:
	snapshot_package_t operator()();
};

class SnapshotGenerator {
public:
	SnapshotGenerator(size_t region_max,size_t payload_max,size_t service_max);

	snapshot_package_t operator()();

	static inline size_t region_size() {
		return s_region_size;
	}

	static inline size_t payload_size() {
		return s_payload_size;
	}

	static inline size_t service_size() {
		return s_service_size;
	}

	static inline void reset_count() {
		s_region_size = 0;
		s_payload_size = 0;
		s_service_size = 0;
	}
private:
	void generate_region(snapshot_package_t& package);

private:
	size_t				m_region_max;
	size_t				m_payload_max;
	size_t				m_service_max;
private:
	static size_t		s_region_size;
	static size_t		s_payload_size;
	static size_t		s_service_size;

	std::default_random_engine		m_generator;
};

testing::Cardinality RegionCardinality();
testing::Cardinality ServiceCardinality();
testing::Cardinality PayloadCardinality();



