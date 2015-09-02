#pragma once

#include <list>
#include <memory>
#include <google/protobuf/message.h>

typedef std::list<std::shared_ptr<google::protobuf::Message>>	snapshot_package_t;

class ISnapshotable {
public:
	virtual snapshot_package_t buildSnapshot() = 0;
};
