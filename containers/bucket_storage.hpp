#ifndef BUCKET_STORAGE_HPP
#define BUCKET_STORAGE_HPP

#include <compare>
#include <new>
#include <stdexcept>
#include <utility>

template< typename T >
class Block
{
  private:
	size_t block_elements_counter;
	size_t capacity;
	Block &operator=(const Block< T > &other);
	Block &operator=(Block< T > &&other) noexcept;

  public:
	struct Element
	{
		T element_data;
		Element *prev_element;
		Element *next_element;

		Element(const T &data, Element *prev, Element *next);

		Element(T &&data, Element *prev, Element *next);
	};

	Element *head_element;
	Element *tail_element;
	Block *prev_block;
	Block *next_block;

	explicit Block(size_t cap);
	~Block();
	[[nodiscard]] bool is_full() const;
	[[nodiscard]] bool is_empty() const noexcept;
	template< typename U >
	void insert_element_general(U &&value);
	void remove_element(Element *element);
	void clear();
};

template< typename T >
Block< T >::Element::Element(const T &data, Element *prev, Element *next) :
	element_data(data), prev_element(prev), next_element(next)
{
}

template< typename T >
Block< T >::Element::Element(T &&data, Element *prev, Element *next) :
	element_data(std::move(data)), prev_element(prev), next_element(next)
{
}

template< typename T >
Block< T >::Block(size_t cap) :
	head_element(nullptr), tail_element(nullptr), prev_block(nullptr), next_block(nullptr), block_elements_counter(0),
	capacity(cap)
{
	if (cap == 0)
	{
		throw std::invalid_argument("Block capacity must be greater than 0");
	}
}

template< typename T >
Block< T > &Block< T >::operator=(const Block< T > &other)
{
	if (this != &other)
	{
		Block temp(std::move(other));
		std::swap(temp);
	}
	return *this;
}

template< typename T >
Block< T > &Block< T >::operator=(Block< T > &&other) noexcept
{
	if (this == &other)
		return *this;

	std::swap(this, other);
	return *this;
}

template< typename T >
Block< T >::~Block()
{
	clear();
}

template< typename T >
bool Block< T >::is_full() const
{
	return capacity == block_elements_counter;
}

template< typename T >
bool Block< T >::is_empty() const noexcept
{
	return block_elements_counter == 0;
}

template< typename T >
template< typename U >
void Block< T >::insert_element_general(U &&value)
{
	auto *new_element = new Element{ std::forward< U >(value), nullptr, nullptr };
	if (!new_element)
	{
		throw std::bad_alloc();
	}
	if (!head_element)
	{
		head_element = tail_element = new_element;
	}
	else
	{
		tail_element->next_element = new_element;
		new_element->prev_element = tail_element;
		tail_element = new_element;
	}
	++block_elements_counter;

	if (block_elements_counter > capacity)
	{
		throw std::logic_error("Block elements counter exceeded capacity");
	}
}

template< typename T >
void Block< T >::remove_element(Element *element)
{
	if (!element)
	{
		throw std::invalid_argument("Cannot remove a null element");
	}
	if (this->is_empty())
	{
		throw std::underflow_error("Cannot remove from an empty block");
	}

	if (element == head_element)
		head_element = element->next_element;
	if (element == tail_element)
		tail_element = element->prev_element;
	if (element->prev_element)
		element->prev_element->next_element = element->next_element;
	if (element->next_element)
		element->next_element->prev_element = element->prev_element;

	delete element;
	--block_elements_counter;
}

template< typename T >
void Block< T >::clear()
{
	if (this->is_empty())
	{
		return;
	}
	Element *current = head_element;
	while (current)
	{
		Element *next = current->next_element;
		delete current;
		current = next;
	}
	head_element = tail_element = nullptr;
	block_elements_counter = 0;
}

template< typename T >
class LinkedStack
{
  public:
	LinkedStack();
	LinkedStack(const LinkedStack< T > &other);
	~LinkedStack();
	void push(Block< T > *block);
	Block< T > *top() const;
	void void_pop();
	void get_rid_of(Block< T > *block);
	void clear();
	[[nodiscard]] bool empty() const;

