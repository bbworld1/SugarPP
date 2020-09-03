#pragma once
#include <type_traits>
#include <utility>
#include <cstring>  //for strcmp()
#include <utility>

template <typename lhsType, typename rhsType, typename = void>
struct comparable : std::false_type
{
};

template <typename lhsType, typename rhsType>
struct comparable<lhsType, rhsType, decltype((std::declval<lhsType>() == std::declval<rhsType>()), void())>
    : std::true_type
{
};

/*Dummy structs representing logical relation used by [GroupedExpression]*/
class Operator {
public:
};
/**
 * @brief A dummy struct for logic NOT
*/
template<typename Case>
class NOT : public Operator
{
    Case caseExpr;
public:
    NOT(Case&& expr) :caseExpr(std::move(expr)) {}

    template<typename Expr, typename = std::enable_if_t<comparable<Expr, Case>::value>>
    bool operator==(Expr&& expr) const
    {
        return expr != caseExpr;
    }
};

/**
 * @brief A dummy struct for logic AND
*/
template<typename Case1, typename Case2>
class AND : public Operator
{
    Case1 caseExpr1;
    Case2 caseExpr2;
public:
    AND(Case1&& expr1, Case2&& expr2) :caseExpr1(std::move(expr1)), caseExpr2(std::move(expr2)) {}

    template<typename Expr, typename = std::enable_if_t<comparable<Expr, Case1>::value&& comparable<Expr, Case2>::value>>
    bool operator==(Expr&& expr) const
    {
        return (caseExpr1 == expr) && (caseExpr2 == expr);
    }

    template<typename Expr, typename Case1, typename Case2, typename = std::enable_if_t<comparable<Expr, Case1>::value&& comparable<Expr, Case2>::value>>
    friend bool operator==(Expr&& expr, AND<Case1, Case2>&& andCase)
    {
        return andCase == expr;
    }
};

/**
 * @brief A dummy struct for logic OR
*/
template<typename Case1, typename Case2>
struct OR
{
    Case1 caseExpr1;
    Case2 caseExpr2;
public:
    OR(Case1&& expr1, Case2&& expr2) :caseExpr1(std::move(expr1)), caseExpr2(std::move(expr2)) {}

    template<typename Expr, typename = std::enable_if_t<comparable<Expr, Case1>::value&& comparable<Expr, Case2>::value>>
    bool operator==(Expr&& expr) const
    {
        return (caseExpr1 == expr) || (caseExpr2 == expr);
    }

    template<typename Expr, typename Case1, typename Case2, typename = std::enable_if_t<comparable<Expr, Case1>::value&& comparable<Expr, Case2>::value>>
    friend bool operator==(Expr&& expr, OR<Case1, Case2>&& orCase)
    {
        return orCase == expr;
    }
};


/*For range matching */
template <typename T, typename T2 = int, typename T3 = typename std::conditional<std::is_integral<T>::value&& std::is_floating_point<T2>::value, double, int>::type>
class Range
{
    T3 start;
    T3 current;
    T3 end_v;
    const T2 step;

public:
    explicit Range(T start, T end, T2 step = 1) : start(static_cast<T3>(start)), current(static_cast<T3>(start)), end_v(static_cast<T3>(end)), step(step) {}
    class RangeIterator
    {
        T3* ptr;
        const T2& step;

    public:
        RangeIterator(T3* ptr, const T2& step) : ptr(ptr), step(step) {}
        RangeIterator& operator++()
        {
            (*ptr) += step;
            return *this;
        }
        T3 operator*() { return (*ptr); }
        bool operator!=(const RangeIterator& iter) { return abs((*ptr)) <= abs(*(iter.ptr)); }
    };
    auto begin() { return RangeIterator(&current, step); }
    auto end() { return RangeIterator(&end_v, step); }
    
    /*Added from my [HavingFun] repo to support range matching*/
    template <typename Num, typename = std::enable_if_t<std::is_arithmetic_v<Num>>>
    friend bool operator==(Num number, Range const& rhs)
    {
        return number >= rhs.start && number <= rhs.end_v ? true : false;
    }

    template<typename Num,typename = std::enable_if_t<std::is_arithmetic_v<Num>>>
    bool operator==(Num number) const
    {
        return number == (*this);
    }
};

/**********************************************************************/
/*Just a dummy type for the Else() case */
struct Else
{
};

/**
 * @brief Dummy struct for is<> and is_not<> type query
 */
template <typename l>
struct is
{
    using type = std::remove_reference_t<l>;
};
template<typename l>
struct is_not
{
    using type = std::remove_reference_t<l>;
};


/**************C++20 concept and abbreviated function template implementation *****************/

#if __cplusplus > 201709L
template <typename T1, typename T2>
constexpr bool comparable = requires(T1 const& lhs, T2 const& rhs)
{
    lhs == rhs;
};

