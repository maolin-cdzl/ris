#pragma once

#include <string>
#include <memory>
#include <random>

#include "snapshot/snapshotable.h"

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
	inline size_t region_size() const {
		return m_region_size;
	}

	inline size_t payload_size() const {
		return m_payload_size;
	}

	inline size_t service_size() const {
		return m_service_size;
	}

	snapshot_package_t operator()();
private:
	void generate_region(snapshot_package_t& package);

private:
	size_t				m_region_max;
	size_t				m_payload_max;
	size_t				m_service_max;
	size_t				m_region_size;
	size_t				m_payload_size;
	size_t				m_service_size;

	std::default_random_engine		m_generator;
};

