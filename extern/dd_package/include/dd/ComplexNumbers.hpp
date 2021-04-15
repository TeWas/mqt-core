/*
 * This file is part of the JKQ DD Package which is released under the MIT license.
 * See file README.md or go to http://iic.jku.at/eda/research/quantum_dd/ for more information.
 */

#ifndef DDcomplex_H
#define DDcomplex_H

#include "Complex.hpp"
#include "ComplexCache.hpp"
#include "ComplexTable.hpp"
#include "ComplexValue.hpp"
#include "Definitions.hpp"

#include <cassert>
#include <cmath>
#include <cstdlib>

namespace dd {
    struct ComplexNumbers {
        ComplexTable<> complexTable{};
        ComplexCache<> complexCache{};

        ComplexNumbers()  = default;
        ~ComplexNumbers() = default;

        void clear() {
            complexTable.clear();
            complexCache.clear();
        }

        static void setTolerance(fp tol) {
            ComplexTable<>::setTolerance(tol);
        }

        // operations on complex numbers
        // meanings are self-evident from the names
        static void add(Complex& r, const Complex& a, const Complex& b) {
            assert(r != Complex::zero);
            assert(r != Complex::one);
            r.r->value = a.r->val() + b.r->val();
            r.i->value = a.i->val() + b.i->val();
        }
        static void sub(Complex& r, const Complex& a, const Complex& b) {
            assert(r != Complex::zero);
            assert(r != Complex::one);
            r.r->value = a.r->val() - b.r->val();
            r.i->value = a.i->val() - b.i->val();
        }
        static void mul(Complex& r, const Complex& a, const Complex& b) {
            assert(r != Complex::zero);
            assert(r != Complex::one);
            if (a.approximatelyOne()) {
                r.setVal(b);
            } else if (b.approximatelyOne()) {
                r.setVal(a);
            } else if (a.approximatelyZero() || b.approximatelyZero()) {
                r.r->value = 0.;
                r.i->value = 0.;
            } else {
                auto ar = a.r->val();
                auto ai = a.i->val();
                auto br = b.r->val();
                auto bi = b.i->val();

                r.r->value = ar * br - ai * bi;
                r.i->value = ar * bi + ai * br;
            }
        }
        static void div(Complex& r, const Complex& a, const Complex& b) {
            assert(r != Complex::zero);
            assert(r != Complex::one);
            if (a.approximatelyEquals(b)) {
                r.r->value = 1.;
                r.i->value = 0.;
            } else if (a.approximatelyZero()) {
                r.r->value = 0.;
                r.i->value = 0.;
            } else if (b.approximatelyOne()) {
                r.setVal(a);
            } else {
                auto ar = a.r->val();
                auto ai = a.i->val();
                auto br = b.r->val();
                auto bi = b.i->val();

                auto cmag = br * br + bi * bi;

                r.r->value = (ar * br + ai * bi) / cmag;
                r.i->value = (ai * br - ar * bi) / cmag;
            }
        }
        static inline fp mag2(const Complex& a) {
            auto ar = a.r->val();
            auto ai = a.i->val();

            return ar * ar + ai * ai;
        }
        static inline fp mag(const Complex& a) {
            return std::sqrt(mag2(a));
        }
        static inline fp arg(const Complex& a) {
            auto ar = a.r->val();
            auto ai = a.i->val();
            return std::atan2(ai, ar);
        }
        static Complex conj(const Complex& a) {
            auto ret = a;
            if (a.i != Complex::zero.i) {
                ret.i = a.i->flipPointerSign();
            }
            return ret;
        }
        static Complex neg(const Complex& a) {
            auto ret = a;
            if (a.i != Complex::zero.i) {
                ret.i = a.i->flipPointerSign();
            }
            if (a.r != Complex::zero.i) {
                ret.r = a.r->flipPointerSign();
            }
            return ret;
        }

        inline Complex addCached(const Complex& a, const Complex& b) {
            auto c = getCached();
            add(c, a, b);
            return c;
        }

        inline Complex subCached(const Complex& a, const Complex& b) {
            auto c = getCached();
            sub(c, a, b);
            return c;
        }

        inline Complex mulCached(const Complex& a, const Complex& b) {
            auto c = getCached();
            mul(c, a, b);
            return c;
        }

        inline Complex divCached(const Complex& a, const Complex& b) {
            auto c = getCached();
            div(c, a, b);
            return c;
        }

        // lookup a complex value in the complex table; if not found add it
        Complex lookup(const Complex& c) {
            if (c == Complex::zero) {
                return Complex::zero;
            }
            if (c == Complex::one) {
                return Complex::one;
            }

            auto valr = c.r->val();
            auto vali = c.i->val();
            return lookup(valr, vali);
        }
        Complex lookup(const fp& r, const fp& i) {
            Complex ret{};

            auto sign_r = std::signbit(r);
            if (sign_r) {
                auto absr = std::abs(r);
                ret.r     = complexTable.lookup(absr)->getNegativePointer();
            } else {
                ret.r = complexTable.lookup(r);
            }

            auto sign_i = std::signbit(i);
            if (sign_i) {
                auto absi = std::abs(i);
                ret.i     = complexTable.lookup(absi)->getNegativePointer();
            } else {
                ret.i = complexTable.lookup(i);
            }

            return ret;
        }
        inline Complex lookup(const ComplexValue& c) { return lookup(c.r, c.i); }

        // reference counting and garbage collection
        static void incRef(const Complex& c) {
            // `zero` and `one` are static and never altered
            if (c != Complex::zero && c != Complex::one) {
                ComplexTable<>::incRef(c.r);
                ComplexTable<>::incRef(c.i);
            }
        }
        static void decRef(const Complex& c) {
            // `zero` and `one` are static and never altered
            if (c != Complex::zero && c != Complex::one) {
                ComplexTable<>::decRef(c.r);
                ComplexTable<>::decRef(c.i);
            }
        }
        std::size_t garbageCollect(bool force = false) {
            return complexTable.garbageCollect(force);
        }

        // provide (temporary) cached complex number
        inline Complex getTemporary() {
            return complexCache.getTemporaryComplex();
        }

        inline Complex getTemporary(const fp& r, const fp& i) {
            auto c     = complexCache.getTemporaryComplex();
            c.r->value = r;
            c.i->value = i;
            return c;
        }

        inline Complex getTemporary(const ComplexValue& c) {
            return getTemporary(c.r, c.i);
        }

        inline Complex getCached() {
            return complexCache.getCachedComplex();
        }

        inline Complex getCached(const fp& r, const fp& i) {
            auto c     = complexCache.getCachedComplex();
            c.r->value = r;
            c.i->value = i;
            return c;
        }

        inline Complex getCached(const ComplexValue& c) {
            return getCached(c.r, c.i);
        }

        void returnToCache(Complex& c) {
            complexCache.returnToCache(c);
        }

        [[nodiscard]] std::size_t cacheCount() const {
            return complexCache.getCount();
        }
    };
} // namespace dd
#endif
