/*
 * Copyright (C) 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Jussi Pakkanen <jussi.pakkanen@canonical.com>
 */

#ifndef GOBJ_MEMORY_H_
#define GOBJ_MEMORY_H_

#include<glib-object.h>
#include<stdexcept>

/**
 * This class is meant for automatically managing the lifetime of C objects derived
 * from gobject. Its API perfectly mirrors the API of unique_ptr except that you
 * can't define your own deleter function as it is always g_object_unref.
 *
 * API/ABI stability is not guaranteed. If you need to pass the object across an ABI
 * boundary, pass the plain gobject.
 *
 * This is how you would use unique_gobj 99% of the time:
 *
 * unique_gobj<GSomeType> o(g_some_type_new(...));
 *
 * More specifically, the object will decrement the gobject reference count
 * of the object it points to when it goes out of scope. It will never increment it.
 * Thus you should only assign to it when already holding a reference. unique_gobj
 * will then take ownership of that particular reference.
 *
 * Floating gobjects can not be put in this container as they are meant to be put
 * into native gobject aware containers immediately upon construction. Trying to insert
 * a floating gobject into a unique_gobj will throw an invalid_argument exception. To
 * prevent accidental memory leaks, the floating gobject is unreffed in this case.
 */
template<typename T>
class unique_gobj final {
private:
  T* u;

  void validate_float(T *t) {
      if(t != nullptr && g_object_is_floating(G_OBJECT(t))) {
          throw std::invalid_argument("Tried to add a floating gobject into a unique_gobj.");
      }
  }

public:
  typedef T element_type;
  typedef T* pointer;
  typedef decltype(g_object_unref) deleter_type;

  constexpr unique_gobj() noexcept : u(nullptr) {}
  explicit unique_gobj(T *t) : u(t) {
      // What should we do if validate throws? Unreffing unknown objs
      // is dodgy but not unreffing runs the risk of
      // memory leaks. Currently unrefs as u is destroyed
      // when this exception is thrown.
      validate_float(t);
  }
  constexpr unique_gobj(nullptr_t) noexcept : u(nullptr) {};
  unique_gobj(unique_gobj &&o) noexcept { u = o.u; o.u = nullptr; }
  unique_gobj(const unique_gobj &o) = delete;
  unique_gobj& operator=(unique_gobj &o) = delete;
  ~unique_gobj() { reset(); }

  deleter_type& get_deleter() noexcept;
  const deleter_type& get_deleter() const noexcept;

  void swap(unique_gobj<T> &o) noexcept { T*tmp = u; u = o.u; o.u = tmp; }
  void reset(pointer p = pointer()) {
      if(u!=nullptr) {
          g_object_unref(G_OBJECT(u));
          u = nullptr;
      }
      // Same throw dilemma as in pointer constructor.
      u = p;
      validate_float(p);
  }

  T* release() noexcept { T* r = u; u=nullptr; return r; }
  T* get() const noexcept { return u; }

  T& operator*() const { return *u; }
  T* operator->() const noexcept { return u; }
  explicit operator bool() const noexcept { return u != nullptr; }

  unique_gobj& operator=(unique_gobj &&o) noexcept { reset(); u = o.u; o.u = nullptr; return *this; }
  unique_gobj& operator=(nullptr_t) noexcept { reset(); return *this; }
  bool operator==(const unique_gobj<T> &o) const noexcept { return u == o.u; }
  bool operator!=(const unique_gobj<T> &o) const noexcept { return u != o.u; }
  bool operator<(const unique_gobj<T> &o) const noexcept { return u < o.u; }
  bool operator<=(const unique_gobj<T> &o) const noexcept { return u <= o.u; }
  bool operator>(const unique_gobj<T> &o) const noexcept { return u > o.u; }
  bool operator>=(const unique_gobj<T> &o) const noexcept { return u >= o.u; }
};

template<typename T>
void ::std::swap(unique_gobj<T> &f, unique_gobj<T> &s) noexcept { f.swap(s); }


#endif
