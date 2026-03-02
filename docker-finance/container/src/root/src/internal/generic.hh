// docker-finance | modern accounting for the power-user
//
// Copyright (C) 2021-2025 Aaron Fiore (Founder, Evergreen Crypto LLC)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

//! \file
//! \author Aaron Fiore (Founder, Evergreen Crypto LLC)
//! \note File intended to be loaded into ROOT.cern framework / Cling interpreter
//! \since docker-finance 1.0.0

#ifndef CONTAINER_SRC_ROOT_SRC_INTERNAL_GENERIC_HH_
#define CONTAINER_SRC_ROOT_SRC_INTERNAL_GENERIC_HH_

#include <algorithm>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "./type.hh"

//! \namespace dfi
//! \since docker-finance 1.0.0
namespace dfi
{
//! \namespace dfi::internal
//! \brief For internal use only
//! \since docker-finance 1.0.0
namespace internal
{
//! \brief Implementation base class pointer for CRTP
//! \ingroup cpp_generic
//! \since docker-finance 1.0.0
template <typename t_derived>
class ImplBase
{
 public:
  ImplBase() = default;
  virtual ~ImplBase() = default;

  ImplBase(const ImplBase&) = default;
  ImplBase& operator=(const ImplBase&) = default;

  ImplBase(ImplBase&&) = default;
  ImplBase& operator=(ImplBase&&) = default;

 public:
  virtual const t_derived* that() const = 0;
  virtual t_derived* that() = 0;
};

//! \brief CRTP for memory-allocator container-aware data type transformer
//! \ingroup cpp_generic
//! \details More than meets the eye
//! \since docker-finance 1.0.0
template <typename t_impl>
class Transform : private ImplBase<t_impl>
{
  static_assert(std::is_class_v<t_impl>);

 public:
  Transform() = default;
  ~Transform() = default;

  Transform(Transform&&) = default;
  Transform& operator=(Transform&&) = default;

  //! \brief Thread-safe copy-ctor
  //! \note Privately implemented
  Transform(const Transform& transform)
      : Transform(transform, std::scoped_lock<std::mutex>(transform.m_mutex))
  {
  }

  //! \brief Thread-safe copy-assignment
  Transform& operator=(const Transform& transform)
  {
    std::scoped_lock lock(m_mutex, transform.m_mutex);
    // copy data as needed
    return *this;
  }

 private:
  //! \brief Thread-safe copy-ctor implementation
  Transform(
      __attribute__((unused)) const Transform& transform,
      const std::scoped_lock<std::mutex>&)
  {
    // copy data as needed
  }

 protected:
  //! \brief Implements const pointer to CRTP-derived (implementation)
  const t_impl* that() const override
  {
    static const auto* const that = static_cast<const t_impl*>(this);
    return that;
  }

  //! \brief Implements pointer to CRTP-derived (implementation)
  t_impl* that() override
  {
    static auto* that = static_cast<t_impl*>(this);
    return that;
  }

 private:
  //! \brief Generic transform encoder
  //! \details CRTP-derived implementation hook
  //! \todo Don't pass reference for trivial types
  template <typename t_tag, typename t_decoded, typename t_encoded>
  t_encoded encoder(t_decoded arg)  // TODO(unassigned): const ref?
  {
    std::scoped_lock lock(m_mutex);
    return that()->template encoder_impl<t_tag, t_decoded, t_encoded>(arg);
  }

 public:
  //! \brief Encode trivial types and std::string_view
  //! \todo std::string_view is trivially copyable in c++23 but `dfi` ROOT build is c++20
  template <
      typename t_tag,
      typename t_encoded,
      typename t_decoded,
      std::enable_if_t<type::is_signature_with_trivial<t_decoded>, bool> = true>
  t_encoded encode(const t_decoded arg)
  {
    return this->encoder<t_tag, t_decoded, t_encoded>(arg);
  }

  //! \brief Encode std::string container
  template <
      typename t_tag,
      typename t_encoded,
      template <typename...> typename t_container,
      typename... t_args,
      std::enable_if_t<
          type::is_signature_with_string<t_container<t_args...>>,
          bool> = true>  // cppcheck-suppress syntaxError
  t_encoded encode(const t_container<t_args...>& arg)
  {
    return this->encoder<t_tag, t_container<t_args...>, t_encoded>(arg);
  }

