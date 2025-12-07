#ifndef ARRAY_HPP
#define ARRAY_HPP
// #if __cplusplus >= 202002L

#include <memory>
#include <new>
#include <vector>
#include <compare>
#include <memory>
#include <type_traits>
#include <initializer_list>
#include <iterator>
#include <cassert>
namespace Potato {
	namespace TypeTools {
		template <typename ValueType>
		struct SimpleType {
			using value_type      = ValueType;
			using size_type       = std::size_t;
			using difference_type = std::ptrdiff_t;
			using pointer         = ValueType*;
			using const_pointer   = const ValueType*;
			using reference       = ValueType&;
			using const_reference = const ValueType&;
		};

		template <typename Alloc>
		constexpr bool IsSimpleAllocVal = std::is_same_v<typename std::allocator_traits<Alloc>::size_type, size_t>
				&& std::is_same_v<typename std::allocator_traits<Alloc>::difference_type, ptrdiff_t>
				&& std::is_same_v<typename std::allocator_traits<Alloc>::pointer, typename Alloc::value_type*>
				&& std::is_same_v<typename std::allocator_traits<Alloc>::const_pointer, const typename Alloc::value_type*>;
	}
	namespace MemoryTools{
		template <typename Container>
		struct [[nodiscard]] CleanGuard {
			Container* container;
			constexpr ~CleanGuard() {
				if (container) {
					container->M_Clear();
				}
			}
		};

		struct ZeroConstructCompressedTag{
			explicit ZeroConstructCompressedTag() = default;
		};
		struct OneConstructCompressedTag{
			explicit OneConstructCompressedTag() = default;
		};
		template <typename Compressed, typename Content, bool = std::is_empty_v<Compressed> && !std::is_final_v<Compressed>>
		struct CompressedPair final : private Compressed {
		public:
			Content data;

			using Base = Compressed;

			template <typename ... Other2>
			constexpr explicit CompressedPair(ZeroConstructCompressedTag, Other2&& ... args)
				noexcept(std::conjunction_v<std::is_nothrow_default_constructible<Compressed>, std::is_nothrow_constructible<Content, Other2...>>)
				: Base(), data(std::forward<Other2>(args)...) {}

			template <typename Other1, typename ... Other2>
			constexpr explicit CompressedPair(OneConstructCompressedTag, Other1&& arg1, Other2&& ... args2)
				noexcept(std::conjunction_v<std::is_nothrow_constructible<Compressed, Other1>, std::is_nothrow_constructible<Content, Other2...>>)
				: Base(std::forward<Other1>(arg1)), data(std::forward<Other2>(args2)...) {}

			constexpr Compressed& GetFirst() noexcept {
				return *this;
			}
			constexpr Compressed& GetFirst() const noexcept {
				return *this;
			}

		};

		template <typename Compressed, typename Content>
		struct CompressedPair<Compressed, Content, false> final {
		public:
			Content data;
			Compressed first;
			template <typename ... Other2>
			constexpr explicit CompressedPair(ZeroConstructCompressedTag, Other2&& ... args)
				noexcept(std::conjunction_v<std::is_nothrow_default_constructible<Compressed>, std::is_nothrow_constructible<Content, Other2...>>)
				: first(), data(std::forward<Other2>(args)...) {}

			template <typename Other1, typename ... Other2>
			constexpr explicit CompressedPair(OneConstructCompressedTag, Other1&& arg1, Other2&& ... args2)
				noexcept(std::conjunction_v<std::is_nothrow_constructible<Compressed, Other1>, std::is_nothrow_constructible<Content, Other2...>>)
				: first(std::forward<Other1>(arg1)), data(std::forward<Other2>(args2)...) {}

			constexpr Compressed& GetFirst() noexcept {
				return first;
			}
			constexpr Compressed& GetFirst() const noexcept {
				return first;
			}
		};

		template <typename Alloc>
		using AllocPointer = typename std::allocator_traits<Alloc>::pointer;

		template <typename Alloc>
		using AllocSize = typename std::allocator_traits<Alloc>::size_type;

		template <typename Ptr>
		constexpr auto Unfancy(Ptr ptr) noexcept -> decltype(std::addressof(*ptr)){
			return std::addressof(*ptr);
		}

		template <typename Ty>
		constexpr Ty* Unfancy(Ty* ptr) noexcept {
			return ptr;
		}

		template <class Ptr>
		constexpr auto UnfancyMaybeNull(Ptr ptr) noexcept {
			// converts from a (potentially null) fancy pointer to a plain pointer
			return ptr ? _STD addressof(*ptr) : nullptr;
		}

