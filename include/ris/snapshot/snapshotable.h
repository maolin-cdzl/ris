#pragma once

#include <memory>
#include <string>
#include <list>
#include <czmq.h>

class SnapshotItem {
public:
	SnapshotItem(const std::string& id);
	~SnapshotItem();

	int addData(const void* src,size_t size);
	int addString(const char* str);

	int send(zsock_t* sock);
private:
	SnapshotItem(const SnapshotItem&);
	SnapshotItem& operator = (const SnapshotItem&);

	zmsg_t*							m_msg;
};

class SnapshotPartition {
public:
	SnapshotPartition(const std::string& id,uint32_t ver);
	~SnapshotPartition();

	int addData(const void* src,size_t size);
	int addString(const char* str);

	void addItem(const std::shared_ptr<SnapshotItem>& item);
	std::shared_ptr<SnapshotItem> popItem();

	int sendHeader(zsock_t* sock);
	int sendBorder(zsock_t* sock);
private:
	SnapshotPartition(const SnapshotPartition&);
	SnapshotPartition& operator = (const SnapshotPartition&);

	zmsg_t*													m_info;
	std::list<std::shared_ptr<SnapshotItem>>				m_items;
};

class Snapshot {
public:
	Snapshot();
	~Snapshot();

	int addData(const void* src,size_t size);
	int addString(const char* str);

	void addPartition(const std::shared_ptr<SnapshotPartition>& part);
	std::shared_ptr<SnapshotPartition> popPartition();

	int sendHeader(zsock_t* sock);
	int sendBorder(zsock_t* sock);
private:
	Snapshot(const Snapshot&);
	Snapshot& operator = (const Snapshot&);

	zmsg_t*											m_info;
	std::list<std::shared_ptr<SnapshotPartition>>	m_partitions;
};


class ISnapshotable {
public:
	virtual std::shared_ptr<Snapshot> buildSnapshot() = 0;
};
