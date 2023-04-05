struct out_of_range_type {};

template <size_t Index, typename ... Tail>
struct get_type_by_index {
    using type = out_of_range_type;
};

template <size_t Index, typename Head, typename ... Tail>
struct get_type_by_index<Index, Head, Tail...> {
    using type = std::conditional_t<Index == 0, Head, typename get_type_by_index<Index - 1, Tail...>::type>;
};

template <size_t Index, typename ... Ts>
using get_type_by_index_t = typename get_type_by_index<Index, Ts...>::type;


template <typename T, typename ... Tail>
struct get_index_by_type {
    static const size_t value = 0;
};

template <typename T, typename Head, typename ... Tail>
struct get_index_by_type<T, Head, Tail...> {
    static const size_t value = std::is_same_v<T, Head> ? 0 : 1 + get_index_by_type<T, Tail...>::value;
};

template <typename T, typename ... Ts>
static const size_t get_index_by_type_v = get_index_by_type<T, Ts...>::value;

template <typename ... Tail>
struct count_types {
    static const size_t value = 0;
};

template <typename Head, typename ... Tail>
struct count_types<Head, Tail...> {
    static const size_t value = 1 + count_types<Tail...>::value;
};

template <typename ... Ts>
static const size_t count_types_v = count_types<Ts...>::value;

template <typename T, typename ... Tail>
struct count_occurrences_of_type {
    static const size_t value = 0;
};

template <typename T, typename Head, typename ... Tail>
struct count_occurrences_of_type<T, Head, Tail...> {
    static const size_t value = std::is_same_v<T, Head> + count_occurrences_of_type<T, Tail...>::value;
};

template <typename ... Ts>
static const size_t count_occurrences_of_type_v = count_occurrences_of_type<Ts...>::value;

//high-level magic which i found here
// https://stackoverflow.com/questions/39547777/implementing-stdvariant-converting-constructor-or-how-to-find-first-overloa

template <typename T>
struct identity { using type = T; };

template <typename... Ts> struct overload;

template <> struct overload<> { void operator()() const; };

template <typename T, typename... Ts>
struct overload<T, Ts...> : overload<Ts...> {
    using overload<Ts...>::operator();
    identity<T> operator()(T) const;
};

// void is a valid variant alternative, but "T operator()(T)" is ill-formed
// when T is void
template <typename... Ts>
struct overload<void, Ts...> : overload<Ts...> {
    using overload<Ts...>::operator();
    identity<void> operator()() const;
};


template <typename T, typename ... Ts>
using best_conversion_t = typename std::result_of_t<overload<Ts...>(T)>::type;


//  meta-programming tools
//------------------------------------------------------------------------------------------------
//  variant

template <typename ... Ts>
class VariantStorage;

template <typename T, typename ... Ts>
class VariantAlternative;

template <typename ... Ts>
class Variant;

template <typename T, typename ...Ts>
bool holds_alternative(const Variant<Ts...>& variant) {
    return variant.index() == get_index_by_type_v<T, Ts...>;
}

template <typename T, typename ... Ts>
T& get(Variant<Ts...>& variant) {
    size_t constexpr index_to_get = get_index_by_type_v<T, Ts...>;
    static_assert(index_to_get != count_types_v<Ts...>, "unknown type");

    if (index_to_get != variant.index()) {
        throw std::logic_error("you picked the wrong house, fool");
    }

    return variant.storage.template get<index_to_get>();
}

template <typename T, typename ... Ts>
const T& get(const Variant<Ts...>& variant) {
    size_t constexpr index_to_get = get_index_by_type_v<T, Ts...>;
    static_assert(index_to_get != count_types_v<Ts...>, "unknown type");

    if (index_to_get != variant.index()) {
        throw std::logic_error("you picked the wrong house, fool");
    }

    return variant.storage.template get<index_to_get>();
}

template <typename T, typename ... Ts>
T&& get(Variant<Ts...>&& variant) {
    size_t constexpr index_to_get = get_index_by_type_v<T, Ts...>;
    static_assert(index_to_get != count_types_v<Ts...>, "unknown type");

    if (index_to_get != variant.index()) {
        throw std::logic_error("you picked the wrong house, fool");
    }

    return std::move(variant).storage.template get<index_to_get>();
}