  private:
	LinkedStack< T > &operator=(const LinkedStack< T > &other);
	LinkedStack< T > &operator=(LinkedStack< T > &&other) noexcept;
	struct Node
	{
		Block< T > *block;
		Node *next;
		Node *prev;

		explicit Node(Block< T > *b);
	};

	Node *head;
	Node *tail;
	size_t stack_size;
};

template< typename T >
LinkedStack< T >::Node::Node(Block< T > *b) : block(b), next(nullptr), prev(nullptr)
{
}

template< typename T >
LinkedStack< T >::LinkedStack() : head(nullptr), tail(nullptr), stack_size(0)
{
}

template< typename T >
LinkedStack< T > &LinkedStack< T >::operator=(const LinkedStack< T > &other)
{
	if (this == &other)
		return *this;

	clear();
	Node *current = other.head;
	while (current)
	{
		push(current->block);
		current = current->next;
	}
	return *this;
}

template< typename T >
LinkedStack< T > &LinkedStack< T >::operator=(LinkedStack< T > &&other) noexcept
{
	if (this == &other)
		return *this;

	clear();
	head = other.head;
	tail = other.tail;
	stack_size = other.stack_size;

	other.head = nullptr;
	other.tail = nullptr;
	other.stack_size = 0;

	return *this;
}

template< typename T >
void LinkedStack< T >::clear()
{
	while (head)
	{
		Node *tmp = head;
		head = head->next;
		delete tmp;
	}
	head = nullptr;
	tail = nullptr;
	stack_size = 0;
}

template< typename T >
LinkedStack< T >::~LinkedStack()
{
	clear();
}

template< typename T >
void LinkedStack< T >::push(Block< T > *block)
{
	Node *new_node = new Node(block);
	if (this->empty())
	{
		head = tail = new_node;
	}
	else
	{
		new_node->next = head;
		head->prev = new_node;
		head = new_node;
	}
	++stack_size;
}

template< typename T >
Block< T > *LinkedStack< T >::top() const
{
	if (!head)
		return nullptr;
	return head->block;
}

template< typename T >
void LinkedStack< T >::void_pop()
{
	if (this->empty())
	{
		throw std::out_of_range("Stack is empty. Cannot pop.");
	}
	Node *temp = head;
	if (head == tail)
	{
		head = tail = nullptr;
	}
	else
	{
		head = head->next;
		head->prev = nullptr;
	}
	delete temp;
	--stack_size;
}

template< typename T >
void LinkedStack< T >::get_rid_of(Block< T > *block)
{
	if (!block)
	{
		throw std::invalid_argument("Block cannot be null");
	}
	Node *current = head;
	while (current && current->block != block)
	{
		current = current->next;
	}

	if (!current)
		return;

	if (current == head)
		head = current->next;
	if (current == tail)
		tail = current->prev;
	if (current->prev)
		current->prev->next = current->next;
	if (current->next)
		current->next->prev = current->prev;
	delete current;
	--stack_size;
}

template< typename T >
bool LinkedStack< T >::empty() const
{
	return stack_size == 0;
}

template< typename T >
class BucketStorageConstIterator;

template< typename T >
class BucketStorageIterator
{
  public:
	static constexpr int POSITION_BEFORE = -1;
	static constexpr int POSITION_AFTER = 1;
	static constexpr int POSITION_EQUAL = 0;
	using value_type = T;
	using reference = T &;
	using const_reference = const T &;
	using pointer = T *;
	using const_pointer = const T *;
	using difference_type = std::ptrdiff_t;
	using iterator_category = std::bidirectional_iterator_tag;

	BucketStorageIterator(Block< T > *block, typename Block< T >::Element *element);

	reference operator*() const;

	pointer operator->() const;

	BucketStorageIterator &operator++();

	BucketStorageIterator operator++(int);

	BucketStorageIterator &operator--();

	BucketStorageIterator operator--(int);

	bool operator==(const BucketStorageIterator &other) const;

	bool operator!=(const BucketStorageIterator &other) const;

	bool operator<(const BucketStorageIterator &other) const;

	bool operator<=(const BucketStorageIterator &other) const;

