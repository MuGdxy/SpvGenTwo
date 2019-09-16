#pragma once

#include "EntryIterator.h"

namespace spvgentwo
{
	// forward decls
	class IAllocator;

	template <class T>
	class List
	{
	public:
		using Iterator = EntryIterator<T>;
		using EntryType = Entry<T>;
		using ValueType = T;
		using ReferenceType = T &;
		using PointerType = T *;

		List(IAllocator* _pAllocator);
		List(const List& _other);
		List(List&& _other);
		virtual ~List();

		List& operator=(const List& _other);
		List& operator=(List&& _other);

		bool operator==(const List<T>&  other) const;
		bool operator!=(const List<T>& _other) const { return !operator==(_other); }

		IAllocator* getAllocator() { return m_pAllocator; }
		const IAllocator* getAllocator() const { return m_pAllocator; }

		void clear(); // destroy entries

		template<class ...Args>
		Entry<T>* emplace_front_entry(Args&& ..._args);

		template<class ...Args>
		T& emplace_front(Args&& ..._args);

		template<class ...Args>
		Entry<T>* emplace_back_entry(Args&& ..._args);

		Entry<T>* prepend_entry(Entry<T>* _entry);

		// adds _entry to the end of the chain, returns same entry
		Entry<T>* append_entry(Entry<T>* _entry);

		// emplace at the end of the linked list
		template<class ...Args>
		T& emplace_back(Args&& ..._args);

		// insert new entry before this entry
		template<class ...Args>
		Entry<T>* insert(Iterator _pos, Args&& ..._args);

		// removes element at pos from list, returns next element
		Entry<T>* erase(Iterator _pos, const bool _destruct = true);

		Iterator begin() const { return Iterator(m_pBegin); }
		Iterator end() const { return Iterator(nullptr); }

		T& front() { return m_pBegin->inner(); }
		const T& front() const { return m_pBegin->inner(); }

		T& back() { return m_pLast->inner(); }
		const T& back() const { return m_pLast->inner(); }

		Entry<T>* last() { return m_pLast; }
		const Entry<T>* last() const { return m_pLast; }

		bool empty() const { return m_pBegin != nullptr; }

		size_t size() const { return m_Elements; }

	protected:
		IAllocator* m_pAllocator = nullptr;
		Entry<T>* m_pBegin = nullptr;
		Entry<T>* m_pLast = nullptr;
		size_t m_Elements = 0u;
	};

	template<class T>
	inline List<T>::List(IAllocator* _pAllocator) : m_pAllocator(_pAllocator)
	{
	}

	template<class T>
	inline List<T>::List(const List& _other) :
		m_pAllocator(_other.m_pAllocator)
	{
		for (const T& e : _other)
		{
			emplace_back(e);
		}
	}

	template<class T>
	inline List<T>::List(List&& _other) :
		m_pAllocator(_other.m_pAllocator),
		m_pBegin(_other.m_pBegin),
		m_pLast(_other.m_pLast),
		m_Elements(_other.m_Elements)
	{
		_other.m_pAllocator = nullptr;
		_other.m_pBegin = nullptr;
		_other.m_pLast = nullptr;
		_other.m_Elements = 0u;
	}

	template<class T>
	inline List<T>::~List()
	{
		clear();
		m_pAllocator = nullptr;
	}

	template<class T>
	inline void List<T>::clear()
	{
		if (m_pBegin != nullptr && m_pAllocator != nullptr)
		{
			m_pBegin->destroyList(m_pAllocator);
			m_pBegin = nullptr;
		}
	}

	template<class T>
	inline List<T>& List<T>::operator=(const List<T>& _other)
	{
		if (this == &_other) return *this;

		Iterator l = begin(), le = end(), r = _other.begin(), re = _other.end();
		for (; l != le && r != re; ++l, ++r)
		{
			(*l) = (*r);
		}

		// left side was longer, destroy rest
		if(l != le)
		{
			l.entry()->destroyList(m_pAllocator);
		}

		// right side was longer, emplace
		for (; r != re; ++r)
		{
			emplace_back(*r);
		}

		m_Elements = _other.m_Elements;
		return *this;
	}

	template<class T>
	inline List<T>& List<T>::operator=(List<T>&& _other)
	{
		if (this == &_other) return *this;

		// destroy left side
		if (m_pBegin != nullptr && m_pAllocator != nullptr)
		{
			m_pBegin->destroyList(m_pAllocator);
		}

		// assign right side
		m_pAllocator = _other.m_pAllocator;
		m_pBegin = _other.m_pBegin;
		m_pLast = _other.m_pLast;
		m_Elements = _other.m_Elements;

		_other.m_pAllocator = nullptr;
		_other.m_pBegin = nullptr;
		_other.m_pLast = nullptr;
		_other.m_Elements = 0u;

		return *this;
	}

	template <class T>
	bool List<T>::operator==(const List<T>& _other) const
	{
		Iterator l = begin(), le = end(), r = _other.begin(), re = _other.end();
		for (; l != le && r != re; ++l, ++r)
		{
			if ((*l) != (*r)) return false;
		}

		// check if both list are at the end (same length)
		return l == le && r == re;
	}

	template<class T>
	inline Entry<T>* List<T>::prepend_entry(Entry<T>* _entry)
	{
		if (m_pBegin == nullptr)
		{
			m_pBegin = _entry;
			m_pLast = m_pBegin;
		}
		else
		{
			m_pBegin = m_pBegin->prepend(_entry);
		}
		++m_Elements;
		return m_pBegin;
	}

	template<class T>
	inline Entry<T>* List<T>::append_entry(Entry<T>* _entry)
	{
		if (m_pBegin == nullptr)
		{
			m_pBegin = _entry;
			m_pLast = m_pBegin;
		}
		else
		{
			m_pLast = m_pLast->append(_entry);
		}
		++m_Elements;
		return m_pLast;
	}

	template<class T>
	template<class ...Args>
	inline Entry<T>* List<T>::emplace_front_entry(Args&& ..._args)
	{
		return prepend_entry(Entry<T>::create(m_pAllocator, std::forward<Args>(_args)...));
	}

	template<class T>
	template<class ...Args>
	inline Entry<T>* List<T>::emplace_back_entry(Args&& ..._args)
	{
		return append_entry(Entry<T>::create(m_pAllocator, std::forward<Args>(_args)...));
	}

	template<class T>
	template<class ...Args>
	inline T& List<T>::emplace_front(Args&& ..._args)
	{
		return **emplace_front_entry(std::forward<Args>(_args)...);
	}

	template<class T>
	template<class ...Args>
	inline T& List<T>::emplace_back(Args&& ..._args)
	{
		return **emplace_back_entry(std::forward<Args>(_args)...);
	}

	template<class T>
	template<class ...Args>
	inline Entry<T>* List<T>::insert(Iterator _pos, Args&& ..._args)
	{
		++m_Elements;
		return _pos.entry()->insert(m_pAllocator, std::forward<Args>(_args)...);
	}

	template<class T>
	inline Entry<T>* List<T>::erase(Iterator _pos, const bool _destruct)
	{
		--m_Elements;
		return _pos.entry()->remove(_destruct ? m_pAllocator : nullptr);
	}
} // !spvgentwo