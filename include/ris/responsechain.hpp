#pragma once

#include <memory>


template<class Handler>
class ResponseChainHandler : public Handler {
public:
	void setNextHandler(const std::shared_ptr<Handler>& h) {
		m_next = h;
	}

	void unsetNextHandler() {
		m_next.reset();
	}

	inline std::shared_ptr<Handler>& nextHandler() {
		return m_next;
	}

	inline const std::shared_ptr<Handler>& nextHandler() const {
		return m_next;
	}
protected:
	std::shared_ptr<Handler>			m_next;
};