	bool operator>(const BucketStorageIterator &other) const;

	bool operator>=(const BucketStorageIterator &other) const;

	operator BucketStorageConstIterator< T >() const;

	Block< T > *current_block;
	typename Block< T >::Element *current_element;

	int compare_position(const BucketStorageIterator &other) const;
};

template< typename T >
BucketStorageIterator< T >::BucketStorageIterator(Block< T > *block, typename Block< T >::Element *element) :
	current_block(block), current_element(element)
{
}

template< typename T >
typename BucketStorageIterator< T >::reference BucketStorageIterator< T >::operator*() const
{
	if (!current_block->next_block && !current_element)
		throw std::out_of_range("Attempted to dereference end() iterator.");

	return current_element->element_data;
}

template< typename T >
typename BucketStorageIterator< T >::pointer BucketStorageIterator< T >::operator->() const
{
	if (!current_block->next_block && !current_element)
		throw std::out_of_range("Attempted to dereference end() iterator.");

	return &(current_element->element_data);
}

template< typename T >
BucketStorageIterator< T > &BucketStorageIterator< T >::operator++()
{
	if (!current_block || !current_element)
	{
		throw std::out_of_range("Iterator cannot be incremented.");
	}
	if (current_element && current_element->next_element)
	{
		current_element = current_element->next_element;
	}
	else if (current_block && current_block->next_block)
	{
		current_block = current_block->next_block;
		current_element = current_block->head_element;
	}
	else
	{
		current_element = nullptr;
	}
	return *this;
}

template< typename T >
BucketStorageIterator< T > BucketStorageIterator< T >::operator++(int)
{
	BucketStorageIterator tmp = *this;
	++(*this);
	return tmp;
}

template< typename T >
BucketStorageIterator< T > &BucketStorageIterator< T >::operator--()
{
	if (!current_block || (!current_block->prev_block && !current_element))
	{
		throw std::out_of_range("Iterator cannot be decremented");
	}
	if (current_block && !current_block->next_block && !current_element)
	{
		current_element = current_block->tail_element;
	}
	else if (current_element && current_element->prev_element)
	{
		current_element = current_element->prev_element;
	}
	else if (current_block->prev_block)
	{
		current_block = current_block->prev_block;
		current_element = current_block->tail_element;
	}
	else
	{
		current_block = nullptr;
		current_element = nullptr;
	}
	return *this;
}

template< typename T >
BucketStorageIterator< T > BucketStorageIterator< T >::operator--(int)
{
	BucketStorageIterator tmp = *this;
	--(*this);
	return tmp;
}

template< typename T >
bool BucketStorageIterator< T >::operator==(const BucketStorageIterator &other) const
{
	return current_block == other.current_block && current_element == other.current_element;
}

template< typename T >
bool BucketStorageIterator< T >::operator!=(const BucketStorageIterator &other) const
{
	return !(*this == other);
}

template< typename T >
bool BucketStorageIterator< T >::operator<(const BucketStorageIterator &other) const
{
	return compare_position(other) < 0;
}

template< typename T >
bool BucketStorageIterator< T >::operator<=(const BucketStorageIterator &other) const
{
	return compare_position(other) <= 0;
}

template< typename T >
bool BucketStorageIterator< T >::operator>(const BucketStorageIterator &other) const
{
	return compare_position(other) > 0;
}

template< typename T >
bool BucketStorageIterator< T >::operator>=(const BucketStorageIterator &other) const
{
	return compare_position(other) >= 0;
}

template< typename T >
BucketStorageIterator< T >::operator BucketStorageConstIterator< T >() const
{
	return BucketStorageConstIterator< T >(this->current_block, this->current_element);
}

