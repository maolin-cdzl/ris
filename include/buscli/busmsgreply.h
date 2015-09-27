#pragma once

#include <string>
#include <set>
#include <list>
#include <czmq.h>
#include "ris/bus.pb.h"

struct BusMsgReplyItem {
	std::set<std::string>		payloads;
	zmsg_t*						msg;
};

class BusMsgReply {
public:
	typedef std::list<BusMsgReplyItem>::const_iterator	const_iterator;
public:
	BusMsgReply(uint64_t msg_id,const std::set<std::string>& payloads);
	~BusMsgReply();

	inline const std::set<std::string>& failures() const {
		return m_fails;
	}

	inline size_t failure_size() const {
		return m_fails.size();
	}

	inline const_iterator begin() const {
		return m_replys.begin();
	}
	inline const_iterator end() const {
		return m_replys.end();
	}

	zmsg_t* reply(const std::string& payload) const;

	void addReply(const bus::ReplyMessage& reply,zmsg_t** p_msg);
	void addFailure(const bus::Failure& fails);

	bool isCompleted() const {
		return m_count == m_expects.size();
	}
private:
	BusMsgReply(const BusMsgReply&) = delete;
	BusMsgReply& operator = (const BusMsgReply&) = delete;
private:
	const uint64_t						m_msg_id;
	const std::set<std::string>			m_expects;
	size_t								m_count;
	std::set<std::string>				m_fails;
	std::list<BusMsgReplyItem>		m_replys;
};