		template <class Ty>
		constexpr Ty* UnfancyMaybeNull(Ty* ptr) noexcept { // do nothing for plain pointers
			return ptr;
		}

		template <typename Ptr, std::enable_if_t<!std::is_pointer_v<Ptr>, int> = 0>
		constexpr Ptr Refancy(typename std::pointer_traits<Ptr>::element_type* ptr) noexcept {
			return std::pointer_traits<Ptr>::pointer_to(*ptr);
		}

		template <typename Ptr, std::enable_if_t<std::is_pointer_v<Ptr>, int> = 0>
		constexpr Ptr Refancy(Ptr ptr) noexcept {
			return ptr;
		}

		template <typename Ptr, std::enable_if_t<!std::is_pointer_v<Ptr>, int> = 0>
		constexpr Ptr RefancyMaybeNull(typename std::pointer_traits<Ptr>::element_type* ptr) noexcept {
			return ptr == nullptr ? Ptr() : std::pointer_traits<Ptr>::pointer_to(*ptr);
		}

		template <typename Ptr, std::enable_if_t<std::is_pointer_v<Ptr>, int> = 0>
		constexpr Ptr RefancyMaybeNull(Ptr ptr) noexcept {
			return ptr;
		}

		template <typename Alloc, typename = void>
		struct HasMemberDestroy : std::false_type{};

		template <typename Alloc>
		struct HasMemberDestroy <
			Alloc,
			std::void_t<
			/* std::declval<Alloc&>() 产生一个 Alloc 的左值引用类型, 能检测到对左值限定（&）的成员函数 */
			/* std::declval<Alloc>() 产生一个 Alloc 的右值引用类型 */
			/* 但是allocator_traits::destroy 在真实调用时接受 Alloc& */
			/* 检测 Alloc 是否有自定义的 destroy(pointer) */
				decltype(std::declval<Alloc&>().destroy(
					std::declval<typename std::allocator_traits<Alloc>::pointer>()
				))
			>
		> : std::true_type{};


		template <typename Alloc>
		constexpr bool UseDefaultDestroyVal = (!HasMemberDestroy<Alloc>::value) ||
			std::is_same_v<Alloc, std::allocator<typename Alloc::value_type>>;

		template <typename Alloc, class=void>
		struct IsDefaultAllocator : std::false_type{};

		template <typename Iter>
		constexpr bool UseMemsetValueConstruct = std::conjunction_v<
				std::bool_constant<std::contiguous_iterator<Iter>>,
				std::is_scalar<std::iter_value_t<Iter>>,
				std::negation<std::is_volatile<std::remove_reference_t<std::iter_reference_t<Iter>>>>,
				std::negation<std::is_member_pointer<std::iter_value_t<Iter>>>
			>;


		template <typename Alloc>
		constexpr void DestroyRange(AllocPointer<Alloc> first, AllocPointer<Alloc> last, Alloc& allocator) noexcept {
			using value_type = typename Alloc::value_type;
			if constexpr (!(std::is_trivially_destructible_v<value_type> && UseDefaultDestroyVal<Alloc>)){
				for (; first != last; ++first){
					std::allocator_traits<Alloc>::destroy(allocator, std::addressof(*first));
				}
			}
		}

		template <typename Alloc>
		class [[nodiscard]] ConstructBackoutGuard {
			using pointer = AllocPointer<Alloc>;

		public:
			constexpr ConstructBackoutGuard(pointer dest, Alloc& allocator)
				: m_dest(dest)
				, m_src(dest)
				, allocator(allocator){}

			ConstructBackoutGuard(const ConstructBackoutGuard&)            = delete;
			ConstructBackoutGuard& operator=(const ConstructBackoutGuard&) = delete;

			constexpr ~ConstructBackoutGuard() {
				DestroyRange(m_dest, m_src, allocator);
			}

			template <typename... Types>
			constexpr void Append(Types&&... args) {
				std::allocator_traits<Alloc>::construct(allocator, Unfancy(m_dest), std::forward<Types>(args)...);
				++m_dest;
			}

