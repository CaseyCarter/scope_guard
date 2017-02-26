* The specifications of the `scope_exit` and `scope_fail` perfect-forwarding constructor templates are word-for-word identical. I recommend using a single specification for both:
  > ```c++
  > template<class EFP>
  > explicit scope_exit(EFP&& f);
  >
  > template<class EFP>
  > explicit scope_fail(EFP&& f);
  > ```
  >
  > *Remarks:* These constructors shall not participate in overload resolution unless `is_constructible_v<EF, EFP>` is `true`.
  >
  > *Requires:* ...

* The `scope_exit` and `scope_fail` perfect-forwarding constructor templates are underconstrained: `is_constructible_v<EF, EFP>` is insufficient to guarantee that initialization of `exit_function` with `f` is well-formed when `EFP` is an object type such that `is_nothrow_constructible<EF, EFP>` is `false`. In particular, when `EF` is a move-only function object type with a throwing move constructor, these constructor templates are ill-formed in that they attempt to copy an rvalue `EF` argument:
  ```c++
  struct F {
      F() = default;
      F(F&&) {}
      void operator()() const {}
  };
  scope_exit<F> guard{F{}}; // is_constructible_v<EF, EF> is true, but copy construction is ill-formed.
  ```
  There are a couple of alternatives to fix this:

  1. Always perfect-forward the initialization and invoke `f` on failure. This greatly simplifies the specification by putting the burden on `EF`'s move constructor to leave `f` in a usable state when it throws. Reword the Effects and Throws elements of `scope_exit` and `scope_fail` to:

     > *Effects:* Initializes `exit_function` with `std::forward<EFP>(f)`. If the initialization throws an exception, calls `f()` before allowing that exception to propagate.
     >
     > *Throws:* Any exception thrown by the initialization of `exit_function`.

  1. Tighten up the constraints to make such ill-formed cases forbidden.

     > *Remarks:* This constructor shall not participate in overload resolution unless `is_constructible_v<EF, EFP> && is_nothrow_constructible_v<EF, EFP> || is_constructible_v<EF, EFP&>` is `true`.
     >
     > *Requires:* [...]
     >
     > *Effects:* If `is_nothrow_constructible_v<EF, EFP>` is `true`, initializes `exit_function` with `std::forward<EFP>(f)`. Otherwise, initializes `exit_function` with `f`. If the initialization throws an exception, calls `f()` before allowing that exception to propagate.
     >
     > *Throws:* Any exception thrown by the initialization of `exit_function`.


* The `scope_success` perfect-forwarding constructor template need not complicate its Effects element as do the other constructor templates, since it never calls `f` and does not care if the initialization throws an exception. I recommend replacing the paragraph with:
  > 11 *Effects:* Initializes `exit_function` with `std::forward<EFP>(f)`. [ *Note:* If copying fails, `f` is not called. — *end note* ]

* [scope.scope_guard]/18 - the *Remarks* element for `~scope_success` - is unnecessary. Presumably this paragraph is a hold-over from an early revision of the paper that did not adjust [res.on.exception.handling] to allow a standard library class to have a throwing destructor.

* The specification of the `scope_guard` class templates is unnecessarily complicated by allowing `EF` to be a function or function object reference. It would be simplified by requiring `EF` to always be an *Lvalue-Callable* (N4640 [func.wrap.func]/2) and `Destructible` object type.