template <size_t Index, typename ... Ts>
auto& get(Variant<Ts...>& variant) {
    return get<get_type_by_index_t<Index, Ts...>, Ts...>(variant);
}

template <size_t Index, typename ... Ts>
const auto& get(const Variant<Ts...>& variant) {
    return get<get_type_by_index_t<Index, Ts...>, Ts...>(variant);
}

template <size_t Index, typename ... Ts>
auto&& get(Variant<Ts...>&& variant) {
    return get<get_type_by_index_t<Index, Ts...>, Ts...>(std::move(variant));
}

template <typename ... Ts>
class VariantStorage {
    template <typename ... Types>
    friend
    class Variant;

    template <typename T, typename ... Types>
    friend
    class VariantAlternative;

    template <typename T, typename ... Types>
    friend T& get(Variant<Types...>&);

    template <typename T, typename ... Types>
    friend const T& get(const Variant<Types...>&);

    template <typename T, typename ... Types>
    friend T&& get(Variant<Types...>&&);


    template <typename ... Types>
    union VariadicUnion {
    };

    template <typename Head, typename ... Tail>
    union VariadicUnion<Head, Tail...> {
        Head head;
        VariadicUnion<Tail...> tail;

        VariadicUnion() {}

        ~VariadicUnion() {}

        template <size_t Index>
        auto& get()& {
            if constexpr (Index == 0) {
                return head;
            } else {
                return tail.template get<Index - 1>();
            }
        }

        template <size_t Index>
        auto&& get()&& {
            if constexpr (Index == 0) {
                return std::move(head);
            } else {
                return std::move(tail.template get<Index - 1>());
            }
        }

        template <size_t Index>
        const auto& get() const& {
            if constexpr (Index == 0) {
                return head;
            } else {
                return tail.template get<Index - 1>();
            }
        }

        template <typename T, typename ... Args>
        void put(Args&& ... args) {
            if constexpr (std::is_same_v<T, Head>) {
                new(std::launder(const_cast<std::remove_const_t<Head>*>(&head))) T(std::forward<Args>(args)...);
            } else {
                tail.template put<T>(std::forward<Args>(args)...);
            }
        }

        template <typename T>
        void destroy() {
            if constexpr (std::is_same_v<T, Head>) {
                head.~Head();
            } else {
                tail.template destroy<T>();
            }
        }
    };


    static const size_t EMPTYINDEX = -1;

    size_t index_ = EMPTYINDEX;
    VariadicUnion<Ts...> storage;

public:
    size_t index() const {
        return index_;
    }

    bool valueless_by_exception() const {
        return index_ == EMPTYINDEX;
    }
};

template <typename T, typename ... Ts>
class VariantAlternative {
    template <typename ... Types>
    friend
    class VariantStorage;

    template <typename ... Types>
    friend
    class Variant;


    using Derived = Variant<Ts...>;
    static constexpr size_t Index = get_index_by_type_v<T, Ts...>;
public:

    VariantAlternative() = default;

    template <typename U, std::enable_if_t<std::is_same_v<T, best_conversion_t<U, Ts...>>, bool> = true>
    VariantAlternative(U&& value) {
        auto this_ptr = static_cast<Derived*>(this);
        try {
            this_ptr->storage.template put<T>(std::forward<U>(value));
        } catch (...) {
            this_ptr->mark_valueless();
            throw;
        }
        this_ptr->index_ = Index;
    }

    void constructDefault() {
        auto this_ptr = static_cast<Derived*>(this);
        try {
            this_ptr->storage.template put<T>();
        } catch (...) {
            this_ptr->mark_valueless();
            throw;
        }
        this_ptr->index_ = Index;
    }

    void destroy() {
        auto this_ptr = static_cast<Derived*>(this);
        if (this_ptr->index_ == Index) {
            this_ptr->storage.template destroy<T>();
        }
    }


    void moveFrom(Derived&& variant) {
        if (variant.index() == Index) {
            auto this_ptr = static_cast<Derived*>(this);
            try {
                this_ptr->storage.template put<T>(get<T>(std::move(variant)));
            } catch (...) {
                this_ptr->mark_valueless();
                throw;
            }
            this_ptr->index_ = Index;
        }
    }

    void copyFrom(const Derived& variant) {
        if (variant.index() == Index) {
            auto this_ptr = static_cast<Derived*>(this);
            try {
                this_ptr->storage.template put<T>(get<T>(variant));
            } catch (...) {
                this_ptr->mark_valueless();
                throw;
            }
            this_ptr->index_ = Index;
        }
    }

};