			constexpr pointer Release() {
				m_src = m_dest;
				return m_dest;
			}
		private:
			pointer m_dest;
			pointer m_src;
			Alloc& allocator;
		};
		/**
		 * @brief 批量零初始化或者调用默认构造函数在一块内存上默认构造
		 * @param start: 起始位置
		 * @param count: 构造数量
		 * @param allocator: 分配器
		 */
		template <typename Alloc>
		constexpr AllocPointer<Alloc> BatchOfZeroConstruction(AllocPointer<Alloc> start, AllocSize<Alloc> count, Alloc& allocator){
			using pointer = typename Alloc::value_type*;
			using size_type = typename std::allocator_traits<Alloc>::size_type;
			using value_type = std::remove_pointer_t<std::remove_cv_t<pointer>>;
			/* memset 优化 */
			if constexpr (std::is_trivially_default_constructible_v<value_type>
						&& std::is_trivially_copyable_v<value_type>
						&& !std::is_constant_evaluated()) /* 常量求值的情况下不使用 memset */
			{
				auto first = Unfancy(start);
				char* const first_ = reinterpret_cast<char*>(first);
				char* const last_ = reinterpret_cast<char*>(first_ + count);
				std::memset(first_, 0, static_cast<std::size_t>(last_ - first_));
				return start + count;
			}
			if (std::is_constant_evaluated()) {
				for (size_type i=0; i < count; ++i){
					/* 在常量求值的语境下 start[i] 为空指针则编译失败 */
					std::construct_at(std::addressof(start[i]));
				}
				return start + count;
			}

			// C++20 中 uninitialized_value_construct_n 不是 constexpr
			// std::uninitialized_value_construct_n(Unfancy(start), count);
			ConstructBackoutGuard<Alloc> Guard{ start, allocator };
			for (; 0 < count; --count){
				Guard.Append();
			}
			return Guard.Release();
		}

		/**
		 * @brief 批量使用值进行对内存进行初始化
		 * @param start: 起始位置
		 * @param count: 构造数量
		 * @param value: 初始化值
		 * @param allocator: 分配器
		 */
		template <typename Alloc>
		constexpr AllocPointer<Alloc> BatchOfFillConstruction(AllocPointer<Alloc> start, AllocSize<Alloc> count, const typename Alloc::value_type& value, Alloc& allocator){
			using Ty = typename Alloc::value_type;
			// memset 路径: 对于标量类型且非 volatile 类型可以使用 memset 优化

			ConstructBackoutGuard<Alloc> Guard{ start, allocator };
			for (; 0 < count; --count){
				Guard.Append();
			}
			return Guard.Release();
		}

	};

	template <typename ValueType, typename SizeType, typename DifferenceType, typename Pointer, typename ConstPointer, typename Reference, typename ConstReference>
	struct GenerateElementWrapper {
		using value_type      = ValueType;
		using size_type       = SizeType;
		using difference_type = DifferenceType;
		using pointer         = Pointer;
		using const_pointer   = ConstPointer;
		using reference       = Reference;
		using const_reference = ConstReference;
	};

	struct ZeroInitTag {
		explicit ZeroInitTag() = default;
	};
	template <typename Array>
	struct ArrayConstIterator {
		using iterator_concept = std::contiguous_iterator_tag;
		using iterator_category = std::random_access_iterator_tag;
		using value_type        = typename Array::value_type;
		using size_type         = typename Array::size_type;
		using difference_type   = typename Array::difference_type;
		using pointer           = typename Array::const_pointer;
		using reference         = const value_type&;

		using M_Ptr = typename Array::pointer;

		constexpr ArrayConstIterator() noexcept : ptr() {}
		constexpr explicit ArrayConstIterator(M_Ptr ptr) noexcept : ptr(ptr) {}

		[[nodiscard]] constexpr reference operator*() const noexcept {
			return *ptr;
		}
		[[nodiscard]] constexpr pointer operator->() const noexcept {
			return ptr;
		}

		[[nodiscard]] constexpr reference operator[](const difference_type index) const noexcept {
			return *(*this + index);
		}

		/* C++20 中定义了 operator<=> 就不用定义 operator<, operator> etc 这类序关系了, 编译器可以自己推导出来 */
		[[nodiscard]] constexpr std::strong_ordering operator<=>(const ArrayConstIterator& other) const noexcept {
			return MemoryTools::UnfancyMaybeNull(ptr) <=> MemoryTools::UnfancyMaybeNull(other.ptr);
		}

		[[nodiscard]] constexpr bool operator==(const ArrayConstIterator& other) const noexcept {
			return ptr == other.ptr;
		}

		[[nodiscard]] constexpr bool operator!=(const ArrayConstIterator& other) const noexcept {
			return !(*this == other.ptr);
		}

		constexpr ArrayConstIterator& operator++() noexcept {
			++ptr;
			return *this;
		}

		constexpr ArrayConstIterator operator++(int) noexcept {
			ArrayConstIterator temp = *this;
			++*this;
			return temp;
		}

		constexpr ArrayConstIterator& operator--() noexcept {
			--ptr;
			return *this;
		}

		constexpr ArrayConstIterator operator--(int) noexcept {
			ArrayConstIterator temp = *this;
			--*this;
			return temp;
		}

