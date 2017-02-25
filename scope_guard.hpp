#pragma once
#ifndef SCOPE_GUARD_HPP
#define SCOPE_GUARD_HPP

#include <functional>
#include <type_traits>
#include <experimental/ranges/concepts>

namespace std::experimental {
    template <class R, class D> class unique_resource;

    // special factory function
    template <class R, class D, class S = R>
    unique_resource<std::decay_t<R>, std::decay_t<D>>
    make_unique_resource_checked(R&& r, const S& invalid, D&& d)
    noexcept(std::is_nothrow_constructible_v<std::decay_t<R>, R> &&
        std::is_nothrow_constructible_v<std::decay_t<D>, D>);

    template <class R, class D>
    unique_resource<std::decay_t<R>, std::decay_t<D>>
    make_unique_resource(R&& r, D&& d)
        noexcept(std::is_nothrow_constructible_v<std::decay_t<R>, R> &&
            std::is_nothrow_constructible_v<std::decay_t<D>, D>);

    template <class R, class D>
    unique_resource<R&, std::decay_t<D>>
    make_unique_resource(std::reference_wrapper<R> r, D&& d)
        noexcept(std::is_nothrow_constructible_v<std::decay_t<D>, D>);

    template <ranges::Destructible EF>
    requires
        (std::is_object_v<EF> ||
            (std::is_lvalue_reference_v<EF> &&
                (std::is_object_v<std::remove_reference_t<EF>> ||
                std::is_function_v<std::remove_reference_t<EF>> ))) &&
        ranges::Invocable<EF&>()
    class basic_scope_exit {
    protected:
        ~basic_scope_exit() {
            if (execute_on_destruction_) {
                exit_function_();
            }
        }
    public:
        template <class EFP>
        requires ranges::Constructible<EF, EFP>()
        constexpr explicit basic_scope_exit(EFP&& f)
        noexcept(std::is_nothrow_constructible_v<EF, EFP>)
        : exit_function_(std::forward<EFP>(f))
        {}
        basic_scope_exit(basic_scope_exit&& that)
        requires
            ranges::MoveConstructible<EF>() &&
            std::is_nothrow_move_constructible_v<EF>
        : exit_function_(std::move(that).exit_function_)
        , execute_on_destruction_(std::exchange(that.execute_on_destruction_, false))
        {}
        basic_scope_exit(basic_scope_exit&& that)
        requires
            ranges::CopyConstructible<EF>() &&
            !std::is_nothrow_move_constructible_v<EF>
        : exit_function_(that.exit_function_)
        , execute_on_destruction_(std::exchange(that.execute_on_destruction_, false))
        {}
        basic_scope_exit& operator=(basic_scope_exit&&) = delete;
        void release() noexcept {
            execute_on_destruction_ = false;
        }
    private:
        EF exit_function_;
        bool execute_on_destruction_{true};
    };

    template <class EF>
    struct scope_exit : basic_scope_exit<EF> {
        template <class EFP>
        requires
            ranges::Constructible<EF, EFP>() &&
            std::is_nothrow_constructible_v<EF, EFP>
        constexpr explicit scope_exit(EFP&& f)
        noexcept(std::is_nothrow_constructible_v<EF, EFP>)
        : basic_scope_exit<EF>(std::forward<EFP>(f))
        {}
        template <class EFP>
        requires ranges::Constructible<EF, const EFP&>()
        constexpr explicit scope_exit(const EFP& f)
        try : basic_scope_exit<EF>(f)
        {}
        catch(...) {
            f();
            throw;
        }
    };

    class exception_count {
    protected:
        exception_count() = default;
        exception_count(const exception_count&) noexcept {}
        exception_count(exception_count&&) noexcept {}
        exception_count& operator=(const exception_count&) noexcept { return *this; }
        exception_count& operator=(exception_count&&) noexcept { return *this; }
        const int uncaught_exceptions_{std::uncaught_exceptions()};
    };

    template <class EF>
    struct scope_success : basic_scope_exit<EF>, exception_count {
        using basic_scope_exit<EF>::basic_scope_exit;
        ~scope_success() {
            if (std::uncaught_exceptions() > this->uncaught_exceptions_) {
                this->release();
            }
        }
    };

    template <class EF>
    struct scope_fail : scope_exit<EF>, exception_count {
        using scope_exit<EF>::scope_exit;
        ~scope_fail() noexcept(noexcept(std::declval<EF&>())) {
            if (std::uncaught_exceptions() <= this->uncaught_exceptions_) {
                this->release();
            }
        }
    };

    template <class EF>
    constexpr scope_exit<std::decay_t<EF>> make_scope_exit(EF&& exit_function) {
        return scope_exit<std::decay_t<EF>>(std::forward<EF>(exit_function));
    }

    template <class EF>
    constexpr scope_fail<std::decay_t<EF>> make_scope_fail(EF&& exit_function) {
        return scope_fail<std::decay_t<EF>>(std::forward<EF>(exit_function));
    }

    template <class EF>
    constexpr scope_success<std::decay_t<EF>> make_scope_success(EF&& exit_function) {
        return scope_success<std::decay_t<EF>>(std::forward<EF>(exit_function));
    }
} // namespace std::experimental

#endif // SCOPE_GUARD_HPP
