#ifndef ARRAY_HPP
#define ARRAY_HPP


#include <memory>
#include <new>
#include <vector>
#include <compare>
#include <memory>
#include <print>
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
		using iterator_concept  = std::contiguous_iterator_tag;
		using iterator_category = std::random_access_iterator_tag;
		using value_type        = typename Array::value_type;
		using size_type         = typename Array::size_type;
		using difference_type   = typename Array::difference_type;
		using pointer           = typename Array::const_pointer;
		using reference         = const value_type&;

		template <typename> friend struct ArrayIterator;
		template <typename> friend struct ArrayConstIterator;
		

		using M_Ptr = typename Array::pointer;

		constexpr ArrayConstIterator() noexcept : ptr() {}
		constexpr explicit ArrayConstIterator(M_Ptr ptr) noexcept : ptr(ptr) {}

		[[nodiscard]] constexpr reference operator*() const noexcept {
			return *ptr;
		}
		[[nodiscard]] constexpr pointer operator->() const noexcept {
			return ptr;
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

		/* operator!= 可以被编译器通过 operator== 自动生成 */
		[[nodiscard]] constexpr bool operator==(const ArrayConstIterator& other) const noexcept {
			return ptr == other.ptr;
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

		using Base::operator-;
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

			pointer start { nullptr };           // 指向已分配内存起始位置
			pointer finish { nullptr };          // 指向已构造元素的末尾
			pointer end_of_storage { nullptr };  // 指向分配内存的末尾
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

		/**
		 * @note: std::allocate 时默认 noexcept 的, 但是标准没有要求自定义的 allocator
		 *    也必须时 noexcept 的, 所以这里不加 noexcept 关键字. 
		 *    如果 noexcept 的函数抛出异常则直接调用 std::terminate 终止程序. 
		 *
		 *    https://en.cppreference.com/w/cpp/language/noexcept_spec.html
		 *
		 */
		constexpr explicit Array(const AllocatorType& allocator)
			noexcept (std::is_nothrow_copy_constructible_v<AllocatorType>)
			: m_Data(MemoryTools::OneConstructCompressedTag{}, allocator) {}

		constexpr explicit Array(const size_type count, const AllocatorType& allocator=AllocatorType())
			noexcept (std::is_nothrow_copy_constructible_v<AllocatorType>)
			: m_Data(MemoryTools::OneConstructCompressedTag{}, allocator) {
			if (count > M_MaxSize()) [[unlikely]] {
				throw std::length_error("Array size exceeds maximum limit.");
			}
			M_FillZeroConstruct(count);
		}
		constexpr Array(size_type count, const_reference value, const AllocatorType& allocator=AllocatorType())
			noexcept (std::is_nothrow_copy_constructible_v<AllocatorType>) 
			: m_Data(MemoryTools::OneConstructCompressedTag{}, allocator) {
			if (count > M_MaxSize()) [[unlikely]] {
				throw std::length_error("Array size exceeds maximum limit.");
			}
			M_FillValueConstruct(count, value);
		}
		template <typename InputIterator>
			/* SFINAE: ,typename = std::enable_if_t<!std::is_integral_v<InputIterator>>*/
			requires (!std::is_integral_v<InputIterator>)
		constexpr Array(InputIterator first, InputIterator last, const AllocatorType& allocator=AllocatorType()) 
			noexcept (std::is_nothrow_copy_constructible_v<AllocatorType>)
			: m_Data(MemoryTools::OneConstructCompressedTag{}, allocator) {

			static_assert(
				std::is_integral_v<typename std::iterator_traits<InputIterator>::value_type> == false,
				"InputIterator should not be an integral type."
			);
			M_RangeInitialize(first, last, typename std::iterator_traits<InputIterator>::iterator_category{});
		}
		Array(std::initializer_list<value_type> list, const AllocatorType& allocator=AllocatorType())
			noexcept (std::is_nothrow_copy_constructible_v<AllocatorType>)
			: Array(list.begin(), list.end(), allocator) /* 委托构造 */ { }
		constexpr Array(const Array& other) {}
		constexpr Array(Array&& other) noexcept {}

	public:
		[[nodiscard]] constexpr Array& operator=(const Array& other) {}
		[[nodiscard]] constexpr Array& operator=(Array&& other) noexcept {}
		[[nodiscard]] constexpr Array& operator=(std::initializer_list<value_type> list) {}
		[[nodiscard]] constexpr reference operator[](const size_type index) noexcept {}
		[[nodiscard]] constexpr const_reference operator[](const size_type index) const noexcept {}
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

		[[nodiscard]] constexpr size_type Find(const_reference item) const {}
		[[nodiscard]] constexpr size_type Find(const_reference item) {}
		template <typename Predicate>
		[[nodiscard]] constexpr size_type FindIf(Predicate pred) const {}
		template <typename Predicate>
		[[nodiscard]] constexpr size_type FindIf(Predicate pred) {}

		template <typename Predicate>
		[[nodiscard]] constexpr Array Filter(Predicate pred) const {}
		template <typename Predicate>
		[[nodiscard]] constexpr Array Filter(Predicate pred) {}

		[[nodiscard]] constexpr size_type Count(const_reference item) const {
			return m_Data.finish - m_Data.start;
		}

		template <typename Predicate>
		[[nodiscard]] constexpr size_type Count(Predicate pred) const {}

		[[nodiscard]] constexpr bool IsContain(const_reference item) const { return true; }
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

		/** 
		 * @brief: 栈语义的API: Push / Pop / Top ========================== 
		 */
		constexpr iterator Push(const value_type& value) {
			return M_Emplace(cend(), value);
		}
		
		constexpr iterator Push(value_type&& value) {
			return M_Emplace(cend(), std::move(value));
		}

		/**
		 * @brief 移除数组末尾的元素
		 * @note: 强异常安全性(Strong Exception Guarantee).
		 *     Pop 和 Top 的分开是 Cpp 标准库的实现者为了保证强异常安全的涉及. 见:
		 *
		 *       https://www.boost.org/doc/user-guide/exception-safety.html
		 *       https://stackoverflow.com/questions/25035691/why-doesnt-stdqueuepop-return-value
		 *     
		 * 简单来说, Top() 是可以发生异常的, 内存耗尽或者临时对象构造失败都会抛出异常,
		 * 如果 Pop() 和 Top() 合并成一个操作, 当 Top() 抛出异常时, 同时元素已经被移除,
		 * 这就违反了强异常安全性的要求: **要么操作成功, 要么容器状态不变**.
		 *
		 * > More formally, we can describe a component as minimally 
		 * > exception-safe if, when exceptions are thrown from within that 
		 * > component, its invariants are intact. 
		 * 	
		 * T Pop() {
		 * 		T result = data.back(); // 1. Copy: 可能抛异常 (Strong Guarantee)
		 * 		data.pop_back();        // 2. Remove: 必须不抛异常
		 * 		return result;          // 3. Return: 可能涉及 Copy/Move 可能抛异常
		 * }
		 * 如果 3. 抛出异常, 则数据没有的同时也了抛出了异常
		 * 
		 * 其他语言之所以可以实现, 因为其底层的对象模型都是以指针,引用的形式进行的. 
		 * 而指针和引用都是轻量级的, 复制和移动都不会抛出异常. 来回移动的只是指针, 真正
		 * 的对象并没有被移动. C++ 之所以不能这样做, 是因为 C++ 的对象模型是值语义的,
		 * 复制和移动都可能涉及到深拷贝, 这就可能抛出异常. 
		 * 在 Rust 中, 编译器保证数据的所有权从 Vec 转移到了返回, 如果 move 过程中出错
		 * 程序会直接 panic, 而不是抛出异常
		 */
		constexpr void Pop() noexcept {
			assert(!IsEmpty() && "Array::Pop(): Array is empty");
			auto& M_Data    = this->m_Data.data;
			pointer& finish = M_Data.finish;
			if (finish > M_Data.start) {
				--finish;
				std::allocator_traits<AllocatorType>::destroy(M_GetAllocator(), finish);
			}
		}

		[[nodiscard]] constexpr reference Top() noexcept {
			assert(!IsEmpty() && "Array::Top(): Array is empty");
			return *(end() - 1);
		}

		[[nodiscard]] constexpr const_reference Top() const noexcept {
			assert(!IsEmpty() && "Array::Top(): Array is empty");
			return *(end() - 1);
		}

		constexpr void Resize(size_type count) {}
		constexpr void Resize(size_type count, const value_type& value) {}
		constexpr void Reserve(size_type capacity) {}
		constexpr void Swap() noexcept {}

		bool IsEmpty() const noexcept {
			return M_Size() == 0;
		}
		size_type Size() const noexcept {}
		size_type Capacity() const noexcept {}
		size_type Slack() const noexcept {}

		void Shrink() noexcept {}

	public:
		[[nodiscard]] constexpr iterator begin() noexcept {
			return iterator(m_Data.data.start);
		}
		[[nodiscard]] constexpr const_iterator begin() const noexcept {
			return const_iterator(m_Data.data.start);
		}
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept {
			return const_iterator(m_Data.data.start);
		}

		[[nodiscard]] constexpr iterator end() noexcept {
			return iterator(m_Data.data.finish);
		}
		[[nodiscard]] constexpr const_iterator end() const noexcept {
			return const_iterator(m_Data.data.finish);
		}
		[[nodiscard]] constexpr const_iterator cend() const noexcept {
			return const_iterator(m_Data.data.finish);
		}

		[[nodiscard]] constexpr reverse_iterator rbegin() noexcept {
			return reverse_iterator(end());
		}
		[[nodiscard]] constexpr reverse_const_iterator rbegin() const noexcept {
			return reverse_const_iterator(end());
		}
		[[nodiscard]] constexpr reverse_const_iterator crbegin() const noexcept {
			return reverse_const_iterator(cend());
		}

		[[nodiscard]] constexpr reverse_iterator rend() noexcept {
			return reverse_iterator(begin());
		}
		[[nodiscard]] constexpr reverse_const_iterator rend() const noexcept {
			return reverse_const_iterator(begin());
		}
		[[nodiscard]] constexpr reverse_const_iterator crend() const noexcept {
			return reverse_const_iterator(cbegin());
		}

	private:

		/**
		 * @brief 清理容器内存的守卫, 用于在异常情况下自动清理容器内存
		 * @usage: 
		 * 	CleanGuard Guard{ this };
		 *         you code ...
		 *     Guard.Release();
		 *
		 *     其工作原理时, 当异常发生时, 会自动调用容器的 M_Clear() 方法清理内存,
		 *     Eg: 在内存分配的时候, 如果构造函数抛出异常, 则异常会一直抛出至 Array 
		 *         的构造函数中, 同时 C++ 标准规则构造函数抛出异常时, 不会调用析构函数
		 *         所以此时就发生了内存泄漏, 使用 CleanGuard 可以避免这种情况的发生.
		 *
		 *     C++ 的生命周期规则认为: 只有当构造函数完整执行完毕, 即 代码执行到了右花
		 *     括号 } 并没有异常，一个对象才被视为"已构造(Fully Constructed)". 
		 *     如果在构造函数体内发生了异常, 那么该对象就不会被视为已构造, 因此其析构函
		 *     数不会被调用. 
		 *     
		 * cppreference.com 的解释: 
		 *     https://en.cppreference.com/w/cpp/language/lifetime.html
		 *     https://en.cppreference.com/w/cpp/language/destructor.html
		 *     https://en.cppreference.com/w/cpp/language/throw.html#Stack_unwinding
		 */
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

			if (count == 0) [[unlikely]] {
				return ;
			}

			pointer mem = M_GetAllocator().allocate(count);

			start           = mem;
			finish          = mem;
			end_of_storage  = mem + count;
		}
		/**
		 * @brief 拷贝填充构造 -> 对应构造函数 Array(n) -> 其会默认进行零初始化或者默认构造
		 * @param count 元素个数
		 */
		constexpr void M_FillZeroConstruct (const size_type count) {
			if (count == 0) [[unlikely]] return;

			auto& allocator = M_GetAllocator();
			auto& M_Data    = this->m_Data.data;
			pointer& start  = M_Data.start;
			pointer& finish = M_Data.finish;

			this->M_AllocateUninitializedMemory(count);
			CleanGuard Guard{ this };
			finish = std::uninitialized_value_construct_n(start, count);
			Guard.Release();
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
			pointer& finish = M_Data.finish;

			this->M_AllocateUninitializedMemory(count);
			CleanGuard Guard{ this };
			finish = std::uninitialized_fill_n(start, count, value);
			Guard.Release();
		}

		template <typename FoewardIterator>
		constexpr void M_RangeInitialize(ForwardIterator first, ForwardIterator last, std::forward_iterator_tag){
			const auto count = static_cast<size_type>(std::distance(first, last));
			if (count == 0) [[unlikely]] return;

			M_AllocateUninitializedMemory(count);
			CleanGuard Guard{ this };

			auto&    M_Data = this->m_Data.data;
			pointer& finish = M_Data.finish;

			finish = std::uninitialized_copy(first, last, M_Data.start);
			Guard.Release();
		}

		template <typename InputIterator>
		constexpr void M_RangeInitialize(InputIterator first, InputIterator last, std::input_iterator_tag){
			for (; first != last; ++first){

			}
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
			auto&    allocator      = M_GetAllocator();
			auto&    M_Data         = this->m_Data.data;
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

		/**
		 * @brief 内部通用的 Emplace 方法, 其是一个通用的插入器, 
		 * @       可以同时用于 Copy 和 Move 的场景  
		 *       其核心操作:
		 *            std::allocator_traits<AllocatorType>::construct(
		 *            	alloc, 
		 *            	ptr, 
		 *            	std::forward<Args>(args)...
		 *            );
		 *      等价于 new (ptr) T(std::forward<Args>(args)...);
		 *       1. 传入一个已经存在的对象: const value_type& obj -> 拷贝构造
		 *       2. 传入一个将要被销毁的对象: value_type&& obj -> 移动构造
		 *  	 3. 传入构造参数列表: Args&& ...args -> 完美转发构造
		 * @param position: 插入位置(必须有效, 由调用者保证)
		 * @param ...args: 构造参数
		 * @return: 返回新插入元素的迭代器
		 */
		template <typename ... Args>
		constexpr iterator M_Emplace(const_iterator position, Args&& ...args){
			auto&    M_Data         = this->m_Data.data;
			pointer& start          = M_Data.start;
			pointer& finish         = M_Data.finish;
			pointer& end_of_storage = M_Data.end_of_storage;

			const auto offset = static_cast<difference_type>(position - cbegin());
			pointer insert_pos_ptr = start + offset;
			
			assert(insert_pos_ptr >= start && insert_pos_ptr <= finish && "Array::M_Emplace(const_iterator, Args&& ...args) Runtime Error: position iterator out of range - 插入位置迭代器越界");

			// Case1: 还有多余空间, 也是最常见的情况
			if (finish != end_of_storage) [[likely]] {
				// Case1.1: 直接在末尾插入元素, 无需搬运数据, 直接就地构造
				if (insert_pos_ptr == finish) {
					std::allocator_traits<AllocatorType>::construct(
						M_GetAllocator(),
						std::addressof(*finish),
						std::forward<Args>(args)...
					);
					++finish;
				}

				// Case1.2: 需要搬运数据
				else {
				/**
				 *   @note: 这里构造 temp 是自引用安全性 
				 *   		args 可能是容器内部元素的引用 (Aliasing)
				 *   	    这意味着我们不能先移动数据覆盖旧值, 再用旧引用去构造, 
				 *   	    必须先构造一个临时对象保存值
				 *   @example: 
				 *   		Array<string> arr = {'a', 'b', 'c'};
				 *       	arr.Emplace(arr.begin(), arr[1]); 
				 *  
				 *   如果此时没有先构造临时对象, 在移动完数据之后 arr[1] 就已经被
				 *   掏空的状态(moved-from), 导致插入的不是 'b' 而是一个空字符串
				 *  
				 *   - 对于拷贝构造 (左值引用): 它创建了一份副本. 即使原数据在搬运
				 *   过程中被覆盖或移动，副本依然有效
				 *   - 对于移动构造 (右值引用): 虽然右值引用通常不涉及自引用(即将销毁)
				 *   但为了逻辑统一和安全，先具象化(Materialize)这个对象，再进行内部
				 *   内存的洗牌
				 */
				 *  
					value_type temp(std::forward<Args>(args)...);

					std::allocator_traits<AllocatorType>::construct(
						M_GetAllocator(),
						std::addressof(*(finish)),
						std::move(*(finish - 1))
					);
					++finish;

					// 将 [pos, finish-2] 整体向后移动一步到 [pos+1, finish-1]
					// finish 已经自增过了，原最后一个元素位于 finish-2
					std::move_backward(insert_pos_ptr, finish - 2, finish - 1);

					*insert_pos_ptr = std::move(temp);
				}
			}
			// Case2: 没有多余空间, 需要重新分配内存(扩容)
			else {
				M_RellocateAndEmplace(insert_pos_ptr, std::forward<Args>(args)...);
			}
			return begin() + offset;
		}

		/**
		 *       start 			finish == end_of_storage
		 *	old: | - - - - - o - - - - - | 	
		 *                     ^
		 *			    insert_pos_ptr (overflow)
		 *
		 *	After CalculateGrowth: (2times growth)
		 *       new_start            new_finish         new_end_of_storage
		 *	new: | - - - - - o - - - - - * - - - - - - - - - - | 	
		 *					 ^
		 *			  new_insert_pos_ptr	 	
		 *
		 */
		template <typename ... Args>
		constexpr void M_RellocateAndEmplace(pointer insert_pos_ptr, Args&& ...args){
			auto&	allocator      = M_GetAllocator();
			auto&   M_Data         = this->m_Data.data;
			
			// step1: 计算新容量
			const size_type old_size = M_Size();
			const size_type new_capacity = M_CalculateGrowth(old_size + 1);

			pointer new_start = allocator.allocate(new_capacity);
			pointer new_finish = new_start;

			const difference_type insert_offset = insert_pos_ptr - M_Data.start;
			pointer new_insert_pos_ptr = new_start + insert_offset;

			// step2: 创建新元素
			try {
				// 强异常保证
				std::allocator_traits<AllocatorType>::construct(
					allocator,
					std::addressof(*new_insert_pos_ptr),
					std::forward<Args>(args)...
				);
			} catch (...) {
				allocator.deallocate(new_start, new_capacity);
				throw;
			}

			// step3: 搬运旧数据
			try {
				// 搬运前半部分
				new_finish = std::uninitialized_move(
					M_Data.start,
					insert_pos_ptr,
					new_start
				);

				// 跳过刚刚插入之后的数据
				++new_finish; 

				// 搬运后半部分
				new_finish = std::uninitialized_move(
					insert_pos_ptr,
					M_Data.finish,
					new_finish
				);
			} catch (...) {
				/**
			     * - 如果搬运过程中抛出异常，销毁新内存中已经构造好的所有对象, 此时 
			     *   new_finish 指向最后尝试构造的位置，或者刚构造完的位置. 
			     * - 如果是 construct(new_pos) 之后的 uninitialized_move 抛出, 则
			     *   需要销毁 new_pos 处的对象
			     * - 如果是在前半段搬运途中失败，new_finish 指向搬運到的位置 destroy 
			     *   会销毁它们. 同时也需要销毁那个单独构造的 new_pos_ptr 处的元素
			     */
                std::destroy(new_start, new_finish); 
                if (new_finish <= new_pos_ptr) {
                    // 意味着异常发生在前半段或者刚好 construct 完
                    std::allocator_traits<AllocatorType>::destroy(allocator, new_pos_ptr);
                }
                allocator.deallocate(new_start, new_capacity);
                throw;
			}

			// step4: 释放旧内存
			std::destroy(M_Data.start, M_Data.finish);
			allocator.deallocate(M_Data.start, static_cast<size_type>(M_Data.end_of_storage - M_Data.start));

			// step5: 更新数据指针
			M_Data.start          = new_start;
			M_Data.finish         = new_finish;
			M_Data.end_of_storage = new_start + new_capacity;
		} 
	private:
		M_RealValueType m_Data;
	};
}

// #endif // _cplusplus > 202002L
#endif // ARRAY_HPP