		constexpr ArrayConstIterator& operator+=(const difference_type offset) noexcept {
			ptr += offset;
			return *this;
		}
		[[nodiscard]] constexpr ArrayConstIterator operator+(const difference_type n) const noexcept {
			return ArrayConstIterator(ptr + n);
		}
		[[nodiscard]] friend constexpr ArrayConstIterator operator+(
			const difference_type offset, ArrayConstIterator next) noexcept
		{
			next += offset;
			return next;
		}

		[[nodiscard]] constexpr ArrayConstIterator operator-(const difference_type n) const noexcept {
			return ArrayConstIterator(ptr - n);
		}
		[[nodiscard]] friend constexpr ArrayConstIterator operator-(
			const difference_type offset, ArrayConstIterator next) noexcept
		{
			next -= offset;
			return next;
		}

		[[nodiscard]] constexpr difference_type operator-(const ArrayConstIterator other) const noexcept {
			return static_cast<difference_type>(ptr - other.ptr);
		}

		// [[nodiscard]] constexpr M_Ptr GetRawPointer() const noexcept {
		// 	return MemoryTools::UnfancyMaybeNull(ptr);
		// }
		//
		// constexpr void SeekTo(const value_type* raw_pointer) {
		// 	ptr = MemoryTools::RefancyMaybeNull<M_Ptr>(const_cast<value_type*>(raw_pointer));
		// }
		M_Ptr ptr;
	};

	template <typename Array>
	struct ArrayIterator : public ArrayConstIterator<Array> {
		using Base = ArrayConstIterator<Array>;
		using iterator_concept = std::contiguous_iterator_tag;
		using iterator_category = std::random_access_iterator_tag;
		using value_type        = typename Array::value_type;
		using difference_type    = typename Array::difference_type;
		using pointer           = typename Array::pointer;
		using reference         = value_type&;

		using Base::Base;

		[[nodiscard]] constexpr reference operator*() const noexcept {
			return *this->ptr;
		}
		[[nodiscard]] constexpr pointer operator->() const noexcept {
			return this->ptr;
		}

		[[nodiscard]] constexpr reference operator[](const difference_type index) const noexcept {
			return *(*this + index);
		}

		constexpr ArrayIterator& operator++() noexcept {
			Base::operator++();
			return *this;
		}

		constexpr ArrayIterator operator++(int) noexcept {
			ArrayIterator temp = *this;
			Base::operator++();
			return temp;
		}

		constexpr ArrayIterator& operator--() noexcept {
			Base::operator--();
			return *this;
		}

		constexpr ArrayIterator operator--(int) noexcept {
			ArrayIterator temp = *this;
			Base::operator--();
			return temp;
		}

		constexpr ArrayIterator& operator+=(const difference_type offset) noexcept {
			Base::operator+=(offset);
			return *this;
		}

		[[nodiscard]] constexpr ArrayIterator operator+(const difference_type offset) const noexcept {
			ArrayIterator temp = *this;
			temp += offset;
			return temp;
		}

		[[nodiscard]] friend constexpr ArrayIterator operator+(
			const difference_type offset, ArrayIterator next) noexcept {
			next += offset;
			return next;
		}

		constexpr ArrayIterator& operator-=(const difference_type offset) noexcept {
			Base::operator-=(offset);
			return *this;
		}

		using Base::operator-;

		[[nodiscard]] constexpr ArrayIterator operator-(const difference_type offset) const noexcept {
			ArrayIterator temp = *this;
			temp -= offset;
			return temp;
		}

	};


	template <typename ElementType, class AllocatorType=std::allocator<ElementType>>
	class Array {
	private:
		using M_AllocatorType =
		typename std::allocator_traits<AllocatorType>
			::template rebind_alloc<ElementType>;
		using M_AllocatorTraits = std::allocator_traits<M_AllocatorType>;
		template <typename ElementTypeWrapper>
		struct ArrayData {
			using value_type      = typename ElementTypeWrapper::value_type;
			using size_type       = typename ElementTypeWrapper::size_type;
			using difference_type = typename ElementTypeWrapper::difference_type;
			using pointer         = typename ElementTypeWrapper::pointer;
			using const_pointer   = typename ElementTypeWrapper::const_pointer;
			using reference       = typename ElementTypeWrapper::reference;
			using const_reference = typename ElementTypeWrapper::const_reference;

			constexpr ArrayData() noexcept
				: start()
				, finish()
				, end_of_storage(){}

			constexpr ArrayData(pointer start, pointer finish, pointer end) noexcept
				: start(start)
				, finish(finish)
				, end_of_storage(end){}