template <typename ... Ts>
class Variant : private VariantStorage<Ts...>, private VariantAlternative<Ts, Ts...> ... {
    template <typename ... Types>
    friend
    class VariantStorage;

    template <typename T, typename ... Types>
    friend
    class VariantAlternative;

    template <typename T, typename ... Types>
    friend T& get(Variant<Types...>&);

    template <typename T, typename ... Types>
    friend const T& get(const Variant<Types...>&);

    template <typename T, typename ... Types>
    friend T&& get(Variant<Types...>&&);


    void clear() {
        (::VariantAlternative<Ts, Ts...>::destroy(), ...);
        this->mark_valueless();
    }

    void mark_valueless() {
        this->index_ = VariantStorage<Ts...>::EMPTYINDEX;
    }

public:
    using VariantAlternative<Ts, Ts...>::VariantAlternative...;
    using VariantStorage<Ts...>::index;
    using VariantStorage<Ts...>::valueless_by_exception;

    Variant() {
        ::VariantAlternative<get_type_by_index_t<0, Ts...>, Ts...>::constructDefault();
    }


    template <typename T, typename U, typename ... Args>
    T& emplace(std::initializer_list<U> InList, Args&& ... args) {
        //initializer list type can not be deduced, so extra version of emplace needed
        clear();
        static_assert(count_occurrences_of_type_v<T, Ts...> == 1 &&
                      std::is_constructible_v<T, std::initializer_list<U>, Args...>,
                      "incorrect usage of emplace");

        try {
            this->storage.template put<T>(InList, std::forward<Args>(args)...);
        } catch (...) {
            this->mark_valueless();
            throw;
        }

        this->index_ = get_index_by_type_v<T, Ts...>;

        return get<T>(*this);
    }

    template <typename T, typename ... Args>
    T& emplace(Args&& ... args) {
        clear();
        static_assert(count_occurrences_of_type_v<T, Ts...> == 1 &&
                      std::is_constructible_v<T, Args...>,
                      "incorrect usage of emplace");

        try {
            this->storage.template put<T>(std::forward<Args>(args)...);
        } catch (...) {
            this->mark_valueless();
            throw;
        }

        this->index_ = get_index_by_type_v<T, Ts...>;

        return get<T>(*this);
    }

    template <size_t Index, typename ... Args>
    auto& emplace(Args&& ... args) {
        return emplace<get_type_by_index_t<Index, Ts...>>(std::forward<Args>(args)...);
    }

    Variant(Variant&& that) {
        if (that.valueless_by_exception()) {
            this->mark_valueless();
            return;
        }

        (::VariantAlternative<Ts, Ts...>::moveFrom(std::move(that)), ...);
    }

    Variant(const Variant& that) : VariantStorage<Ts...>(), VariantAlternative<Ts, Ts...>()... {
        if (that.valueless_by_exception()) {
            this->mark_valueless();
            return;
        }

        (::VariantAlternative<Ts, Ts...>::copyFrom(std::move(that)), ...);
    }

    Variant& operator=(const Variant& that) {
        clear();
        if (that.valueless_by_exception()) {
            this->mark_valueless();
            return *this;
        }

        (::VariantAlternative<Ts, Ts...>::copyFrom(that), ...);

        return *this;
    }

    Variant& operator=(Variant&& that) noexcept {
        clear();
        if (that.valueless_by_exception()) {
            this->mark_valueless();
            return *this;
        }

        (::VariantAlternative<Ts, Ts...>::moveFrom(std::move(that)), ...);
        return *this;

    }


    template <typename T, typename U = best_conversion_t<T, Ts...>>
    Variant& operator=(T&& value) {
        static_assert(count_occurrences_of_type_v<U, Ts...> == 1, "ambiguity...");
        if(holds_alternative<U>(*this)) {
            get<U>(*this) = std::forward<T>(value);
            return *this;
        }

        clear();
        try {
            this->storage.template put<U>(std::forward<T>(value));
            this->index_ = get_index_by_type_v<U, Ts...>;
        } catch (...) {
            this->mark_valueless();
            throw;
        }

        return *this;
    }


    ~Variant() {
        clear();
    }
};