template< typename T >
int BucketStorageIterator< T >::compare_position(const BucketStorageIterator &other) const
{
	if (!current_block || !other.current_block)
	{
		throw std::runtime_error("Cannot compare iterators with null blocks.");
	}

	Block< T > *this_block = current_block;
	Block< T > *other_block = other.current_block;
	typename Block< T >::Element *this_elem = current_element;
	typename Block< T >::Element *other_elem = other.current_element;

	if (this_block == other_block && this_elem == other_elem)
		return POSITION_EQUAL;
	else if (this_block == other_block)
	{
		typename Block< T >::Element *temp_elem = this_elem;
		while (temp_elem != nullptr && temp_elem != this_block->head_element)
		{
			if (temp_elem == other_elem)
			{
				return POSITION_BEFORE;
			}
			temp_elem = temp_elem->prev_element;
		}
		temp_elem = this_elem;
		while (temp_elem != nullptr && temp_elem != this_block->tail_element)
		{
			if (temp_elem == other_elem)
			{
				return POSITION_AFTER;
			}
			temp_elem = temp_elem->next_element;
		}

		throw std::runtime_error("Cannot find the element in the block.");
	}
	else
	{
		Block< T > *temp_block = this_block;
		while (temp_block != nullptr)
		{
			if (temp_block == other_block)
			{
				return POSITION_BEFORE;
			}
			temp_block = temp_block->prev_block;
		}
		temp_block = other_block;
		while (temp_block != nullptr)
		{
			if (temp_block == this_block)
			{
				return POSITION_AFTER;
			}
			temp_block = temp_block->prev_block;
		}
		throw std::runtime_error("Cannot determine relative block positions");
	}
}

template< typename T >
class BucketStorageConstIterator : public BucketStorageIterator< T >
{
  public:
	using BucketStorageIterator< T >::BucketStorageIterator;
	using value_type = T;
	using reference = const T &;
	using pointer = const T *;
	using iterator_category = std::bidirectional_iterator_tag;

	BucketStorageConstIterator &operator++();

	BucketStorageConstIterator operator++(int);

	BucketStorageConstIterator &operator--();

	BucketStorageConstIterator operator--(int);

	reference operator*() const;

	pointer operator->() const;
};

template< typename T >
BucketStorageConstIterator< T > &BucketStorageConstIterator< T >::operator++()
{
	BucketStorageIterator< T >::operator++();
	return *this;
}

template< typename T >
BucketStorageConstIterator< T > BucketStorageConstIterator< T >::operator++(int)
{
	BucketStorageConstIterator tmp = *this;
	BucketStorageIterator< T >::operator++();
	return tmp;
}

template< typename T >
BucketStorageConstIterator< T > &BucketStorageConstIterator< T >::operator--()
{
	BucketStorageIterator< T >::operator--();
	return *this;
}

template< typename T >
BucketStorageConstIterator< T > BucketStorageConstIterator< T >::operator--(int)
{
	BucketStorageConstIterator tmp = *this;
	BucketStorageIterator< T >::operator--();
	return tmp;
}

template< typename T >
typename BucketStorageConstIterator< T >::reference BucketStorageConstIterator< T >::operator*() const
{
	if (!this->current_block->next_block && !this->current_element)
		throw std::out_of_range("Attempted to dereference end() iterator");

	return this->current_element->element_data;
}

template< typename T >
typename BucketStorageConstIterator< T >::pointer BucketStorageConstIterator< T >::operator->() const
{
	if (!this->current_block->next_block && !this->current_element)
		throw std::out_of_range("Attempted to dereference end() iterator");

	return &(this->current_element->element_data);
}

template< typename T >
class BucketStorage
{
	friend class BucketStorageIterator< T >;

  public:
	using value_type = T;
	using reference = T &;
	using const_reference = const T &;
	using difference_type = std::ptrdiff_t;
	using iterator = BucketStorageIterator< T >;
	using const_iterator = BucketStorageConstIterator< T >;
	using size_type = std::size_t;

	explicit BucketStorage();

	explicit BucketStorage(size_t block_capacity);

	BucketStorage(const BucketStorage &other);

	BucketStorage(BucketStorage &&other) noexcept;

	~BucketStorage();

	BucketStorage &operator=(const BucketStorage &other);

	BucketStorage &operator=(BucketStorage &&other) noexcept;

	iterator insert(const value_type &value);

	iterator insert(value_type &&value);

	iterator erase(const_iterator it);

	[[nodiscard]] bool empty() const noexcept;

	[[nodiscard]] size_t size() const noexcept;