			constexpr void CopyData(ArrayData const& other) noexcept(true) {
				start = other.start;
				finish = other.finish;
				end_of_storage = other.end_of_storage;
			}

			constexpr void SwapData(ArrayData& other) noexcept {
				std::swap(start, other.start);
				std::swap(finish, other.finish);
				std::swap(end_of_storage, other.end_of_storage);
			}

			pointer start;
			pointer finish;
			pointer end_of_storage;
		};
	public:
		using value_type             = ElementType;
		using allocator_type         = AllocatorType;
		using size_type              = typename M_AllocatorTraits::size_type;
		using difference_type        = typename M_AllocatorTraits::difference_type;
		using pointer                = typename M_AllocatorTraits::pointer;
		using const_pointer          = typename M_AllocatorTraits::const_pointer;
		using reference              = value_type&;
		using const_reference        = const value_type&;
		using iterator               = ArrayIterator<Array>;
		using const_iterator         = ArrayConstIterator<Array>;
		using reverse_iterator       = std::reverse_iterator<iterator>;
		using reverse_const_iterator = std::reverse_iterator<const_iterator>;

	private:
		using M_DateType = ArrayData<
		std::conditional_t<TypeTools::IsSimpleAllocVal<M_AllocatorType>,
			TypeTools::SimpleType<value_type>,
			GenerateElementWrapper<
				value_type,
				size_type,
				difference_type,
				pointer,
				const_pointer,
				reference,
				const_reference
			>
		>
	>;

		using M_RealValueType = MemoryTools::CompressedPair<M_AllocatorType, M_DateType>;
	public:
		constexpr explicit Array() noexcept
			: m_Data(MemoryTools::ZeroConstructCompressedTag{}) {}
		constexpr explicit Array(const AllocatorType& allocator) noexcept
			: m_Data(MemoryTools::OneConstructCompressedTag{}, allocator) {}

		constexpr explicit Array(const size_type count, const AllocatorType& allocator=AllocatorType())
			: m_Data(MemoryTools::OneConstructCompressedTag{}, allocator) {

			this->M_FillZeroConstruct(count);
		}
		constexpr Array(size_type count, const reference value, const AllocatorType& allocator=AllocatorType()) {}
		template <typename InputIterator>
		constexpr Array(InputIterator first, InputIterator last, const AllocatorType& allocator=AllocatorType()) {}
		Array(std::initializer_list<value_type> list, const AllocatorType& allocator=AllocatorType()){}
		constexpr Array(const Array& other) {}
		constexpr Array(Array&& other) noexcept {}

	public:
		[[nodiscard]] constexpr Array& operator=(const Array& other) {}
		[[nodiscard]] constexpr Array& operator=(Array&& other) noexcept {}
		[[nodiscard]] constexpr Array& operator=(std::initializer_list<value_type> list) {}
		[[nodiscard]] constexpr reference operator[](const size_type index) noexcept {}
		[[nodiscard]] constexpr const reference operator[](const size_type index) const noexcept {}
		[[nodiscard]] Array& operator+=(const Array& other) {}
		[[nodiscard]] Array& operator+=(Array&& other) {}
		[[nodiscard]] Array& operator+=(std::initializer_list<value_type> list) {}
	public:
		constexpr void Clear() noexcept {}

		value_type* Data() noexcept {}
		const value_type* Data() const noexcept {}

		void Assign(size_type count, const value_type& value) {}
		template <typename InputIterator>
		constexpr void Assign(InputIterator first, InputIterator last) {}
		constexpr void Assign(std::initializer_list<value_type> list) {}

		[[nodiscard]] constexpr reference At(const size_type index) {}
		[[nodiscard]] constexpr const_reference At(const size_type index) const {}

		[[nodiscard]] constexpr size_type Find(const reference item) const {}
		[[nodiscard]] constexpr size_type Find(const reference item) {}
		template <typename Predicate>
		[[nodiscard]] constexpr size_type FindIf(Predicate pred) const {}
		template <typename Predicate>
		[[nodiscard]] constexpr size_type FindIf(Predicate pred) {}

		template <typename Predicate>
		[[nodiscard]] constexpr Array Filter(Predicate pred) const {}
		template <typename Predicate>
		[[nodiscard]] constexpr Array Filter(Predicate pred) {}

		[[nodiscard]] constexpr size_type Count(const reference item) const {}

		template <typename Predicate>
		[[nodiscard]] constexpr size_type Count(Predicate pred) const {}

		[[nodiscard]] constexpr bool IsContain(const reference item) const { return true; }
		template <typename Predicate>
		[[nodiscard]] constexpr bool IsContain(Predicate pred) const { return true; }

