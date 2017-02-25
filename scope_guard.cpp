#include <cassert>
#include <iostream>
#include "scope_guard.hpp"

using std::experimental::scope_exit;
using std::experimental::make_scope_exit;

namespace {
    int count = 0;
}

int main() {
    assert(!count);
    {
        // move-only noexcept object type
        struct F {
            F() noexcept { std::cout << __PRETTY_FUNCTION__ << '\n'; }
            F(F&&) noexcept { std::cout << __PRETTY_FUNCTION__ << '\n'; }
            void operator()() const { std::cout << __PRETTY_FUNCTION__ << '\n'; --count; }
        };
        static_assert(std::is_nothrow_move_constructible<F>::value);
        count = 3;
        scope_exit<F> guard1(F{});
        F f{};
        scope_exit<F&> guard2(f);
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
        count = 5;
        scope_exit<F> guard1(F{});
        F f{};
        scope_exit<F&> guard2(f);
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
        count = 5;
        try { scope_exit<F> _(F{}); std::abort(); }
        catch(int) { std::cout << "...catch\n"; }
        F f{};
        scope_exit<F&> guard2(f);
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
        F f{};
        scope_exit<F&> guard2(f);
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
        count = 3;
        F f{};
        scope_exit<F&> guard1(f);
        scope_exit<F> guard2(f);
        auto guard3 = make_scope_exit(f);
    }
    assert(!count);
}
