#pragma once
#ifndef SCOPE_GUARD_HPP
#define SCOPE_GUARD_HPP

#include <functional>
#include <type_traits>
#include <utility>
#include <stl2/detail/compressed_pair.hpp>
#include <stl2/detail/fwd.hpp>
#include <stl2/detail/concepts/function.hpp>
#include <stl2/detail/concepts/object.hpp>

STL2_OPEN_NAMESPACE { namespace ext {
#if 0 // NYI
    template <class R,class D>
    class unique_resource {
    public:
        template <class RR, class DD>
        explicit unique_resource(RR&& r, DD&& d)
        noexcept(std::is_nothrow_constructible<R, RR>::value &&
            std::is_nothrow_constructible<D, DD>::value);
        unique_resource(unique_resource&& that)
        noexcept(std::is_nothrow_move_constructible<R>::value &&
            std::is_nothrow_move_constructible<D>::value);
        unique_resource(const unique_resource&) = delete;
        ~unique_resource();
        unique_resource& operator=(unique_resource&& that);
        unique_resource& operator=(const unique_resource&) = delete;
        void swap(unique_resource& that);
        void reset();
        template <class RR>
        void reset(RR&& r);
        void release() noexcept;
        const R& get() const noexcept;
        R operator->() const noexcept;
        see_below operator*() const noexcept;
        const D& get_deleter() const noexcept;
    private:
        R resource;
        D deleter;
        bool execute_on_destruction = true;
    };

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

    namespace detail {
        template <Destructible EF, bool NoThrowDestruction = true>
        requires std::is_object<EF>::value && Invocable<EF&>()
        class scope_guard {
        private:
            compressed_pair<EF, bool> data_;
            EF& exit_function() { return data_.first(); }
            bool& execute_on_destruction() { return data_.second(); }
        protected:
            ~scope_guard() noexcept(NoThrowDestruction) {
                if (execute_on_destruction()) {
                    exit_function()();
                }
            }
        public:
            template <class EFP>
            requires Constructible<EF, EFP>()
            constexpr explicit scope_guard(EFP&& f)
            noexcept(std::is_nothrow_constructible<EF, EFP>::value)
            : data_(std::forward<EFP>(f), true)
            {}
            scope_guard(scope_guard&& that) noexcept
            requires
                MoveConstructible<EF>() &&
                std::is_nothrow_move_constructible<EF>::value
            : data_(std::move(that).data_)
            {
                that.release();
            }
            scope_guard(scope_guard&& that)
            requires
                CopyConstructible<EF>() &&
                !std::is_nothrow_move_constructible<EF>::value
            : data_(that.data_)
            {
                that.release();
            }
            scope_guard& operator=(scope_guard&&) = delete;
            void release() noexcept {
                execute_on_destruction() = false;
            }
        };

        template <class F, bool NoThrowDestruction>
        requires Invocable<F&>()
        class scope_guard<std::reference_wrapper<F>, NoThrowDestruction> {
        private:
            F* ptr_;
        protected:
            ~scope_guard() noexcept(NoThrowDestruction) {
                if (ptr_) {
                    std::reference_wrapper<F>{*ptr_}();
                }
            }
        public:
            constexpr explicit scope_guard(std::reference_wrapper<F> rw) noexcept
            : ptr_(std::addressof(rw.get()))
            {}
            scope_guard(scope_guard&& that) noexcept
            : ptr_(std::exchange(that.ptr_, nullptr))
            {}
            scope_guard& operator=(scope_guard&&) = delete;
            void release() noexcept {
                ptr_ = nullptr;
            }
        };

        class exception_count {
        public:
            exception_count() = default;
            exception_count(const exception_count&) noexcept {}
            exception_count(exception_count&&) noexcept {}
            exception_count& operator=(const exception_count&) noexcept { return *this; }
            exception_count& operator=(exception_count&&) noexcept { return *this; }

            bool more_exceptions() const noexcept {
                return std::uncaught_exceptions() > uncaught_exceptions_;
            }
        private:
            const int uncaught_exceptions_ = std::uncaught_exceptions();
        };
    }

    template <class EF, bool NoThrowDestruction = true>
    struct scope_exit : private detail::scope_guard<EF, NoThrowDestruction> {
        template <class EFP>
        requires
            Constructible<EF, EFP>() &&
            std::is_nothrow_constructible<EF, EFP>::value
        constexpr explicit scope_exit(EFP&& f)
        noexcept(std::is_nothrow_constructible<EF, EFP>::value)
        : detail::scope_guard<EF>(std::forward<EFP>(f))
        {}
        template <class EFP>
        requires
            Constructible<EF, EFP&>() &&
            !std::is_nothrow_constructible<EF, EFP>::value
        constexpr explicit scope_exit(EFP&& f)
        try : detail::scope_guard<EF>(f)
        {}
        catch(...) {
            f();
            throw;
        }
        using detail::scope_guard<EF>::release;
    };

    template <class EF>
    class scope_success : detail::scope_guard<EF>
    {
    public:
        ~scope_success() {
            if (count.more_exceptions()) {
                this->release();
            }
        }
        using detail::scope_guard<EF>::detail::scope_guard;
        using detail::scope_guard<EF>::release;

    private:
        detail::exception_count count;
    };

    template <class EF>
    class scope_fail : scope_exit<EF, noexcept(noexcept(std::declval<EF&>()))>
    {
    public:
        ~scope_fail() {
            if (!count.more_exceptions()) {
                this->release();
            }
        }
        using scope_exit<EF>::scope_exit;
        using scope_exit<EF>::release;

    private:
        detail::exception_count count;
    };

    template <class EF>
    requires
        Constructible<std::decay_t<EF>, EF>() &&
        Invocable<std::decay_t<EF>&>()
    constexpr scope_exit<std::decay_t<EF>> make_scope_exit(EF&& exit_function)
    noexcept(std::is_nothrow_constructible<scope_exit<std::decay_t<EF>>, EF>::value)
    {
        return scope_exit<std::decay_t<EF>>(std::forward<EF>(exit_function));
    }

    template <class EF>
    requires
        Constructible<std::decay_t<EF>, EF>() &&
        Invocable<std::decay_t<EF>&>()
    constexpr scope_fail<std::decay_t<EF>> make_scope_fail(EF&& exit_function)
    noexcept(std::is_nothrow_constructible<scope_fail<std::decay_t<EF>>, EF>::value)
    {
        return scope_fail<std::decay_t<EF>>(std::forward<EF>(exit_function));
    }

    template <class EF>
    requires
        Constructible<std::decay_t<EF>, EF>() &&
        Invocable<std::decay_t<EF>&>()
    constexpr scope_success<std::decay_t<EF>> make_scope_success(EF&& exit_function)
    noexcept(std::is_nothrow_constructible<scope_success<std::decay_t<EF>>, EF>::value)
    {
        return scope_success<std::decay_t<EF>>(std::forward<EF>(exit_function));
    }
}} STL2_CLOSE_NAMESPACE

#endif // SCOPE_GUARD_HPP