		[[nodiscard]] constexpr reference Front(const size_type index) {}
		[[nodiscard]] constexpr const_reference Front(const size_type index) const {}

		[[nodiscard]] constexpr reference Back(const size_type index) {}
		[[nodiscard]] constexpr const_reference Back(const size_type index) const {}

		constexpr iterator Insert(const_iterator position, value_type& value) {}
		constexpr iterator Insert(const_iterator position, const value_type& value) {}
		constexpr iterator Insert(const_iterator position, size_type count, const value_type& value) {}
		template <typename InputIterator>
		constexpr iterator Insert(const_iterator position, InputIterator first, InputIterator last) {}
		constexpr iterator Insert(const_iterator position, std::initializer_list<value_type> list) {}

		constexpr iterator InsertAt(size_type index, value_type& value) {}
		constexpr iterator InsertAt(size_type index, const value_type& value) {}
		constexpr iterator InsertAt(size_type index, size_type count, const value_type& value) {}

		constexpr iterator InsertUnique(size_type index, value_type& value) {}
		constexpr iterator InsertUnique(size_type index, const value_type& value) {}


		constexpr void Append(Array&& array) {}
		constexpr void Append(pointer ptr, size_type count) {}

		constexpr void InsertUninitializedItem(const_iterator position) {}
		constexpr void InsertUninitializedItem(const_iterator position, const size_type count) {}
		constexpr reference InsertZeroedItem(const_iterator position) {}
		constexpr reference InsertZeroedItem(const_iterator position, const size_type count) {}

		constexpr iterator Erase(const_iterator position){}
		constexpr iterator Erase(const_iterator first, const_iterator last){}
		constexpr iterator EraseAt(const size_type index){}
		template <typename Predicate>
		constexpr iterator EraseIf(Predicate pred){}

		template <typename ... Args>
		constexpr iterator Emplace(const_iterator position, Args&& ... args) {}
		template <typename ... Args>
		constexpr iterator EmplaceAt(size_type index, Args&& ... args) {}

		template <typename ... Args>
		constexpr reference EmplaceBack(Args&& ... args) {}

		constexpr iterator Push(const value_type& value) {}
		constexpr iterator Push(value_type& value) {}
		constexpr iterator Pop() noexcept {}
		constexpr iterator Top() noexcept {}
		constexpr const_iterator Top() const noexcept {}

		constexpr void Resize(size_type count) {}
		constexpr void Resize(size_type count, const value_type& value) {}
		constexpr void Reserve(size_type capacity) {}
		constexpr void Swap() noexcept {}

		bool IsEmpty() const noexcept {}
		size_type GetSize() const noexcept {}
		size_type GetCapacity() const noexcept {}
		size_type GetSlack() const noexcept {}

		void Shrink() noexcept {}

	public:
		[[nodiscard]] constexpr iterator begin() noexcept {}
		[[nodiscard]] constexpr const_iterator begin() const noexcept {}
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept {}

		[[nodiscard]] constexpr iterator end() noexcept {}
		[[nodiscard]] constexpr const_iterator end() const noexcept {}
		[[nodiscard]] constexpr const_iterator cend() const noexcept {}

		[[nodiscard]] constexpr reverse_iterator rbegin() noexcept {}
		[[nodiscard]] constexpr reverse_const_iterator rbegin() const noexcept {}
		[[nodiscard]] constexpr reverse_const_iterator crbegin() const noexcept {}

		[[nodiscard]] constexpr reverse_iterator rend() noexcept {}
		[[nodiscard]] constexpr reverse_const_iterator rend() const noexcept {}
		[[nodiscard]] constexpr reverse_const_iterator crend() const noexcept {}

	private:

		template <typename Container>
		struct [[nodiscard]] CleanGuard {
			Container* container;
			constexpr ~CleanGuard() {
				if (container) {
					container->M_Clear();
				}
			}
			constexpr void Release() noexcept {
				container = nullptr;
			}
		};
		struct [[nodiscard]] ReallocateGuard {
			AllocatorType& allocator;
			pointer new_start;
			size_type new_capacity;
			pointer constructed_start;
			pointer constructed_finish;

			ReallocateGuard& operator=(const ReallocateGuard& other) = delete;
			ReallocateGuard(const ReallocateGuard& other) = delete;

			constexpr ~ReallocateGuard() noexcept {
				if (new_start) {
					MemoryTools::DestroyRange(constructed_start, constructed_finish, allocator);
					allocator.deallocate(new_start, new_capacity);
				}
			}

			constexpr void Release() noexcept {
				new_start = nullptr;
			}
		};
		struct [[nodiscard]] SimpleReallocateGuard {
			AllocatorType& allocator;
			pointer new_start;
			size_type new_capacity;

