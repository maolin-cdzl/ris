#pragma once

#include <memory>
#include <string>
#include <list>
#include <czmq.h>


class SnapshotValues {
public:
	typedef std::pair<std::string,std::string>							snapshot_item_value_t;
	typedef std::list<snapshot_item_value_t>							snapshot_item_value_container_t;
	typedef typename snapshot_item_value_container_t::iterator			iterator;
	typedef typename snapshot_item_value_container_t::const_iterator	const_iterator;
public:
	SnapshotValues() = default;
	virtual ~SnapshotValues();

	void addValue(const std::string& name,const std::string& val);


	inline snapshot_item_value_container_t& container() {
		return m_values;
	}

	inline iterator begin() {
		return m_values.begin();
	}
	inline const_iterator begin() const {
		return m_values.begin();
	}

	inline iterator end() {
		return m_values.end();
	}
	inline const_iterator end() const {
		return m_values.end();
	}
protected:
	int packageExtends(zmsg_t* msg);
	int parseValues(zmsg_t* msg);
protected:
	snapshot_item_value_container_t		m_values;
};

class SnapshotItem : public SnapshotValues {
public:
	SnapshotItem(const std::string& type,const std::string& id);

	inline std::string type() const {
		return m_type;
	}
	inline std::string id() const {
		return m_id;
	}

	int package(zmsg_t* msg);

	static bool is(zmsg_t* msg);
	static std::shared_ptr<SnapshotItem> parse(zmsg_t* msg);
private:
	SnapshotItem(const SnapshotItem&);
	SnapshotItem& operator = (const SnapshotItem&);

	const std::string			m_type;
	const std::string			m_id;
};

class SnapshotPartition : public SnapshotValues {
public:
	SnapshotPartition(const std::string& id,uint32_t ver);

	inline std::string id() const {
		return m_id;
	}
	inline uint32_t version() const {
		return m_version;
	}

	void addItem(const std::shared_ptr<SnapshotItem>& item);
	std::shared_ptr<SnapshotItem> popItem();

	int package(zmsg_t* msg);
	int packageBorder(zmsg_t* msg);

	static bool is(zmsg_t* msg);
	static bool isBorder(zmsg_t* msg);
	static std::shared_ptr<SnapshotPartition> parse(zmsg_t* msg);
private:
	SnapshotPartition(const SnapshotPartition&);
	SnapshotPartition& operator = (const SnapshotPartition&);

	const std::string										m_id;
	const uint32_t											m_version;
	std::list<std::shared_ptr<SnapshotItem>>				m_items;
};

class Snapshot : public SnapshotValues {
public:
	Snapshot() = default;

	void addPartition(const std::shared_ptr<SnapshotPartition>& part);
	std::shared_ptr<SnapshotPartition> popPartition();

	int package(zmsg_t* msg);
	int packageBorder(zmsg_t* msg);

	static bool is(zmsg_t* msg);
	static bool isBorder(zmsg_t* msg);
	static std::shared_ptr<Snapshot> parse(zmsg_t* msg);
private:
	Snapshot(const Snapshot&);
	Snapshot& operator = (const Snapshot&);

	std::list<std::shared_ptr<SnapshotPartition>>	m_partitions;
};

