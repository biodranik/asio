//
// experimental/basic_channel.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2022 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_ASIO_EXPERIMENTAL_BASIC_CHANNEL_HPP
#define BOOST_ASIO_EXPERIMENTAL_BASIC_CHANNEL_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <boost/asio/detail/config.hpp>
#include <boost/asio/detail/non_const_lvalue.hpp>
#include <boost/asio/detail/null_mutex.hpp>
#include <boost/asio/execution/executor.hpp>
#include <boost/asio/execution_context.hpp>
#include <boost/asio/experimental/detail/channel_send_functions.hpp>
#include <boost/asio/experimental/detail/channel_service.hpp>

#include <boost/asio/detail/push_options.hpp>

namespace boost {
namespace asio {
namespace experimental {
namespace detail {

} // namespace detail

/// A channel for messages.
template <typename Executor, typename Traits, typename... Signatures>
class basic_channel
#if !defined(GENERATING_DOCUMENTATION)
  : public detail::channel_send_functions<
      basic_channel<Executor, Traits, Signatures...>,
      Executor, Signatures...>
#endif // !defined(GENERATING_DOCUMENTATION)
{
private:
  class initiate_async_send;
  class initiate_async_receive;
  typedef detail::channel_service<boost::asio::detail::null_mutex> service_type;
  typedef typename service_type::template implementation_type<
      Traits, Signatures...>::payload_type payload_type;

  template <typename... PayloadSignatures,
      BOOST_ASIO_COMPLETION_TOKEN_FOR(PayloadSignatures...) CompletionToken>
  auto do_async_receive(detail::channel_payload<PayloadSignatures...>*,
      BOOST_ASIO_MOVE_ARG(CompletionToken) token)
    -> decltype(
        async_initiate<CompletionToken, PayloadSignatures...>(
          declval<initiate_async_receive>(), token))
  {
    return async_initiate<CompletionToken, PayloadSignatures...>(
        initiate_async_receive(this), token);
  }

public:
  /// The type of the executor associated with the channel.
  typedef Executor executor_type;

  /// Rebinds the channel type to another executor.
  template <typename Executor1>
  struct rebind_executor
  {
    /// The channel type when rebound to the specified executor.
    typedef basic_channel<Executor1, Traits, Signatures...> other;
  };

  /// The traits type associated with the channel.
  typedef typename Traits::template rebind<Signatures...>::other traits_type;

  /// Construct a basic_channel.
  /**
   * This constructor creates and channel.
   *
   * @param ex The I/O executor that the channel will use, by default, to
   * dispatch handlers for any asynchronous operations performed on the channel.
   *
   * @param max_buffer_size The maximum number of messages that may be buffered
   * in the channel.
   */
  basic_channel(const executor_type& ex, std::size_t max_buffer_size = 0)
    : service_(&boost::asio::use_service<service_type>(
            basic_channel::get_context(ex))),
      impl_(),
      executor_(ex)
  {
    service_->construct(impl_, max_buffer_size);
  }

  /// Construct and open a basic_channel.
  /**
   * This constructor creates and opens a channel.
   *
   * @param context An execution context which provides the I/O executor that
   * the channel will use, by default, to dispatch handlers for any asynchronous
   * operations performed on the channel.
   *
   * @param max_buffer_size The maximum number of messages that may be buffered
   * in the channel.
   */
  template <typename ExecutionContext>
  basic_channel(ExecutionContext& context, std::size_t max_buffer_size = 0,
      typename constraint<
        is_convertible<ExecutionContext&, execution_context&>::value,
        defaulted_constraint
      >::type = defaulted_constraint())
    : service_(&boost::asio::use_service<service_type>(context)),
      impl_(),
      executor_(context.get_executor())
  {
    service_->construct(impl_, max_buffer_size);
  }

#if defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)
  /// Move-construct a basic_channel from another.
  /**
   * This constructor moves a channel from one object to another.
   *
   * @param other The other basic_channel object from which the move will occur.
   *
   * @note Following the move, the moved-from object is in the same state as if
   * constructed using the @c basic_channel(const executor_type&) constructor.
   */
  basic_channel(basic_channel&& other)
    : service_(other.service_),
      executor_(other.executor_)
  {
    service_->move_construct(impl_, other.impl_);
  }

  /// Move-assign a basic_channel from another.
  /**
   * This assignment operator moves a channel from one object to another.
   * Cancels any outstanding asynchronous operations associated with the target
   * object.
   *
   * @param other The other basic_channel object from which the move will occur.
   *
   * @note Following the move, the moved-from object is in the same state as if
   * constructed using the @c basic_channel(const executor_type&)
   * constructor.
   */
  basic_channel& operator=(basic_channel&& other)
  {
    if (this != &other)
    {
      service_->move_assign(impl_, *other.service_, other.impl_);
      executor_.~executor_type();
      new (&executor_) executor_type(other.executor_);
      service_ = other.service_;
    }
    return *this;
  }

  // All channels have access to each other's implementations.
  template <typename, typename, typename...>
  friend class basic_channel;

  /// Move-construct a basic_channel from another.
  /**
   * This constructor moves a channel from one object to another.
   *
   * @param other The other basic_channel object from which the move will occur.
   *
   * @note Following the move, the moved-from object is in the same state as if
   * constructed using the @c basic_channel(const executor_type&)
   * constructor.
   */
  template <typename Executor1>
  basic_channel(
      basic_channel<Executor1, Traits, Signatures...>&& other,
      typename constraint<
          is_convertible<Executor1, Executor>::value
      >::type = 0)
    : service_(other.service_),
      executor_(other.executor_)
  {
    service_->move_construct(impl_, *other.service_, other.impl_);
  }

  /// Move-assign a basic_channel from another.
  /**
   * This assignment operator moves a channel from one object to another.
   * Cancels any outstanding asynchronous operations associated with the target
   * object.
   *
   * @param other The other basic_channel object from which the move will
   * occur.
   *
   * @note Following the move, the moved-from object is in the same state as if
   * constructed using the @c basic_channel(const executor_type&)
   * constructor.
   */
  template <typename Executor1>
  typename constraint<
    is_convertible<Executor1, Executor>::value,
    basic_channel&
  >::type operator=(basic_channel<Executor1, Traits, Signatures...>&& other)
  {
    if (this != &other)
    {
      service_->move_assign(impl_, *other.service_, other.impl_);
      executor_.~executor_type();
      new (&executor_) executor_type(other.executor_);
      service_ = other.service_;
    }
    return *this;
  }
#endif // defined(BOOST_ASIO_HAS_MOVE) || defined(GENERATING_DOCUMENTATION)

  /// Destructor.
  ~basic_channel()
  {
    service_->destroy(impl_);
  }

  /// Get the executor associated with the object.
  executor_type get_executor() BOOST_ASIO_NOEXCEPT
  {
    return executor_;
  }

  /// Get the capacity of the channel's buffer.
  std::size_t capacity() BOOST_ASIO_NOEXCEPT
  {
    return service_->capacity(impl_);
  }

  /// Determine whether the channel is open.
  bool is_open() const BOOST_ASIO_NOEXCEPT
  {
    return service_->is_open(impl_);
  }

  /// Reset the channel to its initial state.
  void reset()
  {
    service_->reset(impl_);
  }

  /// Close the channel.
  void close()
  {
    service_->close(impl_);
  }

  /// Cancel all asynchronous operations waiting on the channel.
  /**
   * All outstanding send operations will complete with the error
   * @c boost::asio::experimental::error::channel_canceld. Outstanding receive
   * operations complete with the result as determined by the channel traits.
   */
  void cancel()
  {
    service_->cancel(impl_);
  }

  /// Determine whether a message can be received without blocking.
  bool ready() const BOOST_ASIO_NOEXCEPT
  {
    return service_->ready(impl_);
  }

#if defined(GENERATING_DOCUMENTATION)

  /// Try to send a message without blocking.
  /**
   * Fails if the buffer is full and there are no waiting receive operations.
   *
   * @returns @c true on success, @c false on failure.
   */
  template <typename... Args>
  bool try_send(BOOST_ASIO_MOVE_ARG(Args)... args);

  /// Try to send a number of messages without blocking.
  /**
   * @returns The number of messages that were sent.
   */
  template <typename... Args>
  std::size_t try_send_n(std::size_t count, BOOST_ASIO_MOVE_ARG(Args)... args);

  /// Asynchronously send a message.
  template <typename... Args,
      BOOST_ASIO_COMPLETION_TOKEN_FOR(void (boost::system::error_code))
        CompletionToken BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(executor_type)>
  auto async_send(BOOST_ASIO_MOVE_ARG(Args)... args,
      BOOST_ASIO_MOVE_ARG(CompletionToken) token);

#endif // defined(GENERATING_DOCUMENTATION)

  /// Try to receive a message without blocking.
  /**
   * Fails if the buffer is full and there are no waiting receive operations.
   *
   * @returns @c true on success, @c false on failure.
   */
  template <typename Handler>
  bool try_receive(BOOST_ASIO_MOVE_ARG(Handler) handler)
  {
    return service_->try_receive(impl_, BOOST_ASIO_MOVE_CAST(Handler)(handler));
  }

  /// Asynchronously receive a message.
  template <typename CompletionToken
      BOOST_ASIO_DEFAULT_COMPLETION_TOKEN_TYPE(executor_type)>
  auto async_receive(
      BOOST_ASIO_MOVE_ARG(CompletionToken) token
        BOOST_ASIO_DEFAULT_COMPLETION_TOKEN(Executor))
#if !defined(GENERATING_DOCUMENTATION)
    -> decltype(
        this->do_async_receive(static_cast<payload_type*>(0),
          BOOST_ASIO_MOVE_CAST(CompletionToken)(token)))
#endif // !defined(GENERATING_DOCUMENTATION)
  {
    return this->do_async_receive(static_cast<payload_type*>(0),
        BOOST_ASIO_MOVE_CAST(CompletionToken)(token));
  }

private:
  // Disallow copying and assignment.
  basic_channel(const basic_channel&) BOOST_ASIO_DELETED;
  basic_channel& operator=(const basic_channel&) BOOST_ASIO_DELETED;

  template <typename, typename, typename...>
  friend class detail::channel_send_functions;

  // Helper function to get an executor's context.
  template <typename T>
  static execution_context& get_context(const T& t,
      typename enable_if<execution::is_executor<T>::value>::type* = 0)
  {
    return boost::asio::query(t, execution::context);
  }

  // Helper function to get an executor's context.
  template <typename T>
  static execution_context& get_context(const T& t,
      typename enable_if<!execution::is_executor<T>::value>::type* = 0)
  {
    return t.context();
  }

  class initiate_async_send
  {
  public:
    typedef Executor executor_type;

    explicit initiate_async_send(basic_channel* self)
      : self_(self)
    {
    }

    executor_type get_executor() const BOOST_ASIO_NOEXCEPT
    {
      return self_->get_executor();
    }

    template <typename SendHandler>
    void operator()(BOOST_ASIO_MOVE_ARG(SendHandler) handler,
        BOOST_ASIO_MOVE_ARG(payload_type) payload) const
    {
      boost::asio::detail::non_const_lvalue<SendHandler> handler2(handler);
      self_->service_->async_send(self_->impl_,
          BOOST_ASIO_MOVE_CAST(payload_type)(payload),
          handler2.value, self_->get_executor());
    }

  private:
    basic_channel* self_;
  };

  class initiate_async_receive
  {
  public:
    typedef Executor executor_type;

    explicit initiate_async_receive(basic_channel* self)
      : self_(self)
    {
    }

    executor_type get_executor() const BOOST_ASIO_NOEXCEPT
    {
      return self_->get_executor();
    }

    template <typename ReceiveHandler>
    void operator()(BOOST_ASIO_MOVE_ARG(ReceiveHandler) handler) const
    {
      boost::asio::detail::non_const_lvalue<ReceiveHandler> handler2(handler);
      self_->service_->async_receive(self_->impl_,
          handler2.value, self_->get_executor());
    }

  private:
    basic_channel* self_;
  };

  // The service associated with the I/O object.
  service_type* service_;

  // The underlying implementation of the I/O object.
  typename service_type::template implementation_type<
      Traits, Signatures...> impl_;

  // The associated executor.
  Executor executor_;
};

} // namespace experimental
} // namespace asio
} // namespace boost

#include <boost/asio/detail/pop_options.hpp>

#endif // BOOST_ASIO_EXPERIMENTAL_BASIC_CHANNEL_HPP
