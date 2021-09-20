#pragma once
template <typename IteratorT>
class iterator_range {
	IteratorT begin_iterator, end_iterator;

public:
	//TODO: Add SFINAE to test that the Container's iterators match the range's
	//      iterators.
	template <typename Container>
	iterator_range(Container &&c)
		//TODO: Consider ADL/non-member begin/end calls.
		: begin_iterator(c.begin()), end_iterator(c.end()) {}
	iterator_range(IteratorT begin_iterator, IteratorT end_iterator)
		: begin_iterator(std::move(begin_iterator)),
		end_iterator(std::move(end_iterator)) {}

	IteratorT begin() const { return begin_iterator; }
	IteratorT end() const { return end_iterator; }
	bool empty() const { return begin_iterator == end_iterator; }
};