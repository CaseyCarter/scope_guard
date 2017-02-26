#include <cassert>
#include <iostream>
#include "scope_guard.hpp"

namespace {
    int count = 0;
}

namespace ranges = std::experimental::ranges;

int main() {
    using ranges::Same;
    using ranges::ext::scope_exit;
    using ranges::ext::make_scope_exit;

    assert(!count);
    {
        // move-only noexcept object type
        struct F {
            F() noexcept { std::cout << __PRETTY_FUNCTION__ << '\n'; }
            F(F&&) noexcept { std::cout << __PRETTY_FUNCTION__ << '\n'; }
            void operator()() const { std::cout << __PRETTY_FUNCTION__ << '\n'; --count; }
        };
        static_assert(std::is_nothrow_move_constructible<F>::value);
        count = 2;
        Same<scope_exit<F>> guard1 = make_scope_exit(F{});
        static_assert(sizeof(guard1) == sizeof(bool));
        F f{};
        Same<scope_exit<std::reference_wrapper<F>>> guard2 = make_scope_exit(std::ref(f));
        static_assert(sizeof(guard2) == sizeof(void*));
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
        count = 3;
        Same<scope_exit<F>> guard1 = make_scope_exit(F{});
        static_assert(sizeof(guard1) == sizeof(bool));
        F f{};
        Same<scope_exit<std::reference_wrapper<F>>> guard2 = make_scope_exit(std::ref(f));
        static_assert(sizeof(guard2) == sizeof(void*));
        Same<scope_exit<F>> guard3 = make_scope_exit(f);
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
        count = 4;
        try {
            Same<scope_exit<F>> _ = make_scope_exit(F{});
            static_assert(sizeof(_) == sizeof(bool));
            std::abort();
        } catch(int) {
            std::cout << "...catch\n";
        }
        F f{};
        Same<scope_exit<std::reference_wrapper<F>>> guard2 = make_scope_exit(std::ref(f));
        static_assert(sizeof(guard2) == sizeof(void*));
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
        count = 2;
        Same<scope_exit<F>> guard1 = make_scope_exit(F{});
        static_assert(sizeof(guard1) == sizeof(bool));
        F f{};
        Same<scope_exit<std::reference_wrapper<F>>> guard2 = make_scope_exit(std::ref(f));
        static_assert(sizeof(guard2) == sizeof(void*));
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
        count = 2;
        F f{};
        Same<scope_exit<F>> guard1 = make_scope_exit(f);
        static_assert(sizeof(guard1) == sizeof(bool));
        Same<scope_exit<std::reference_wrapper<F>>> guard2 = make_scope_exit(std::ref(f));
        static_assert(sizeof(guard2) == sizeof(void*));
    }
    assert(!count);
}
