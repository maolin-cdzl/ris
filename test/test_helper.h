#pragma once

#include <memory>
#include <czmq.h>
#include "gmock/gmock-cardinalities.h"
#include "snapshot/snapshotclient.h"
#include "snapshot/snapshotbuilder.h"
#include "zmqx/zlooptimer.h"

std::string newUUID();

class LoopStoper {
public:
	LoopStoper(zloop_t* loop,long timeout);
	~LoopStoper();

	void cancel();
private:
	static int onTimer(zloop_t* loop,int timeid,void* arg);
private:
	zloop_t*	m_loop;
	int			m_tid;
};

class ReadableHelper {
public:
	ReadableHelper(zloop_t* loop);
	~ReadableHelper();

	inline zmsg_t* message() {
		return m_msg;
	}

	int register_read(zsock_t* sock);
	int register_read_int(zsock_t* sock);

	int unregister();
private:
	static int readAndInterrupt(zloop_t* loop,zsock_t* reader,void* arg);
	static int readAndGo(zloop_t* loop,zsock_t* reader,void* arg);

private:
	zloop_t*						m_loop;
	zsock_t*						m_sock;
	zmsg_t*							m_msg;
};

class CompleteResultHelper {
public:
	CompleteResultHelper();
	void onComplete(int err);
	
	inline int result() const {
		return m_result;
	}
private:
	int								m_result;
};

class TaskRunner {
public:
	typedef std::function<void()>			task_t;
	TaskRunner(zloop_t* loop);
	~TaskRunner();

	void push(const task_t& task);

	int start(uint64_t interval);
	void stop();
private:
	int onTimer();
private:
	std::list<task_t>						m_tasks;
	ZLoopTimer								m_timer;
};


class SnapshotClientRepeater {
public:
	SnapshotClientRepeater(zloop_t* loop);

	int start(size_t count,const std::shared_ptr<ISnapshotBuilder>& builder,const std::string& address);

	inline size_t limits() const {
		return m_limits;
	}
	inline size_t finish_count() const {
		return m_count;
	}
	inline size_t success_count() const {
		return m_success_count;
	}
private:
	void onComplete(int err);
private:
	std::shared_ptr<SnapshotClient>				m_client;
	std::shared_ptr<ISnapshotBuilder>			m_builder;
	std::string									m_address;
	size_t										m_limits;
	size_t										m_count;
	size_t										m_success_count;
};

class SnapshotClientParallelRepeater {
public:
	SnapshotClientParallelRepeater(zloop_t* loop); 
	int start(size_t count,const std::shared_ptr<ISnapshotBuilder>& builder,const std::string& address);

	inline size_t limits() const {
		return m_limits;
	}
	inline size_t finish_count() const {
		return m_count;
	}
	inline size_t success_count() const {
		return m_success_count;
	}
private:
	void onComplete(int err);
private:
	zloop_t*									m_loop;
	std::list<std::shared_ptr<SnapshotClient>>	m_clients;
	std::shared_ptr<ISnapshotBuilder>			m_builder;
	std::string									m_address;
	size_t										m_limits;
	size_t										m_count;
	size_t										m_success_count;
};

class InvokeCardinality : public testing::CardinalityInterface {
public:
	InvokeCardinality(const std::function<size_t()>& min,const std::function<size_t()>& max);
	virtual ~InvokeCardinality();

	virtual int ConservativeLowerBound() const;
	virtual int ConservativeUpperBound() const;

	// Returns true iff call_count calls will satisfy this cardinality.
	virtual bool IsSatisfiedByCallCount(int call_count) const;

	// Returns true iff call_count calls will saturate this cardinality.
	virtual bool IsSaturatedByCallCount(int call_count) const;

	// Describes self to an ostream.
	virtual void DescribeTo(::std::ostream* os) const;

public:
	static testing::Cardinality makeAtLeast(const std::function<size_t()>& min);
	static testing::Cardinality makeAtMost(const std::function<size_t()>& max);
	static testing::Cardinality makeBetween(const std::function<size_t()>& min,const std::function<size_t()>& max);
private:
	static size_t Zero();
	static size_t Unlimited();
private:
	std::function<size_t()>						m_fn_min;
	std::function<size_t()>						m_fn_max;
};