			SimpleReallocateGuard& operator=(const SimpleReallocateGuard& other) = delete;
			SimpleReallocateGuard(const SimpleReallocateGuard& other) = delete;

			constexpr ~SimpleReallocateGuard() noexcept {
				if (new_start) {
					allocator.deallocate(new_start, new_capacity);
				}
			}

			constexpr void Release() noexcept {
				new_start = nullptr;
			}
		};
	private:
		/**
		* @brief 分配未初始化内存, 用于构造时申请内存
		* @param count 分配的容量 (对象个数)
		*/
		constexpr void M_AllocateUninitializedMemory(size_type count){
			auto& M_Data            = this->m_Data.data;
			pointer& start          = M_Data.start;
			pointer& finish         = M_Data.finish;
			pointer& end_of_storage = M_Data.end_of_storage;

			assert(start == nullptr && finish == nullptr && end_of_storage == nullptr && "memory must not be already allocated.");

			const pointer mem = M_GetAllocator().allocate(count);
			start           = mem;
			finish          = mem;
			end_of_storage  = mem + count;
		}
		/**
		 * @brief 拷贝填充构造 -> 对应构造函数 Array(n) -> 其会默认进行零初始化或者默认构造
		 * @param count 元素个数
		 */
		template <typename Ty2>
		constexpr void M_FillZeroConstruct (const size_type count, const Ty2& value) {
			if (count == 0) return;

			auto& allocator = M_GetAllocator();
			auto& M_Data    = this->m_Data.data;
			pointer& start  = M_Data.start;

			this->M_AllocateUninitializedMemory(count);
			CleanGuard Guard{ this };
			std::uninitialized_value_construct_n(start, count);
			Guard.container = nullptr;
		}

		/**
		 * @brief 拷贝填充构造 -> 对应构造函数 Array(n) -> 其会默认进行零初始化或者默认构造
		 * @param count 元素个数
		 * @param value 填充值
		 */
		template <typename Ty2>
		constexpr void M_FillValueConstruct(const size_type count, const Ty2& value) {
			if (count == 0) return;

			auto& allocator = M_GetAllocator();
			auto& M_Data    = this->m_Data.data;
			pointer& start  = M_Data.start;

			this->M_AllocateUninitializedMemory(count);
			CleanGuard Guard{ this };
			std::uninitialized_fill_n(start, count, value);
			Guard.Release();
		}

		template <typename ResizeType>
		constexpr void M_Resize(const size_type new_size, const ResizeType& value) {
			auto& allocator         = M_GetAllocator();
			auto& M_Data            = this->m_Data.data;
			pointer& start          = M_Data.start;
			pointer& finish         = M_Data.finish;
			pointer& end_of_storage = M_Data.end_of_storage;

			const auto old_size = static_cast<size_type>(finish - start);

			if (new_size < old_size) {
				// 内存收缩
				const pointer new_finish = start + new_size;
				std::destroy_n(new_finish, old_size - new_size);
				finish -= new_finish;
			} else if (new_size > old_size) {
				// 内存扩张, 先检查容量是否足够
				const auto old_capacity = static_cast<size_type>(end_of_storage - start);
				if (new_size > old_capacity) {
					// 元素不够, 重新分配内存
					M_Relocate(new_size, value);
					return ;
				}
				// old_capacity 还够用, 则不分配内存直接使用存货
				auto new_strat = start + new_size;
				auto increased_size = new_size - old_size;

				if constexpr (std::is_same_v<ResizeType, ZeroInitTag>) {
					finish = std::uninitialized_value_construct_n(new_strat, increased_size);
				}else {
					// 默认路径: 使用 val 进行 resize
					finish = std::uninitialized_fill_n(new_strat, increased_size, value);
				}
			}
		}