/**
 * @brief The non-Else Final Case Primary template
 *
 * @param expr: the expression to match
 * @param to_match: the case to match with expression
 * @param ReturnResult: the return expression if [expr] matches [to_match]
 * @return [ReturnResult] if matches, else return the default constructed object the same type as [expr]
 */
auto when(auto&& expr, auto&& to_match, auto&& ReturnResult)
{
    using ReturnType = std::remove_reference_t<decltype(ReturnResult)>;
    /*Handle the special case when [to_match] is already a boolean type */
    if constexpr (std::is_same_v<std::remove_reference_t<decltype(to_match)>, bool>)
    {
        if (to_match)
            return ReturnResult;
    }
    /*If [to_match] is something comparable to [expr]*/
    if constexpr (comparable<decltype(expr), decltype(to_match)>)
    {
        if (to_match == expr)
            return ReturnResult;
    }
    return ReturnType{};
}

/**
 * @brief The final Else case
 *
 * @param auto&&: place holder for non-used [expression]
 * @param ReturnResult: return value for Else case
 */
auto when(auto&&, Else, auto&& ReturnResult)
{
    return ReturnResult;
}

/**
 * @brief The special case for handling is<SomeType> expression
 *
 * @tparam is_type: the type query for whether [expr] has the same type
 * @param expr: the expression to match
 * @param ReturnResult: the return expression if [expr] has the same type as [is_type]
 */
template <typename is_type>
auto when(auto&& expr, is<is_type>, auto&& ReturnResult)
{
    if constexpr (std::is_same_v<decltype(expr), typename is<is_type>::type>)
        return ReturnResult;
    return decltype(ReturnResult){};
}
template <typename is_not_type>
auto when(auto&& expr, is_not<is_not_type>, auto&& ReturnResult)
{
    if constexpr (!std::is_same_v<decltype(expr), typename is_not<is_not_type>::type>)
        return ReturnResult;
    return decltype(ReturnResult){};
}

/**
 * @brief Special case for handling C string
*/
auto when(const char* expr, const char* to_match, auto&& ReturnResult)
{
    if (strcmp(expr) == strcmp(to_match))
        return ReturnResult;
    return decltype(ReturnResult){};
}


/**
 * @brief primary recursive template
*/
auto when(auto&& expr, auto&& case1, auto&& return1, auto&& case2, auto&&... args)
{
    if constexpr (std::is_same_v<decltype(case1), bool>)
    {
        if (case1)
            return return1;
    }
    if constexpr (comparable<decltype(expr), decltype(case1)>)
    {
        if (case1 == expr)
            return return1;
        else
            return when(expr, case2, args...);
    }
    return when(expr, case2, args...);
}
/* A note: It seems we don't need to forward the argument here. Because no copy/move is performed when I tried:
    auto func2(auto&&){}
    auto func1(auto&& object) { return func2(object); }
    The object won't get moved nor copied.
*/


/**
 * @brief Special case for handling C string
*/
auto when(const char* expr, const char* case1, auto&& return1, auto&& case2, auto&&... args)
{
    if (strcmp(expr) == strcmp(case1))
        return return1;
    else
        return when(expr, case2, args...);
}


/**
 * @brief Special case for handling is<> and is_not<> type query
*/
template <typename is_type>
auto when(auto&& expr, is<is_type>, auto&& return1, auto&& case2, auto&&... args)
{
    if constexpr (std::is_same_v<decltype(expr), typename is<is_type>::type>)
        return return1;
    return when(expr, case2, args...);
}
template <typename is_not_type>
auto when(auto&& expr, is_not<is_not_type>, auto&& return1, auto&& case2, auto&&... args)
{
    if constexpr (!std::is_same_v<decltype(expr), typename is_not<is_not_type>::type>)
        return return1;
    return when(expr, case2, args...);
}

#else
/*****************************C++17 template implementation ***************************************/

/**
 * @brief The comparable "concept" struct for determining whether two type can be compared using "=="
*/
//template <typename lhsType, typename rhsType, typename = void>
//struct comparable : std::false_type
//{
//};
//
//template <typename lhsType, typename rhsType>
//struct comparable<lhsType, rhsType, decltype((std::declval<lhsType>() == std::declval<rhsType>()), void())>
//    : std::true_type
//{
//};

template<size_t N, typename...T>
struct TypeHolder;

template<typename T0, typename...T>
struct TypeHolder<0, T0, T...>
{
    using type = T0;
};

template<size_t N, typename T0, typename...T>
struct TypeHolder<N, T0, T...>
{
    using type = typename TypeHolder<N - 1, T...>::type;
};

template<size_t Limit, size_t I, typename...Args>
constexpr auto isSameReturnType(Args&...args) ->std::enable_if_t < I >= Limit, bool>
{
    return true;
}

template<size_t Limit, size_t I, typename...Args>
constexpr auto isSameReturnType(Args&...args)->std::enable_if_t < I<Limit, bool>
{
    if constexpr (std::is_same_v <typename TypeHolder<I + 2, Args...>::type, typename TypeHolder<I, Args...>::type>)
        return isSameReturnType<Limit, I + 2>(args...);
    return false;
}

