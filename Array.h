#ifndef ARRAY_HPP
#define ARRAY_HPP
// #if __cplusplus >= 202002L

#include <memory>
#include <new>
#include <vector>
#include <compare>
#include <type_traits>
#include <initializer_list>
#include <memory>
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
		struct [[nodiscard]] TidyGuard {
			Container* container;
			constexpr ~TidyGuard() {
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

		/**
		 * @brief 批量构造元素
		 * @param start: 起始位置
		 * @param count: 构造数量
		 * @param allocator: 分配器
		 */
		template <typename Alloc>
		constexpr AllocPointer<Alloc> BatchOfConstruction(AllocPointer<Alloc> start, AllocSize<Alloc> count, Alloc& allocator){
			using pointer = typename Alloc::value_type*;
			using value_type = std::remove_pointer_t<std::remove_cv_t<pointer>>;
			/* memset 优化 */
			if constexpr (std::is_trivially_default_constructible_v<value_type> && std::is_trivially_copyable_v<value_type>) {
#if defined(__cpp_lib_is_constant_evaluated) || (defined(__cpp_consteval) || (__cpp_lib_is_constant_evaluated >= 201907L))
				if (!std::is_constant_evaluated())
#endif
				{
					auto first = Unfancy(start);
					char* const first_ = reinterpret_cast<char*>(first);
					char* const last_ = reinterpret_cast<char*>(first_ + count);
					std::memset(first_, 0, static_cast<std::size_t>(last_ - first_));
					return start + count;
				}
			}

			std::uninitialized_value_construct_n(Unfancy(start), count);
			return start + count;
		}
	};

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
			: M_start()
			, M_finish()
			, M_End_Of_Storage(){}

		constexpr ArrayData(pointer start, pointer finish, pointer end) noexcept
			: M_start(start)
			, M_finish(finish)
			, M_End_Of_Storage(end){}

		constexpr void CopyData(ArrayData const& other) noexcept(true) {
			M_start = other.M_start;
			M_finish = other.M_finish;
			M_End_Of_Storage = other.M_End_Of_Storage;
		}

		constexpr void SwapData(ArrayData& other) noexcept {
			std::swap(M_start, other.M_start);
			std::swap(M_finish, other.M_finish);
			std::swap(M_End_Of_Storage, other.M_End_Of_Storage);
		}

		constexpr void StealData(ArrayData& other) noexcept {
			M_start = other.M_start;
			M_finish = other.M_finish;
			M_End_Of_Storage = other.M_End_Of_Storage;

			other.M_start = nullptr;
			other.M_finish = nullptr;
			other.M_End_Of_Storage = nullptr;
		}

		pointer M_start;
		pointer M_finish;
		pointer M_End_Of_Storage;
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

	template <typename Array>
	struct ArrayConstIterator {};

	template <typename Array>
	struct ArrayIterator : public ArrayConstIterator<Array>{};


	template <typename ElementType, class AllocatorType=std::allocator<ElementType>>
	class Array {
	private:
		using M_AllocatorType =
		typename std::allocator_traits<AllocatorType>
			::template rebind_alloc<ElementType>;
		using M_AllocatorTraits = std::allocator_traits<M_AllocatorType>;

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
		constexpr explicit Array(size_type count, const AllocatorType& allocator) {}
		constexpr Array(size_type count, const reference value, const AllocatorType& allocator) {}
		template <typename InputIterator>
		constexpr Array(InputIterator first, InputIterator last, const AllocatorType& allocator) {}
		Array(std::initializer_list<value_type> list, const AllocatorType& allocator){}
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
		struct [[nodiscard]] ReallocateGuard {
			AllocatorType& allocator;
			pointer new_start;
			size_type new_capacity;
			pointer constructed_start;
			pointer constructed_finish;

			ReallocateGuard& operator=(const ReallocateGuard& other) = delete;
			ReallocateGuard(const ReallocateGuard& other) = delete;

			constexpr ~ReallocateGuard() noexcept {
				if (new_start != nullptr) {
					MemoryTools::DestroyRange(constructed_start, constructed_finish, allocator);
					allocator.deallocate(new_start, new_capacity);
				}
			}
		};
		struct [[nodiscard]] SimpleReallocateGuard {
			AllocatorType& allocator;
			pointer new_start;
			size_type new_capacity;

			SimpleReallocateGuard& operator=(const SimpleReallocateGuard& other) = delete;
			SimpleReallocateGuard(const SimpleReallocateGuard& other) = delete;

			constexpr ~SimpleReallocateGuard() noexcept {
				if (new_start != nullptr) {
					allocator.deallocate(new_start, new_capacity);
				}
			}
		};
	private:
		template <typename ... ConstructParams>
		constexpr void M_Construct(const size_type count, ConstructParams&& ... args){

		}

		constexpr size_type M_CalculateGrowth(const size_type new_size) const {
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
			pointer& start          = M_Data.M_start;
			pointer& finish         = M_Data.M_finish;
			pointer& end_of_storage = M_Data.M_End_Of_Storage;

			return start ? static_cast<size_type>(end_of_storage - start) : 0u;
		}

		[[nodiscard]] constexpr size_type M_Size() const noexcept {
			auto& M_Data            = this->m_Data.data;
			pointer& start          = M_Data.M_start;
			pointer& finish         = M_Data.M_finish;
			pointer& end_of_storage = M_Data.M_End_Of_Storage;

			return start ? static_cast<size_type>(finish - start) : 0u;
		}

		[[nodiscard]] constexpr bool M_IsEmpty() const noexcept {
			return M_Size() == 0;
		}

		/**
		* @brief 分配未初始化内存, 用于构造时申请内存
		* @param capacity 分配的容量
		*/
		constexpr void M_AllocateUninitializedMemory(size_type capacity){
			auto& M_Data            = this->m_Data.data;
			pointer& start          = M_Data.M_start;
			pointer& finish         = M_Data.M_finish;
			pointer& end_of_storage = M_Data.M_End_Of_Storage;

			assert(capacity != 0 && "M_AllocateNonZeroUninitializedMemory Runtime Error: capacity can not be zero.");
			assert(start != nullptr && finish != nullptr && end_of_storage != nullptr && "M_AllocateNonZeroUninitializedMemory Runtieme Error: memory has been allocated.");

			const pointer mem = M_GetAllocator().allocate(capacity);
			start           = mem;
			finish          = mem;
			end_of_storage  = mem + capacity;
		}

		/**
		* @brief 清除所有已从start开始, 往后count个元素, 释放调用析构函数
		*/
		constexpr void M_DestroyRange(pointer start, const size_type count) noexcept {
			auto& allocator = M_GetAllocator();
			auto& M_Data            = this->m_Data.data;
			pointer& start_          = M_Data.M_start;
			pointer& finish_         = M_Data.M_finish;
			pointer& end_of_storage_ = M_Data.M_End_Of_Storage;
		}

		[[nodiscard]] constexpr const M_AllocatorType& M_GetAllocator() const noexcept {
			return m_Data.GetFirst();
		}
		[[nodiscard]] constexpr M_AllocatorType& M_GetAllocator() noexcept {
			return m_Data.GetFirst();
		}

		template <typename OtherTy>
		constexpr void M_Reallocate(const size_type new_size, const OtherTy& val) {
			auto& allocator = M_GetAllocator();
			auto& M_Data            = this->m_Data.data;
			pointer& start          = M_Data.M_start;
			pointer& finish         = M_Data.M_finish;
			pointer& end_of_storage = M_Data.M_End_Of_Storage;

			const auto old_size = static_cast<size_type>(finish - start);
			size_type new_capacity = M_CalculateGrowth(new_size);

			const pointer new_array_start = allocator.allocate(new_capacity);
			const pointer new_array_appended_first = new_array_start + old_size;

			ReallocateGuard Guard{
				.allocator = allocator,
				.new_start = new_array_start,
				.new_capacity = new_capacity,
				.constructed_start = new_array_start,
				.constructed_finish = new_array_start
			};

			auto& new_array_appended_finish = Guard.constructed_finish;


		}

	private:
		M_RealValueType m_Data;
	};
}

// #endif // _cplusplus > 202002L
#endif // ARRAY_HPP