		/**
		 * @brief 重定位Array: 重新分配内存, 并搬运旧数据, 剩余部分用 value 填充
		 * @tparam Ty2: 应该与 ElementType 兼容, 即 Ty2 可以隐式转换至 Ty2
		 * @param new_size: 该函数默认 new_size > old_size 所以需要外部调用者保证
		 * @param value
		 */
		template <typename Ty2>
		constexpr void M_Relocate(const size_type new_size, const Ty2& value) {
			assert(new_size < M_MaxSize() && "Array::M_Relocate(const size_type, const Ty2&) Runtime Error: Exceeding maximum size limit - 内存分配数量超过内存限制");

			auto& allocator = M_GetAllocator();
			auto& M_Data            = this->m_Data.data;
			pointer& start          = M_Data.start;
			pointer& finish         = M_Data.finish;
			pointer& end_of_storage = M_Data.end_of_storage;

			const auto old_size = static_cast<size_type>(finish - start);
			size_type new_capacity = M_CalculateGrowth(new_size);

			const pointer new_arr = allocator.allocate(new_capacity);
			const pointer appended_start = new_arr + old_size;

			ReallocateGuard Guard{
				.allocator = allocator,
				.new_start = new_arr,
				.new_capacity = new_capacity,
				.constructed_start = appended_start,
				.constructed_finish = appended_start
			};
			auto& appended_finish = Guard.constructed_finish;
			const auto& increased_size = new_size - old_size;
			assert(increased_size>=0 && "Array::M_Relocate(const size_type, const Ty2&) Runtime Error:: new_size must be greater than old_size - M_Relocate 默认 new size >= old size");

			if constexpr (std::is_same_v<Ty2, ZeroInitTag>) {
				appended_finish = std::uninitialized_value_construct_n(appended_start, increased_size);
			}else {
				appended_finish = std::uninitialized_fill_n(appended_start, increased_size, value);
			}

			// 搬运旧数据: 能移动就移动, 不能移动就拷贝
			if constexpr (std::is_nothrow_move_constructible_v<ElementType> || !std::is_copy_constructible_v<ElementType>) {
				std::uninitialized_move_n(start, old_size, new_arr);
			}else {
				std::uninitialized_copy_n(start, old_size, new_arr);
			}

			this->M_UpdateData(new_arr, new_size, new_capacity);
			Guard.Release();
		}

		constexpr void M_UpdateData(const pointer first, const size_type size, const size_type capacity) noexcept {
			auto& allocator = M_GetAllocator();
			auto& M_Data            = this->m_Data.data;
			pointer& start          = M_Data.start;
			pointer& finish         = M_Data.finish;
			pointer& end_of_storage = M_Data.end_of_storage;

			if (!first) {
				this->M_Clear();
			}
			start = first;
			finish = first + size;
			end_of_storage = first + capacity;
		}

		[[nodiscard]] constexpr size_type M_CalculateGrowth(const size_type new_size) const {
			const size_type old_capacity = this->M_Capacity();
			const auto max_size = this->M_MaxSize();

			if (old_capacity < max_size - old_capacity) {
				return max_size;
			}

			const size_type new_capacity = old_capacity + old_capacity / 2;

			if (new_capacity < new_size) {
				new_capacity = new_size;
			}

			return new_capacity;
		}

		/**
		 * @berief 当前实现与平台限制下, 调用者理论上能容纳的最大元素个数
		 */
		[[nodiscard]] constexpr size_type M_MaxSize() const noexcept {
			return std::min(static_cast<size_type>(std::numeric_limits<size_type>::max()), M_AllocatorTraits::max_size(M_GetAllocator()));
		}

		[[nodiscard]] constexpr size_type M_Capacity() const noexcept {
			auto& M_Data            = this->m_Data.data;
			pointer& start          = M_Data.start;
			pointer& finish         = M_Data.finish;
			pointer& end_of_storage = M_Data.end_of_storage;

			return start ? static_cast<size_type>(end_of_storage - start) : 0u;
		}

		[[nodiscard]] constexpr size_type M_Size() const noexcept {
			auto& M_Data            = this->m_Data.data;
			pointer& start          = M_Data.start;
			pointer& finish         = M_Data.finish;
			pointer& end_of_storage = M_Data.end_of_storage;

			return start ? static_cast<size_type>(finish - start) : 0u;
		}

		[[nodiscard]] constexpr bool M_IsEmpty() const noexcept {
			return M_Size() == 0;
		}

		[[nodiscard]] constexpr const M_AllocatorType& M_GetAllocator() const noexcept {
			return m_Data.GetFirst();
		}
		[[nodiscard]] constexpr M_AllocatorType& M_GetAllocator() noexcept {
			return m_Data.GetFirst();
		}

		constexpr void M_Clear() noexcept {
			auto& allocator = M_GetAllocator();
			auto& M_Data            = this->m_Data.data;
			pointer& start          = M_Data.start;
			pointer& finish         = M_Data.finish;
			pointer& end_of_storage = M_Data.end_of_storage;


			if (start != nullptr) {
				std::destroy_n(start, finish - start);
				allocator.deallocate(start, static_cast<size_type>(end_of_storage - start));

				start = nullptr;
				finish = nullptr;
				end_of_storage = nullptr;
			}
		}

	private:
		M_RealValueType m_Data;
	};
}

// #endif // _cplusplus > 202002L
#endif // ARRAY_HPP
