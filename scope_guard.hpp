#pragma once
#ifndef SCOPE_GUARD_HPP
#define SCOPE_GUARD_HPP

#include <functional>
#include <type_traits>
#include <utility>
#include <experimental/ranges/concepts>

namespace std::experimental {
#if 0 // NYI
    template <class R, class D> class unique_resource;

    // special factory function
    template <class R, class D, class S = R>
    unique_resource<std::decay_t<R>, std::decay_t<D>>
    make_unique_resource_checked(R&& r, const S& invalid, D&& d)
    noexcept(std::is_nothrow_constructible<std::decay_t<R>, R>::value &&
        std::is_nothrow_constructible<std::decay_t<D>, D>::value);

    template <class R, class D>
    unique_resource<std::decay_t<R>, std::decay_t<D>>
    make_unique_resource(R&& r, D&& d)
        noexcept(std::is_nothrow_constructible<std::decay_t<R>, R>::value &&
            std::is_nothrow_constructible<std::decay_t<D>, D>::value);

    template <class R, class D>
    unique_resource<R&, std::decay_t<D>>
    make_unique_resource(std::reference_wrapper<R> r, D&& d)
        noexcept(std::is_nothrow_constructible<std::decay_t<D>, D>::value);
#endif // NYI

    template <ranges::Destructible _EF>
    requires
        (std::is_object<_EF>::value ||
            (std::is_lvalue_reference<_EF>::value &&
                (std::is_object<std::remove_reference_t<_EF>>::value ||
                 std::is_function<std::remove_reference_t<_EF>>::value))) &&
        ranges::Invocable<_EF&>()
    class __basic_scope_exit {
    protected:
        ~__basic_scope_exit() {
            if (__execute_on_destruction) {
                __exit_function();
            }
        }
    public:
        template <class _EFP>
        requires ranges::Constructible<_EF, _EFP>()
        constexpr explicit __basic_scope_exit(_EFP&& __fn)
        noexcept(std::is_nothrow_constructible<_EF, _EFP>::value)
        : __exit_function(std::forward<_EFP>(__fn))
        {}
        __basic_scope_exit(__basic_scope_exit&& __that)
        requires
            ranges::MoveConstructible<_EF>() &&
            std::is_nothrow_move_constructible<_EF>::value
        : __exit_function(std::move(__that).__exit_function)
        , __execute_on_destruction(std::exchange(__that.__execute_on_destruction, false))
        {}
        __basic_scope_exit(__basic_scope_exit&& __that)
        requires
            ranges::CopyConstructible<_EF>() &&
            !std::is_nothrow_move_constructible<_EF>::value
        : __exit_function(__that.__exit_function)
        , __execute_on_destruction(std::exchange(__that.__execute_on_destruction, false))
        {}
        __basic_scope_exit& operator=(__basic_scope_exit&&) = delete;
        void release() noexcept {
            __execute_on_destruction = false;
        }
    private:
        _EF __exit_function;
        bool __execute_on_destruction = true;
    };

    template <class _EF>
    struct scope_exit : __basic_scope_exit<_EF> {
        template <class _EFP>
        requires
            ranges::Constructible<_EF, _EFP>() &&
            std::is_nothrow_constructible<_EF, _EFP>::value
        constexpr explicit scope_exit(_EFP&& __fn)
        noexcept(std::is_nothrow_constructible<_EF, _EFP>::value)
        : __basic_scope_exit<_EF>(std::forward<_EFP>(__fn))
        {}
        template <class _EFP>
        requires ranges::Constructible<_EF, const _EFP&>()
        constexpr explicit scope_exit(const _EFP& __fn)
        try : __basic_scope_exit<_EF>(__fn)
        {}
        catch(...) {
            __fn();
            throw;
        }
    };

    class __exception_count {
    protected:
        __exception_count() = default;
        __exception_count(const __exception_count&) noexcept {}
        __exception_count(__exception_count&&) noexcept {}
        __exception_count& operator=(const __exception_count&) noexcept { return *this; }
        __exception_count& operator=(__exception_count&&) noexcept { return *this; }

        const int __uncaught_exceptions = std::uncaught_exceptions();
    };

    template <class _EF>
    struct scope_success : __basic_scope_exit<_EF>, __exception_count {
        using __basic_scope_exit<_EF>::__basic_scope_exit;
        ~scope_success() {
            if (std::uncaught_exceptions() > this->__uncaught_exceptions) {
                this->release();
            }
        }
    };

    template <class _EF>
    struct scope_fail : scope_exit<_EF>, __exception_count {
        using scope_exit<_EF>::scope_exit;
        ~scope_fail() noexcept(noexcept(std::declval<_EF&>())) { // FIXME: It's too late to apply noexcept(false) here.
            if (std::uncaught_exceptions() <= this->__uncaught_exceptions) {
                this->release();
            }
        }
    };

    template <class _EF>
    constexpr scope_exit<std::decay_t<_EF>> make_scope_exit(_EF&& __exit_function) {
        return scope_exit<std::decay_t<_EF>>(std::forward<_EF>(__exit_function));
    }

    template <class _EF>
    constexpr scope_fail<std::decay_t<_EF>> make_scope_fail(_EF&& __exit_function) {
        return scope_fail<std::decay_t<_EF>>(std::forward<_EF>(__exit_function));
    }

    template <class _EF>
    constexpr scope_success<std::decay_t<_EF>> make_scope_success(_EF&& __exit_function) {
        return scope_success<std::decay_t<_EF>>(std::forward<_EF>(__exit_function));
    }
} // namespace std::experimental

#endif // SCOPE_GUARD_HPP
