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

template<typename T>
class unique_gobj {
private:
  T* u;

  void unref() {
      if(u==nullptr)
          return;
      g_object_unref(G_OBJECT(u));
      u = nullptr;
  }

public:
  typedef T element_type;
  typedef T* pointer;
  typedef decltype(g_object_unref) deleter_type;

  constexpr unique_gobj() : u(nullptr) {}
  explicit unique_gobj(T *t) : u(t) {
      if(u != nullptr && g_object_is_floating(G_OBJECT(u))) {
          // What should we do here. Unreffing unknown objs
          // is dodgy but not unreffing runs the risk of
          // memory leaks. Currently unrefs as u is destroyed
          // when this exception is thrown.
          throw std::invalid_argument("Tried to add a floating gobject into a unique_gobj.");
      }
  }
  unique_gobj(unique_gobj &&o) { u = o.u; o.u = nullptr; }
  unique_gobj(const unique_gobj &o) = delete;
  unique_gobj& operator=(unique_gobj &o) = delete;
  ~unique_gobj() { unref(); }

  void swap(unique_gobj<T> &o) noexcept { T*tmp = u; u = o.u; o.u = tmp; }
  void reset() noexcept { unref(); }
  T* release() noexcept { T* r = u; u=nullptr; return r; }
  T* get() const noexcept { return u; }

  T& operator*() const noexcept { return *u; }
  T* operator->() const noexcept { return u; }
  explicit operator bool() const noexcept { return u != nullptr; }

  unique_gobj& operator=(unique_gobj &&o) noexcept { unref(); u = o.u; o.u = nullptr; return *this; }
  unique_gobj& operator=(nullptr_t) const noexcept(noexcept(u=nullptr)) { u = nullptr; return *this; }
  bool operator==(const unique_gobj<T> &o) const noexcept(noexcept(u == o.u)) { return u == o.u; }
  bool operator!=(const unique_gobj<T> &o) const noexcept(noexcept(u != o.u)) { return u != o.u; }
  bool operator<(const unique_gobj<T> &o) const noexcept(noexcept(u < o.u)) { return u < o.u; }
  bool operator<=(const unique_gobj<T> &o) const noexcept(noexcept(u <= o.u)) { return u <= o.u; }
  bool operator>(const unique_gobj<T> &o) const noexcept(noexcept(u > o.u)) { return u > o.u; }
  bool operator>=(const unique_gobj<T> &o) const noexcept(noexcept(u >= o.u)) { return u >= o.u; }
};

template<typename T>
void ::std::swap(unique_gobj<T> &f, unique_gobj<T> &s) noexcept { f.swap(s); }


#endif