	size_type capacity() const noexcept;

	void shrink_to_fit();

	void clear_blocks_and_elements_inside();

	void clear();

	void swap(BucketStorage &other) noexcept;

	iterator begin() noexcept;

	iterator end() noexcept;

	const_iterator begin() const noexcept;

	const_iterator end() const noexcept;

	const_iterator cbegin() const noexcept;

	const_iterator cend() const noexcept;

	iterator get_to_distance(iterator it, const difference_type distance);

  private:
	Block< T > *retrieve_block();

	void remove_block(Block< T > *block);

	void copy_storage_elements(const BucketStorage &other);

	Block< T > *head_block;
	Block< T > *tail_block;
	size_t block_capacity;
	size_t elements_count;
	size_t blocks_count;
	LinkedStack< T > *available_blocks;
};

template< typename T >
BucketStorage< T >::BucketStorage() : BucketStorage(64)
{
}

template< typename T >
BucketStorage< T >::BucketStorage(size_t block_capacity) :
	block_capacity(block_capacity), elements_count(0), blocks_count(0), head_block(nullptr), tail_block(nullptr),
	available_blocks(new LinkedStack< T >())
{
}

template< typename T >
BucketStorage< T >::BucketStorage(const BucketStorage &other) :
	block_capacity(other.block_capacity), elements_count(other.elements_count), blocks_count(other.blocks_count),
	head_block(nullptr), tail_block(nullptr), available_blocks(new LinkedStack< T >())
{
	this->copy_storage_elements(other);
}

template< typename T >
BucketStorage< T >::BucketStorage(BucketStorage &&other) noexcept :
	block_capacity(other.block_capacity), elements_count(other.elements_count), blocks_count(other.blocks_count),
	head_block(std::move(other.head_block)), tail_block(std::move(other.tail_block)), available_blocks(other.available_blocks)
{
	other.head_block = nullptr;
	other.tail_block = nullptr;
	other.elements_count = 0;
	other.blocks_count = 0;
	other.available_blocks = nullptr;
}

template< typename T >
BucketStorage< T >::~BucketStorage()
{
	clear();
	delete available_blocks;
}

template< typename T >
BucketStorage< T > &BucketStorage< T >::operator=(const BucketStorage &other)
{
	if (this != &other)
	{
		clear();
		block_capacity = other.block_capacity;
		this->copy_storage_elements(other);
	}
	return *this;
}