  //! \brief Encode allocator-aware non-associative containers
  //! \return Encoded container copy
  //! \note string types are specialized elsewhere
  //! \todo expand to all basic_string/basic_string_view types?
  template <
      typename t_tag,
      typename t_encoded,
      template <typename...> typename t_container,
      typename... t_args,
      std::enable_if_t<
          type::is_signature_with_value_type_allocator<t_container<t_args...>>,
          bool> = true>
  t_container<t_encoded> encode(const t_container<t_args...>& arg)
  {
    using t_value = typename t_container<t_args...>::value_type;

    t_container<t_encoded> ret{};

    if constexpr (
        // TODO(afiore): c++20 forward iterator tag
        std::is_same_v<t_container<t_value>, std::forward_list<t_value>>)
      {
        std::for_each(
            arg.cbegin(), arg.cend(), [this, &ret](const auto& value) {
              ret.push_front(this->encoder<t_tag, t_value, t_encoded>(value));
            });
        ret.reverse();
      }
    else if constexpr (
        // TODO(afiore): c++20 bidirectional iterator tag
        std::is_same_v<t_container<t_value>, std::set<t_value>>
        || std::is_same_v<t_container<t_value>, std::multiset<t_value>>
        || std::is_same_v<t_container<t_value>, std::unordered_set<t_value>>
        || std::
            is_same_v<t_container<t_value>, std::unordered_multiset<t_value>>)
      {
        std::for_each(
            arg.cbegin(), arg.cend(), [this, &ret](const auto& value) {
              ret.insert(this->encoder<t_tag, t_value, t_encoded>(value));
            });
      }
    else
      {
        // TODO(afiore): c++20 random access / legacy contiguous iterator tag
        std::for_each(
            arg.cbegin(), arg.cend(), [this, &ret](const auto& value) {
              ret.push_back(this->encoder<t_tag, t_value, t_encoded>(value));
            });
      }

    return ret;
  }

  //! \brief Encode allocator-aware associative containers
  //! \return Encoded container copy
  //! \since docker-finance 1.0.0
  template <
      typename t_tag,
      typename t_encoded,
      template <typename...> typename t_container,
      typename... t_args,
      std::enable_if_t<
          type::is_signature_with_mapped_type_allocator<t_container<t_args...>>,
          bool> = true>
  t_container<typename t_container<t_args...>::key_type, t_encoded> encode(
      const t_container<t_args...>& arg)
  {
    using t_key = typename t_container<t_args...>::key_type;
    using t_mapped = typename t_container<t_args...>::mapped_type;

    t_container<t_key, t_encoded> ret{};

    std::for_each(arg.cbegin(), arg.cend(), [this, &ret](const auto& value) {
      ret.insert(
          {value.first,
           this->encoder<t_tag, t_mapped, t_encoded>(value.second)});
    });

    return ret;
  }

 protected:
  //! \brief Base-type mutex
  //! \note Could be used by a derived class
  mutable std::mutex m_mutex;
};

//! \brief CRTP for random number generation
//! \ingroup cpp_generic
//! \since docker-finance 1.0.0
template <typename t_impl>
class Random : private ImplBase<t_impl>
{
  static_assert(std::is_class_v<t_impl>);

 public:
  Random() = default;
  ~Random() = default;

  Random(Random&&) = default;
  Random& operator=(Random&&) = default;

  //! \brief Thread-safe copy-ctor
  //! \note Privately implemented
  Random(const Random& random)
      : Random(random, std::scoped_lock<std::mutex>(random.m_mutex))
  {
  }

  //! \brief Thread-safe copy-assignment
  Random& operator=(const Random& random)
  {
    std::scoped_lock lock(m_mutex, random.m_mutex);
    // copy data as needed
    return *this;
  }

 private:
  //! \brief Thread-safe copy-ctor implementation
  Random(
      __attribute__((unused)) const Random& random,
      const std::scoped_lock<std::mutex>&)
  {
    // copy data as needed
  }

 protected:
  //! \brief Implements const pointer to CRTP-derived (implementation)
  const t_impl* that() const override
  {
    static const auto* const that = static_cast<const t_impl*>(this);
    return that;
  }

  //! \brief Implements pointer to CRTP-derived (implementation)
  t_impl* that() override
  {
    static auto* that = static_cast<t_impl*>(this);
    return that;
  }

 public:
  //! \brief generate random number with ceiling of template type
  template <typename t_random>
  t_random generate()
  {
    static_assert(type::is_real_integral<t_random>::value);

    std::scoped_lock lock(m_mutex);
    return that()->template generate_impl<t_random>();
  }

 protected:
  //! \brief Base-type mutex
  //! \note Could be used by a derived class
  mutable std::mutex m_mutex;
};

}  // namespace internal
}  // namespace dfi

#endif  // CONTAINER_SRC_ROOT_SRC_INTERNAL_GENERIC_HH_

// # vim: sw=2 sts=2 si ai et
