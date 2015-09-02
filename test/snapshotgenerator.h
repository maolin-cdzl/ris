#pragma once

#include <string>
#include <memory>
#include <random>

#include "snapshot/snapshotable.h"

class EmptySnapshotGenerator : public ISnapshotable {
public:
	virtual snapshot_package_t buildSnapshot();
};

class DuplicateRegionGenerator : public ISnapshotable {
public:
	virtual snapshot_package_t buildSnapshot();
};

class DuplicatePayloadGenerator : public ISnapshotable {
public:
	virtual snapshot_package_t buildSnapshot();
};

class DuplicateServiceGenerator : public ISnapshotable {
public:
	virtual snapshot_package_t buildSnapshot();
};

class UnmatchedRegionGenerator : public ISnapshotable {
public:
	virtual snapshot_package_t buildSnapshot();
};

class UncompletedRegionGenerator : public ISnapshotable {
public:
	virtual snapshot_package_t buildSnapshot();
};

class SnapshotGenerator : public ISnapshotable {
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

	virtual snapshot_package_t buildSnapshot();
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