template< typename T >
BucketStorage< T > &BucketStorage< T >::operator=(BucketStorage &&other) noexcept
{
	if (this != &other)
	{
		BucketStorage temp(std::move(other));
		swap(temp);
	}
	return *this;
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::insert(const value_type &value)
{
	Block< T > *current_block = retrieve_block();
	current_block->insert_element_general(value);
	++elements_count;
	if (current_block->is_full())
	{
		available_blocks->void_pop();
	}
	return iterator(current_block, current_block->tail_element);
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::insert(value_type &&value)
{
	Block< T > *current_block = retrieve_block();
	current_block->insert_element_general(std::move(value));
	++elements_count;
	if (current_block->is_full())
	{
		available_blocks->void_pop();
	}
	return iterator(current_block, current_block->tail_element);
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::erase(const_iterator it)
{
	Block< T > *current_block = it.current_block;
	typename Block< T >::Element *current_element = it.current_element;

	if (!current_block || !current_element)
	{
		return iterator(nullptr, nullptr);
	}

	typename Block< T >::Element *next_element = current_element->next_element;

	auto safe_remove = [this, current_block, current_element]()
	{
		current_block->remove_element(current_element);
		--elements_count;
		if (current_block->is_empty())
		{
			available_blocks->get_rid_of(current_block);
			remove_block(current_block);
		}
	};

	if (!current_block->prev_block && !current_block->next_block && !current_element->next_element && !current_element->prev_element)
	{
		safe_remove();
		return end();
	}
	else if (!current_element->next_element && current_block->next_block)
	{
		Block< T > *next_block = it.current_block->next_block;
		safe_remove();
		return iterator(next_block, next_block->head_element);
	}

	safe_remove();
	return iterator(current_block, next_element);
}

template< typename T >
bool BucketStorage< T >::empty() const noexcept
{
	return elements_count == 0;
}

template< typename T >
size_t BucketStorage< T >::size() const noexcept
{
	return elements_count;
}

template< typename T >
typename BucketStorage< T >::size_type BucketStorage< T >::capacity() const noexcept
{
	return block_capacity * blocks_count;
}

template< typename T >
void BucketStorage< T >::shrink_to_fit()
{
	BucketStorage< T > new_storage(block_capacity);
	while (!this->empty())
	{
		new_storage.insert(std::move(*begin()));
		erase(begin());
	}
	swap(new_storage);
}

template< typename T >
void BucketStorage< T >::clear_blocks_and_elements_inside()
{
	Block< T > *current_block = head_block;
	while (current_block)
	{
		Block< T > *next_block = current_block->next_block;
		current_block->clear();
		delete current_block;
		current_block = next_block;
	}
	head_block = tail_block = nullptr;
	elements_count = 0;
	blocks_count = 0;
}

template< typename T >
void BucketStorage< T >::clear()
{
	clear_blocks_and_elements_inside();
	if (available_blocks != nullptr)
	{
		available_blocks->clear();
	}
	elements_count = 0;
	blocks_count = 0;
	head_block = nullptr;
	tail_block = nullptr;
}

template< typename T >
void BucketStorage< T >::swap(BucketStorage &other) noexcept
{
	using std::swap;
	swap(head_block, other.head_block);
	swap(tail_block, other.tail_block);
	swap(block_capacity, other.block_capacity);
	swap(elements_count, other.elements_count);
	swap(blocks_count, other.blocks_count);
	swap(available_blocks, other.available_blocks);
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::begin() noexcept
{
	return iterator(head_block, head_block ? head_block->head_element : nullptr);
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::end() noexcept
{
	return iterator(tail_block, nullptr);
}

template< typename T >
typename BucketStorage< T >::const_iterator BucketStorage< T >::begin() const noexcept
{
	return const_iterator(head_block, head_block ? head_block->head_element : nullptr);
}

template< typename T >
typename BucketStorage< T >::const_iterator BucketStorage< T >::end() const noexcept
{
	return const_iterator(tail_block, nullptr);
}

template< typename T >
typename BucketStorage< T >::const_iterator BucketStorage< T >::cbegin() const noexcept
{
	return begin();
}

template< typename T >
typename BucketStorage< T >::const_iterator BucketStorage< T >::cend() const noexcept
{
	return end();
}

template< typename T >
typename BucketStorage< T >::iterator BucketStorage< T >::get_to_distance(iterator it, const difference_type distance)
{
	difference_type remaining = distance;
	while (remaining != 0)
	{
		if (remaining > 0)
		{
			++it;
			--remaining;
		}
		else
		{
			--it;
			++remaining;
		}
	}
	return it;
}

template< typename T >
Block< T > *BucketStorage< T >::retrieve_block()
{
	if (!available_blocks->empty())
	{
		return available_blocks->top();
	}

	auto *new_block = new Block< T >(block_capacity);
	available_blocks->push(new_block);
	++blocks_count;

	if (!head_block)
	{
		head_block = tail_block = new_block;
	}
	else
	{
		tail_block->next_block = new_block;
		new_block->prev_block = tail_block;
		tail_block = new_block;
	}

	return new_block;
}

template< typename T >
void BucketStorage< T >::remove_block(Block< T > *block)
{
	if (!block)
	{
		throw std::runtime_error("Attempted to remove a null block");
	}
	if (block == head_block)
		head_block = block->next_block;
	if (block == tail_block)
		tail_block = block->prev_block;
	if (block->prev_block)
		block->prev_block->next_block = block->next_block;
	if (block->next_block)
		block->next_block->prev_block = block->prev_block;
	--blocks_count;
	delete block;
}

template< typename T >
void BucketStorage< T >::copy_storage_elements(const BucketStorage &other)
{
	for (auto it = other.begin(); it != other.end(); ++it)
	{
		insert(*it);
	}
}

#endif /* BUCKET_STORAGE_HPP */
