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
#include <optional>

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
			std::conditional_t<
				TypeTools::IsSimpleAllocVal<M_AllocatorType>,
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

		// @brief: 访问数组元素访问 API 
		[[nodiscard]] constexpr reference At(const size_type index) {}
		[[nodiscard]] constexpr const_reference At(const size_type index) const {}
		[[nodiscard]] constexpr reference Front(const size_type index) {}
		[[nodiscard]] constexpr const_reference Front(const size_type index) const {}
		[[nodiscard]] constexpr reference Back(const size_type index) {}
		[[nodiscard]] constexpr const_reference Back(const size_type index) const {}

		[[nodiscard]] constexpr size_type Find(const_reference item) const {}
		[[nodiscard]] constexpr size_type Find(const_reference item) {}
		template <typename Predicate>
		[[nodiscard]] constexpr size_type FindIf(Predicate pred) const {}
		template <typename Predicate>
		[[nodiscard]] constexpr size_type FindIf(Predicate pred) {}

		/**
		 * @brief: 函数式API, 它们都不会修改原有数组的值, 而是返回一个新的数组或者值
		 * 		- Filter: 会返回一个新的数组, 该数组包含所有满足谓词条件的元素.
		 *      - Transform 会返回一个新的数组, 内部元素均应用 FnTransform
		 *      - Count 会计算满足条件的元素数量并返回该数量.
		 *      - IsContain  会检查数组中是否包含满足条件的元素并返回布尔值.
		 *      - Sort 会返回一个新的数组, 该数组包含排序后的元素.
		 */
		template <typename Predicate>
		[[nodiscard]] constexpr Array Filter(Predicate pred) const {}
		template <typename Predicate>
		[[nodiscard]] constexpr Array Filter(Predicate pred) {}
		template <typename FnTransform>
		[[nodiscard]] constexpr Array<std::invoke_result_t<FnTransform, value_type>> Transform(FnTransform func) const {}
		template <typename FnTransform>
		[[nodiscard]] constexpr Array<std::invoke_result_t<FnTransform, value_type>> Transform(FnTransform func) {}

		[[nodiscard]] constexpr size_type Count(const_reference item) const {
			size_type count = 0;
			for (const auto& element : *this) {
				if (element == item) {
					++count;
				}
			}
			return count;
		}

		template <typename Predicate>
		[[nodiscard]] constexpr size_type Count(Predicate pred) const {}

		[[nodiscard]] constexpr bool IsContain(const_reference item) const { return true; }
		template <typename Predicate>
		[[nodiscard]] constexpr bool IsContain(Predicate pred) const { return true; }
		template <typename Compare>
		[[nodiscard]] constexpr Array Sort(Compare comp) { }

		template <typename Ty2>
		int IsContinuousSubSequence(const Array<Ty2>& sub) const noexcept;
		template <typename Ty2>
		bool IsSequence(const Array<Ty2>& sub) const noexcept;

		template <typename Ty2>
		Array Intersection(const Array<Ty2>& other) const noexcept;
		template <typename Ty2>
		Array Union(const Array<Ty2>& other) const noexcept;
		template <typename Ty2>
		Array Difference(const Array<Ty2>& other) const noexcept;

		constexpr iterator Insert(const_iterator position, const value_type& value) {
			return M_Emplace(position, 1, value);
		}
		constexpr iterator Insert(const_iterator position, value_type&& value) {
			return M_Emplace(position, 1, std::move(value));
		}
		constexpr iterator Insert(const_iterator position, size_type count, const value_type& value) {
			pointer ptr = M_InsertHole(position, count, false);
			std::fill_n(ptr, count, value);
			return iterator(ptr);
		}
		template <typename InputIterator>
		constexpr iterator Insert(const_iterator position, InputIterator first, InputIterator last) {
			if constexpr (std::is_base_of_v<std::forward_iterator_tag, typename std::iterator_traits<InputIterator>::iterator_category>) {
				const auto count = static_cast<size_type>(std::distance(first, last));
				pointer ptr = M_InsertHole(position, count, false);
				std::copy(first, last, ptr);
				return iterator(ptr);
			} else {
				// InputIterator cannot calculate count, so we can't use M_InsertHole easily without buffering or repeated inserts
				// Repeated inserts is the standard way for InputIterator
				difference_type offset = position - cbegin();
				for (; first != last; ++first) {
					position = Insert(begin() + offset, *first);
					offset++;
					position++; // Point to next slot
				}
				return begin() + offset; 
			}
		}
		constexpr iterator Insert(const_iterator position, std::initializer_list<value_type> list) {
			return Insert(position, list.begin(), list.end());
		}

		constexpr iterator InsertAt(size_type index, value_type& value) {
			return Insert(begin() + index, value);
		}
		constexpr iterator InsertAt(size_type index, const value_type& value) {
			return Insert(begin() + index, value);
		}
		constexpr iterator InsertAt(size_type index, size_type count, const value_type& value) {
			return Insert(begin() + index, count, value);
		}

		constexpr void InsertUninitializedItem(const_iterator position) {
			M_InsertHoles(position, 1, false);
		}
		constexpr void InsertUninitializedItem(const_iterator position, const size_type count) {
			M_InsertHoles(position, count, false);
		}
		constexpr reference InsertZeroedItem(const_iterator position) {
			return *M_InsertHoles(position, 1, true);
		}
		constexpr reference InsertZeroedItem(const_iterator position, const size_type count) {
			return *M_InsertHoles(position, count, true);
		}

		constexpr iterator InsertUnique(size_type index, value_type& value) {}
		constexpr iterator InsertUnique(size_type index, const value_type& value) {}

		/**
		 * @brief: Append系列API: 追加元素到数组末尾, 返回数组本身的引用, 支持链式调用
		 *  - 可以追加单个元素:
		 *   	- 以元素的形式追加: Append(const value_type& value)
		 *   	- 以右值的形式追加: Append(value_type&& value)
		 *      - 以参数包的形式追加: Append(Args&& ... args)
		 *	- 可以追加另一个数组的所有元素:
		 *      - 以右值的形式追加: Append(Array&& array)
		 *      - 以指针和数量的形式追加: Append(const_pointer ptr, size_type count)
		 *      - 以迭代器的形式追加: Append(InputIterator first, InputIterator last)
		 *      
		 */
		constexpr Array& Append(value_type&& value) {
			M_EmplaceBack(std::move(value));
			return *this;
		}
		constexpr Array& Append(const value_type& value) {
			M_EmplaceBack(value);
			return *this;
		}
		constexpr Array& Append(Array&& array) {
			if (this == &array || array.IsEmpty()) {
				return *this;
			}
			/* https://en.cppreference.com/w/cpp/iterator/move_iterator.html */
			M_AppendRange(std::make_move_iterator(array.begin()), array.M_Size());
			array.M_Clear();
			return *this;
		}
		constexpr Array& Append(const_pointer ptr, size_type count) {
			if (count == 0 || ptr == nullptr) {
				return *this;
			}
			M_AppendRange(ptr, count);
			return *this;
		}


		/**
		 * @note: Cpp 标准中, InputIterator 是单向迭代器不能使用 std::distance 计算距离
		 *     只有 ForwardIterator 及以上的迭代器才能使用 std::distance 计算距离.
		 *     
		 *		https://en.cppreference.com/w/cpp/header/iterator.html
		 *		https://en.cppreference.com/w/cpp/named_req/Iterator.html
		 */
		template <typename InputIterator>
			requires (!std::is_integral_v<InputIterator>)
		constexpr Array& Append(InputIterator first, InputIterator last) {
			if constexpr (std::is_base_of_v<std::forward_iterator_tag, typename std::iterator_traits<InputIterator>::iterator_category>) {
				const auto count = static_cast<size_type>(std::distance(first, last));
				M_AppendRange(first, count);
			} else {
				for (; first != last; ++first) {
					M_EmplaceBack(*first);
				}
			}
			return *this;
		}

		template <typename ...Args>
		constexpr Array& Append(Args&& ... args) {
			M_EmplaceBack(std::forward<Args>(args)...);
			return *this;
		}

		template <typename ... Args>
		constexpr iterator Emplace(const_iterator position, Args&& ... args) {
			return M_Emplace(position, 1, std::forward<Args>(args)...);
		}
		template <typename ... Args>
		constexpr iterator EmplaceAt(size_type index, Args&& ... args) {
			return M_Emplace(cbegin() + index, 1, std::forward<Args>(args)...);
		}

		template <typename ... Args>
		constexpr reference EmplaceBack(Args&& ... args) {
			return M_EmplaceBack(std::forward<Args>(args)...);
		}

		constexpr iterator Erase(const_iterator position){}
		constexpr iterator Erase(const_iterator first, const_iterator last){}
		constexpr iterator EraseAt(const size_type index){}
		constexpr std::shared_ptr<value_type> EraseAsShared(const size_type index){}
		constexpr std::unique_ptr<value_type> EraseAsUnique(const size_type index){}
		constexpr std::optional<value_type> EraseAsOptional(const size_type index){}
		template <typename Predicate>
		constexpr iterator EraseIf(Predicate pred){}


		/** 
		 * @brief: 栈语义的API: Push / Pop / Top ========================== 
		 */
		constexpr iterator Push(const value_type& value) {
			return M_Emplace(cend(), 1, value);
		}
		
		constexpr iterator Push(value_type&& value) {
			return M_Emplace(cend(), 1, std::move(value));
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
		constexpr void Swap(Array& other) noexcept {}
		
	
		bool IsEmpty() const noexcept {
			return M_Size() == 0;
		}
		size_type Size() const noexcept {
			return M_Size();
		}
		size_type Capacity() const noexcept {
			return M_Capacity();
		}
		size_type Slack() const noexcept {}

		void ShrinkToFit() noexcept {}

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

			M_InsertHoles(first, count, false);

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

		template <typename ForwardIterator>
		constexpr void M_RangeInitialize(ForwardIterator first, ForwardIterator last, std::forward_iterator_tag){
			const auto count = static_cast<size_type>(std::distance(first, last));
			if (count == 0) [[unlikely]] return;

			M_AllocateUninitializedMemory(count);
			
			CleanGuard Guard{ this };

			auto&    M_Data = this->m_Data.data;
			M_Data.finish = std::uninitialized_copy(first, last, M_Data.start);
			
			Guard.Release();
		}

		template <typename InputIterator>
		constexpr void M_RangeInitialize(InputIterator first, InputIterator last, std::input_iterator_tag){
			this->Append(first, last);
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
				ShrinkToFit(new_size);
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

			// 搬运旧数据: (old_start, count, new_start)
			M_TryUninitializedMove(start, old_size, new_arr);
			
			// 重要: 提交之前必须销毁旧对象
			if (start) {
				std::destroy_n(start, old_size);
				allocator.deallocate(start, static_cast<size_type>(end_of_storage - start));
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
		 * @param count: 重复插入的数量
		 * @return: 返回新插入元素的迭代器
		 */
		template <typename ... Args>
		constexpr iterator M_Emplace(const_iterator position, const size_t count, Args&& ...args) {
			auto& M_Data         = this->m_Data.data;
			const difference_type offset = position - cbegin();
			pointer insert_pos_ptr = M_Data.start + offset;

			if (count == 0) return iterator(insert_pos_ptr);

			// 1. Fast Path: Single Element Append (Zero Overhead)
			if (count == 1 && insert_pos_ptr == M_Data.finish && M_Data.finish != M_Data.end_of_storage) {
				std::allocator_traits<AllocatorType>::construct(
					M_GetAllocator(), 
					M_Data.finish, 
					std::forward<Args>(args)...
				);
				M_Data.finish++;
				return iterator(insert_pos_ptr);
			}

			// 2. Materialize Temporary
			// 必须先构造临时对象，因为：
			// a) 可能扩容/移动，导致 args 指向的源数据失效 (Aliasing)
			// b) count > 1 时，args 被使用多次，不能 forward
			// c) M_InsertHoles 会修改内存布局
			value_type temp(std::forward<Args>(args)...);

			// 3. Make Space
			// 注意: M_InsertHoles 现在会对非 Trivial 类型进行 Default Construct
			// 这意味着该区域已经是 Valid Object
			pointer ptr = M_InsertHoles(insert_pos_ptr, count, false);

			// 4. Fill Values
			// 使用 Assignment 而非 Construct，因为对象已存在
			// 如果 value_type 是 Trivial, Assignment 和 Construct 是一样的 (memcpy)
			// 如果 value_type 非 Trivial, M_InsertHoles 已构造了默认值，这里进行赋值覆盖
			if (count == 1) {
				if constexpr (std::is_move_assignable_v<value_type>) {
					*ptr = std::move(temp);
				} else {
					*ptr = temp;
				}
			} else {
				std::fill_n(ptr, count, temp);
			}

			return iterator(ptr);
		}
		
		/**
		 * @brief 内部通用的 EmplaceBack 方法
		 * @note 现已简化为 M_Emplace 的 Wrapper，逻辑已整合
		 */
		template <typename ... Args>
		constexpr reference M_EmplaceBack(Args&& ... args) {
			return *M_Emplace(cend(), 1, std::forward<Args>(args)...);
		}

		/**
		 * @brief: 重新分配内存并在指定位置插入任意多空洞
		 * @param pos: 插入位置
		 * @param count: 空洞数量
		 * @param is_zero_construct: 是否进行零初始化
		 * @return: 返回新内存中空洞的起始位置
		 *
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
		pointer M_InsertHoles(pointer pos, size_type count, bool is_zero_construct) {
			auto& allocator = M_GetAllocator();
			auto& M_Data    = this->m_Data.data;

			const size_type old_size     = M_Size();
			const size_type new_capacity = M_CalculateGrowth(old_size + count);
			
			const size_type forward_size = static_cast<size_type>(pos - M_Data.start);
			const size_type backward_size = static_cast<size_type>(M_Data.finish - pos);

			pointer new_start = allocator.allocate(new_capacity);
			pointer new_hole_start = new_start + forward_size;
			pointer new_backward_start = new_hole_start + count;
			pointer new_finish = new_backward_start + backward_size;

			ReallocateGuard Guard{
				.allocator = allocator,
				.new_start = new_arr,
				.new_capacity = new_capacity,
				.constructed_start = appended_start,
				.constructed_finish = appended_start
			};
			
			if constexpr (TypeTools::IsSimpleAllocVal<M_AllocatorType> && std::is_trivially_copyable_v<ElementType>) {
				if (forward_size > 0) std::memcpy(new_start, M_Data.start, forward_size * sizeof(ElementType));
				if (backward_size > 0) std::memcpy(new_backward_start, pos, backward_size * sizeof(ElementType));
				
				if (is_zero_construct) {
					std::memset(new_hole_start, 0, count * sizeof(ElementType));
				}
				// 即使不 zero construct，对于 trivial 类型，内存已经是分配状态，逻辑上可以说既是 initialized 也是 uninitialized
				Guard.Release();
			} else {
				// 1. Move前半部分
				M_TryUninitializedMove(M_Data.start, forward_size, new_start);
				Guard._c_finish = new_hole_start;

				// 2. 插入空洞
				if (is_zero_construct) {
					Guard._c_finish = std::uninitialized_value_construct_n(Guard._c_finish, count);
				} else {
					// 确保非 trivial 类型也被构造，避免 UB
					Guard._c_finish = std::uninitialized_default_construct_n(Guard._c_finish, count);
				}

				// 3. Move后半部分
				M_TryUninitializedMove(pos, backward_size, new_backward_start);
				Guard._c_finish = new_finish;
				Guard.Release();
			}

			if (M_Data.start) {
				std::destroy(M_Data.start, M_Data.finish);
				allocator.deallocate(M_Data.start, static_cast<size_type>(M_Data.end_of_storage - M_Data.start));
			}

			M_Data.start          = new_start;
			M_Data.finish         = new_finish;
			M_Data.end_of_storage = new_start + new_capacity;

			return new_hole_start;
		}

		pointer M_TryUninitializedMove(pointer old_start, size_type count, pointer new_start) {
			if constexpr (std::is_nothrow_move_constructible_v<ElementType> || !std::is_copy_constructible_v<ElementType>) {
				return std::uninitialized_move(old_start, old_start + count, new_start);
			} else {
				return std::uninitialized_copy(old_start, old_start + count, new_start);
			}
		}
		
		void M_Swap(pointer& lhs, pointer& rhs) noexcept {
			std::swap(lhs, rhs);
		}
		pointer M_EraseElement(pointer pos, const size_type count) {
			auto& M_Data = this->m_Data.data;

			pointer new_finish = std::move(pos + count, M_Data.finish, pos);

			std::destroy(new_finish, M_Data.finish);
			
			M_Data.finish = new_finish;
			return pos;
		}
	private:
		M_RealValueType m_Data;
	};

}

/**
 * @brief: 多数组交集运算
 * @return: 输出 Array 的类型是所有 Array::value_type 的 intersection type
 */
template <typename ... Arrays>
Array Intersection(const Arrays& ... arrays){

}

/**
 * @brief: 多数组并集运算
 * @return: 输出 Array 的类型是所有 Array::value_type 的 intersection type
 */
template <typename ... Arrays>
Array Union (const Arrays& ... arrays){

}

/**
 * @brief: array1 - array2 差集运算
 * @return: 输出 Array 的类型是 Ty1 和 Ty2 的 difference type
 *    如果不存在 common type, 则返回空 Array
 */
template <typename Ty1, typename Ty2>
Array Difference(const Arrays& array1, const Arrays& array2){

}

/**
 * @brief: 判断 sub 数组是否为 origin 数组的子序列
 * @tparam Ty1: origin 数组的元素类型
 * @tparam Ty2: sub 数组的元素类型
 * @note: Ty1 Ty2 必须存在公共类型, 且元素支持 operator==
 * @note: 这里的子序列并不要求是连续的, 只要保持相对顺序即可
 */
template <typename Ty1, typename Ty2>
	requires std::is_convertible_v<Ty2, Ty1>
bool IsSubSequence(const Array<Ty1>& origin, const Array<Ty2>& sub)


/**
 * @brief: 判断 sub 数组是否为 origin 数组的子序列
 * @tparam Ty1: origin 数组的元素类型
 * @tparam Ty2: sub 数组的元素类型
 * @return: 如果 sub 是 origin 的连续子序列, 则返回子序列的起始索引, 否则返回 -1
 * @note: Ty1 Ty2 必须存在公共类型, 且元素支持 operator==
 */
template <typename Ty1, typename Ty2>
	requires std::is_convertible_v<Ty2, Ty1>
int IsContinuousSubSequence(const Array<Ty1>& origin, const Array<Ty2>& sub);

bool operator==(const Array& lhs, const Array& rhs) noexcept {

}
bool operator!=(const Array& lhs, const Array& rhs) noexcept {

}
bool operator<=>(const Array& lhs, const Array& rhs) noexcept {

}

/**
 * @brief: 循环访问器, 用于循环访问数组中的元素
 */
struct Walker {
	/**
	* @brief: 循环访问元素 API: Next / Back 
	* @bote: 从第一次开始使用Next()/Back()开始, 会依次返回数组中的每一个元素,
	*     当访问到数组末尾时, 会从头开始继续访问, 形成一个循环访问的效果.
	*/
	[[nodiscard]] value_type Next() noexcept {}
	[[nodiscard]] value_type Back() noexcept {}
	[[nodiscard]] void SetCursor(int index) noexcept {}
	template <typename InputIterator>
	[[nodiscard]] void SetCursor(InputIterator position) noexcept {}

private:
	int m_Footprint {0};
	Array<ElementType>* m_World;
};
	


#endif // ARRAY_HPP
