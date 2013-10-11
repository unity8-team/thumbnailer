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

#include<memory>
#include<glib-object.h>

template<typename T>
void gobj_unref(T *t) { g_object_unref(G_OBJECT(t)); }

template<typename T>
class unique_gobj {
private:
  std::unique_ptr<T, void(*)(T*)> u;

public:
  typedef T element_type;
  typedef T* pointer;
  typedef decltype(gobj_unref<T>) deleter_type;

  constexpr unique_gobj() : u(nullptr, gobj_unref<T>) {}
  explicit unique_gobj(T *t) : u(t, gobj_unref<T>) {}
  unique_gobj(unique_gobj &&o) noexcept = default;
  unique_gobj(const unique_gobj &o) = delete;
  unique_gobj& operator=(unique_gobj &o) = delete;

  void swap(unique_gobj<T> &o) noexcept { u.swap(o.u); }
  void reset() noexcept { u.reset(); }
  T* release() noexcept { return u.release(); }
  T* get() const noexcept { return u.get(); }

  T& operator*() noexcept { return *u; }
  T* operator->() noexcept(noexcept(u.get)) { return u.get(); }
  explicit operator bool() noexcept(noexcept(bool(u))) { return bool(u); }

  unique_gobj& operator=(unique_gobj &&o) noexcept(noexcept(u = o)) { u = o; return *this; }
  unique_gobj& operator=(nullptr_t) noexcept(noexcept(u=nullptr)) { u = nullptr; return *this; }
  bool operator==(const unique_gobj<T> &o) noexcept(noexcept(u == o.u)) { return u == o.u; }
  bool operator!=(const unique_gobj<T> &o) noexcept(noexcept(u != o.u)) { return u != o.u; }
  bool operator<(const unique_gobj<T> &o) noexcept(noexcept(u < o.u)) { return u < o.u; }
  bool operator<=(const unique_gobj<T> &o) noexcept(noexcept(u <= o.u)) { return u <= o.u; }
  bool operator>(const unique_gobj<T> &o) noexcept(noexcept(u > o.u)) { return u > o.u; }
  bool operator>=(const unique_gobj<T> &o) noexcept(noexcept(u >= o.u)) { return u >= o.u; }
};

template<typename T>
void ::std::swap(unique_gobj<T> &f, unique_gobj<T> &s) noexcept { f.swap(s); }


#endif