/**
 * @brief The non-Else Final Case Primary template
 *
 * @param expr: the expression to match
 * @param to_match: the case to match with expression
 * @param ReturnResult: the return expression if [expr] matches [to_match]
 * @return [ReturnResult] if matches, else return the default constructed object the same type as [expr]
 */
template <typename ExprType, typename CaseType, typename ReturnType>
auto when(ExprType&& expr, CaseType&& to_match, ReturnType&& ReturnResult)
{
    /*Handle the special case when [to_match] is already a boolean type */
    if constexpr (std::is_same_v<std::remove_reference_t<CaseType>, bool>)
    {
        if (to_match)
            return ReturnResult;
    }
    /*If [to_match] is something comparable to [expr]*/
    if constexpr (comparable<ExprType, CaseType>::value)
    {
        if (to_match == expr)
            return ReturnResult;
    }
    return std::remove_reference_t<ReturnType>{};
}
/**
 * @brief The special case for handling is<SomeType> expression
 *
 * @tparam is_type: the type query for whether [expr] has the same type
 * @param expr: the expression to match
 * @param ReturnResult: the return expression if [expr] has the same type as [is_type]
 * @return []
 */
template <typename ExprType, typename is_type, typename ReturnType>
auto when(ExprType&&, is<is_type>, ReturnType&& ReturnResult)
{
    if constexpr (std::is_same_v<std::remove_reference_t<ExprType>, typename is<is_type>::type>)
        return ReturnResult;
    return std::remove_reference_t<ReturnType>{};
}
template <typename ExprType, typename is_not_type, typename ReturnType>
auto when(ExprType&&, is_not<is_not_type>, ReturnType&& ReturnResult)
{
    if constexpr (!std::is_same_v<std::remove_reference_t<ExprType>, typename is_not<is_not_type>::type>)
        return ReturnResult;
    return std::remove_reference_t<ReturnType>{};
}

/**
 * @brief Special case for handling C string
*/
template <typename ReturnType>
auto when(const char* Expr, const char* Case, ReturnType&& ReturnResult)
{
    if (strcmp(Expr, Case) == 0)
        return ReturnResult;
    return std::remove_reference_t<ReturnType>{};
}

/**
 * @brief The special case for handling <Else> expression
 *
 * @param ExprType&&: place holder for non-used [expression]
 * @param ReturnResult: return value for Else case
 */
template <typename ExprType, typename ReturnType>
auto when(ExprType&&, Else, ReturnType&& ReturnResult)
{
    return ReturnResult;
}


/**
 * @brief primary recursive template
*/
template <typename ExprType, typename Case1Type, typename Return1Type, typename Case2Type, typename... Args>
auto when(ExprType&& expr, Case1Type&& case1, Return1Type&& return1, Case2Type&& case2, Args&&... args)
{

    if constexpr (std::is_same_v<std::remove_reference_t<Case1Type>, bool>)
    {
        if (case1)
            return return1;
    }
    if constexpr (comparable<ExprType, Case1Type>::value)
    {
        if (case1 == expr)
            return return1;
        else
            return when(std::forward<ExprType>(expr), std::forward<Case2Type>(case2), std::forward<Args>(args)...);
    }
    return when(std::forward<ExprType>(expr), std::forward<Case2Type>(case2), std::forward<Args>(args)...);
}

/**
 * @brief Special case for handling C string
*/
template <typename Return1Type, typename Case2Type, typename... Args>
auto when(const char* Expr, const char* Case1, Return1Type&& return1, Case2Type&& case2, Args&&... args)
{
    if (strcmp(Expr, Case1)==0)
        return return1;
    else
        return when(Expr, std::forward<Case2Type>(case2), std::forward<Args>(args)...);
}

/**
 * @brief Special case for handling is<> and is_not<> type query
*/
template <typename ExprType, typename is_type, typename Return1Type, typename Case2Type, typename... Args>
auto when(ExprType&& expr, is<is_type>, Return1Type&& return1, Case2Type&& case2, Args... args)
{
    if constexpr (std::is_same_v<std::remove_reference_t<ExprType>, typename is<is_type>::type>)
        return std::forward<Return1Type>(return1);
    return when(std::forward<ExprType>(expr), std::forward<Case2Type>(case2), std::forward<Args>(args)...);
}
template <typename ExprType, typename is_not_type, typename Return1Type, typename Case2Type, typename... Args>
auto when(ExprType&& expr, is_not<is_not_type>, Return1Type&& return1, Case2Type&& case2, Args... args)
{
    if constexpr (!std::is_same_v<std::remove_reference_t<ExprType>, typename is_not<is_not_type>::type>)
        return std::forward<Return1Type>(return1);
    return when(std::forward<ExprType>(expr), std::forward<Case2Type>(case2), std::forward<Args>(args)...);
}

#endif