#pragma once

#include <string>
#include <memory>
#include <random>

#include "snapshot/snapshotable.h"
#include "test_helper.h"

class ISnapshotGeneratorImpl {
public:
	virtual ~ISnapshotGeneratorImpl() { }
	virtual snapshot_package_t build() = 0;
	virtual size_t region_size() const = 0;
	virtual size_t payload_size() const = 0;
	virtual size_t service_size() const = 0;
};


class EmptySnapshotGenerator : public ISnapshotGeneratorImpl {
public:
	virtual snapshot_package_t build();
	virtual size_t region_size() const;
	virtual size_t payload_size() const;
	virtual size_t service_size() const;
};

class DuplicateRegionGenerator : public ISnapshotGeneratorImpl {
public:
	virtual snapshot_package_t build();
	virtual size_t region_size() const;
	virtual size_t payload_size() const;
	virtual size_t service_size() const;
};

class DuplicatePayloadGenerator : public ISnapshotGeneratorImpl {
public:
	virtual snapshot_package_t build();
	virtual size_t region_size() const;
	virtual size_t payload_size() const;
	virtual size_t service_size() const;
};

class DuplicateServiceGenerator : public ISnapshotGeneratorImpl {
public:
	virtual snapshot_package_t build();
	virtual size_t region_size() const;
	virtual size_t payload_size() const;
	virtual size_t service_size() const;
};

class UnmatchedRegionGenerator : public ISnapshotGeneratorImpl {
public:
	virtual snapshot_package_t build();
	virtual size_t region_size() const;
	virtual size_t payload_size() const;
	virtual size_t service_size() const;
};

class UncompletedRegionGenerator : public ISnapshotGeneratorImpl {
public:
	virtual snapshot_package_t build();
	virtual size_t region_size() const;
	virtual size_t payload_size() const;
	virtual size_t service_size() const;
};

class SnapshotGenerator : public ISnapshotGeneratorImpl  {
public:
	SnapshotGenerator(size_t region_max,size_t payload_max,size_t service_max);

	virtual snapshot_package_t build();
	virtual size_t region_size() const;
	virtual size_t payload_size() const;
	virtual size_t service_size() const;

	inline void reset_count() {
		m_region_size = 0;
		m_payload_size = 0;
		m_service_size = 0;
	}

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

testing::Cardinality RegionNumber(const std::shared_ptr<ISnapshotGeneratorImpl>& impl);
testing::Cardinality RegionAtMost(const std::shared_ptr<ISnapshotGeneratorImpl>& impl);

testing::Cardinality ServiceNumber(const std::shared_ptr<ISnapshotGeneratorImpl>& impl);
testing::Cardinality ServiceAtMost(const std::shared_ptr<ISnapshotGeneratorImpl>& impl);

testing::Cardinality PayloadNumber(const std::shared_ptr<ISnapshotGeneratorImpl>& impl);
testing::Cardinality PayloadAtMost(const std::shared_ptr<ISnapshotGeneratorImpl>& impl);



