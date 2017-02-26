#include <cassert>
#include <iostream>
#include "scope_guard.hpp"

namespace {
    int count = 0;
}

int main() {
    using std::experimental::ranges::ext::scope_exit;
    using std::experimental::ranges::ext::make_scope_exit;

    assert(!count);
    {
        // move-only noexcept object type
        struct F {
            F() noexcept { std::cout << __PRETTY_FUNCTION__ << '\n'; }
            F(F&&) noexcept { std::cout << __PRETTY_FUNCTION__ << '\n'; }
            void operator()() const { std::cout << __PRETTY_FUNCTION__ << '\n'; --count; }
        };
        static_assert(std::is_nothrow_move_constructible<F>::value);
        static_assert(sizeof(scope_exit<F>) == sizeof(bool));
        count = 3;
        scope_exit<F> guard1(F{});
        F f{};
        scope_exit<std::reference_wrapper<F>> guard2(std::ref(f));
        static_assert(sizeof(guard2) == sizeof(void*));
        auto guard3 = make_scope_exit(F{});
    }
    assert(!count);
    std::cout << '\n';
    {
        // copyable throwing object type (does not throw)
        struct F {
            F() noexcept { std::cout << __PRETTY_FUNCTION__ << '\n'; }
            F(F&&); // undefined
            F(const F&) { std::cout << __PRETTY_FUNCTION__ << '\n'; }
            void operator()() const { std::cout << __PRETTY_FUNCTION__ << '\n'; --count; }
        };
        static_assert(!std::is_nothrow_move_constructible<F>::value);
        static_assert(sizeof(scope_exit<F>) == sizeof(bool));
        count = 5;
        scope_exit<F> guard1(F{});
        F f{};
        scope_exit<std::reference_wrapper<F>> guard2(std::ref(f));
        static_assert(sizeof(guard2) == sizeof(void*));
        scope_exit<F> guard3(f);
        auto guard4 = make_scope_exit(F{});
        auto guard5 = make_scope_exit(f);
    }
    assert(!count);
    std::cout << '\n';
    {
        // copyable throwing object type (does throw)
        struct F {
            F() noexcept { std::cout << __PRETTY_FUNCTION__ << '\n'; }
            F(const F&) { std::cout << __PRETTY_FUNCTION__ << ": throw...\n"; throw 42; }
            void operator()() const { std::cout << __PRETTY_FUNCTION__ << '\n'; --count; }
        };
        static_assert(!std::is_nothrow_move_constructible<F>::value);
        static_assert(sizeof(scope_exit<F>) == sizeof(bool));
        count = 5;
        try { scope_exit<F> _(F{}); std::abort(); }
        catch(int) { std::cout << "...catch\n"; }
        F f{};
        scope_exit<std::reference_wrapper<F>> guard2(std::ref(f));
        static_assert(sizeof(guard2) == sizeof(void*));
        try { scope_exit<F> _(f); std::abort(); }
        catch(int) { std::cout << "...catch\n"; }
        try { make_scope_exit(F{}); std::abort(); }
        catch(int) { std::cout << "...catch\n"; }
        try { make_scope_exit(f); std::abort(); }
        catch(int) { std::cout << "...catch\n"; }
    }
    assert(!count);
    std::cout << '\n';
    {
        // copyable noexcept object type (use move constructor)
        struct F {
            F() noexcept { std::cout << __PRETTY_FUNCTION__ << '\n'; }
            F(const F&) noexcept; // undefined
            F(F&&) noexcept { std::cout << __PRETTY_FUNCTION__ << '\n'; }
            void operator()() const { std::cout << __PRETTY_FUNCTION__ << '\n'; --count; }
        };
        static_assert(std::is_nothrow_move_constructible<F>::value);
        count = 3;
        scope_exit<F> guard1(F{});
        static_assert(sizeof(guard1) == sizeof(bool));
        F f{};
        scope_exit<std::reference_wrapper<F>> guard2(std::ref(f));
        static_assert(sizeof(guard2) == sizeof(void*));
        auto guard3 = make_scope_exit(F{});
    }
    assert(!count);
    std::cout << '\n';
    {
        // copyable noexcept object type (use copy constructor)
        struct F {
            F() noexcept { std::cout << __PRETTY_FUNCTION__ << '\n'; }
            F(const F&) noexcept { std::cout << __PRETTY_FUNCTION__ << '\n'; }
            F(F&&) noexcept; // undefined
            void operator()() const { std::cout << __PRETTY_FUNCTION__ << '\n'; --count; }
        };
        static_assert(std::is_nothrow_move_constructible<F>::value);
        static_assert(sizeof(scope_exit<F>) == sizeof(bool));
        count = 3;
        F f{};
        scope_exit<std::reference_wrapper<F>> guard1(std::ref(f));
        static_assert(sizeof(guard1) == sizeof(void*));
        scope_exit<F> guard2(f);
        auto guard3 = make_scope_exit(f);
    }
    assert(!count);
}
