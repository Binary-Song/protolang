#pragma once
#include "entity_system.h"
namespace protolang
{

class OverloadSetIterator;

// 重载函数的集合，请用iterator或range-for访问
class OverloadSet : public IEntity
{
	friend class OverloadSetIterator;
	friend class OverloadSetConstIterator;
	std::vector<IOp *> m_funcs;
	OverloadSet       *m_next = nullptr;

public:
	static constexpr const char* TYPE_NAME = "function";

	explicit OverloadSet(OverloadSet *next)
	    : m_next(next)
	{}
	void   add_func(IOp *func) { m_funcs.push_back(func); }
	void   set_next(OverloadSet *next) { m_next = next; }
	size_t count() const;
	OverloadSetIterator      begin();
	OverloadSetIterator      end();
	OverloadSetConstIterator begin() const;
	OverloadSetConstIterator end() const;
	OverloadSetConstIterator cbegin() const;
	OverloadSetConstIterator cend() const;


private:
	std::string dump_json() override;
};

class OverloadSetIterator
{
private:
	bool                         is_end = false;
	std::vector<IOp *>::iterator m_iter;
	OverloadSet                 *m_curr_set;

public:
	OverloadSetIterator()
	    : is_end(true)
	{}
	OverloadSetIterator(std::vector<IOp *>::iterator iter,
	                    OverloadSet                 *curr);
	OverloadSetIterator &operator++();
	bool  operator==(const OverloadSetIterator &iter) const;
	bool  operator!=(const OverloadSetIterator &iter) const;
	IOp *&operator*() const { return *m_iter; }

private:
	void jump_until_valid();
};
class OverloadSetConstIterator
{
private:
	bool                               is_end = false;
	std::vector<IOp *>::const_iterator m_iter;
	const OverloadSet                 *m_curr_set;

public:
	OverloadSetConstIterator()
	    : is_end(true)
	{}
	OverloadSetConstIterator(
	    std::vector<IOp *>::const_iterator iter,
	    const OverloadSet                 *curr);
	OverloadSetConstIterator &operator++();
	bool operator==(const OverloadSetConstIterator &iter) const;
	bool operator!=(const OverloadSetConstIterator &iter) const;
	IOp *const &operator*() const { return *m_iter; }

private:
	void jump_until_valid();
};
}