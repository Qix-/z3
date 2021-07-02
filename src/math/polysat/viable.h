/*++
Copyright (c) 2021 Microsoft Corporation

Module Name:

    maintain viable domains


Author:

    Nikolaj Bjorner (nbjorner) 2021-03-19
    Jakob Rath 2021-04-6

Notes:

    NEW_VIABLE uses cheaper book-keeping, but is partial.
    
--*/
#pragma once

#define NEW_VIABLE 0

#include <limits>

#include "math/dd/dd_bdd.h"
#include "math/polysat/types.h"
#include "math/interval/mod_interval.h"


namespace polysat {

    class solver;

#if NEW_VIABLE
    //
    // replace BDDs by viable sets that emulate affine relations.
    // viable_set has an interval of feasible values.
    // it also can use ternary bit-vectors.
    // or we could also just use a vector of lbool instead of ternary bit-vectors
    // updating them at individual positions is relatively cheap instead of copying the
    // vectors every time a range is narrowed.
    //
    class viable_set : public mod_interval<rational> {
        unsigned m_num_bits;
        rational p2() const { return rational::power_of_two(m_num_bits); }
        bool is_max(rational const& a) const override;
        void intersect_eq(rational const& a, bool is_positive);
        bool narrow(std::function<bool(rational const&)>& eval);
    public:
        viable_set(unsigned num_bits): m_num_bits(num_bits) {}
        ~viable_set() override {}
        dd::find_t find_hint(rational const& c, rational& val) const;
        bool intersect_eq(rational const& a, rational const& b, bool is_positive);
        bool intersect_le(rational const& a, rational const& b, rational const& c, rational const& d, bool is_positive);
        rational prev(rational const& p) const;
    };
#endif

    class viable {
        typedef dd::bdd bdd;
        typedef dd::fdd fdd;
        solver&                      s;
        dd::bdd_manager              m_bdd;
        scoped_ptr_vector<dd::fdd>   m_bits;
#if NEW_VIABLE
        struct cached_constraint {
            enum op_code { is_ule, is_eq };
            op_code  m_op;
            unsigned m_num_bits;
            rational a, b, c, d;
            bdd      repr;
            unsigned m_activity = 0;
            cached_constraint(unsigned n, rational const& a, rational const& b, rational const& c, rational const& d, bdd& f) :
                m_op(op_code::is_ule), m_num_bits(n), a(a), b(b), c(c), d(d), repr(f) {}
            cached_constraint(unsigned n, rational const& a, rational const& b, bdd& f) :
                m_op(op_code::is_eq), m_num_bits(n), a(a), b(b), repr(f) {}

            struct hash {
                unsigned operator()(cached_constraint const* e) const {
                    return mk_mix(e->a.hash(), e->b.hash(), mk_mix(e->c.hash(), e->d.hash(), e->m_num_bits)) + e->m_op;
                }
            };
            struct eq {
                bool operator()(cached_constraint const* x, cached_constraint const* y) const {
                    return x->m_op == y->m_op && x->a == y->a && x->b == y->b && x->c == y->c && x->d == y->d && x->m_num_bits == y->m_num_bits;
                }
            };
        };
        vector<viable_set>                                       m_viable; 
        vector<std::pair<pvar, viable_set>>                      m_viable_trail;
        hashtable<cached_constraint*, cached_constraint::hash, cached_constraint::eq> m_constraint_cache;

        void intersect_ule_bdd(pvar v, rational const& a, rational const& b, rational const& c, rational const& d, bool is_positive);
        void intersect_eq_bdd(pvar v, rational const& a, rational const& b, bool is_positive);
        cached_constraint& cache_constraint(pvar v, cached_constraint& entry0, std::function<bdd(void)>& mk_constraint);
        void gc_cached_constraints();
        void narrow(pvar v, bdd const& is_false);
#else

        
        vector<bdd>                  m_viable;   // set of viable values.
        vector<std::pair<pvar, bdd>> m_viable_trail;


        /**
         * Register all values that are not contained in vals as non-viable.
         */
        void intersect_viable(pvar v, bdd vals);
#endif
        dd::bdd_manager& get_bdd() { return m_bdd; }
        dd::fdd const& sz2bits(unsigned sz);
        dd::fdd const& var2bits(pvar v);


    public:
        viable(solver& s);

        ~viable();

        void push(unsigned num_bits) { 
#if NEW_VIABLE
            m_viable.push_back(viable_set(num_bits)); 
#else
            m_viable.push_back(m_bdd.mk_true()); 
#endif
        }

        void pop() { 
            m_viable.pop_back(); 
        }

        void push_viable(pvar v);

        void pop_viable();

        /**
         * update state of viable for pvar v
         * based on affine constraints
         */

        void intersect_eq(rational const& a, pvar v, rational const& b, bool is_positive);

        void intersect_ule(pvar v, rational const& a, rational const& b, rational const& c, rational const& d, bool is_positive);

        /**
         * Check whether variable v has any viable values left according to m_viable.
         */
        bool has_viable(pvar v);

        bool is_false(pvar v) { return !has_viable(v); }

        /**
         * check if value is viable according to m_viable.
         */
        bool is_viable(pvar v, rational const& val);

        /**
         * register that val is non-viable for var.
         */
        void add_non_viable(pvar v, rational const& val);


        /**
         * Find a next viable value for variable.
         */
        dd::find_t find_viable(pvar v, rational & val);

        /** Log all viable values for the given variable.
         * (Inefficient, but useful for debugging small instances.)
         */
        void log(pvar v);

        /** Like log(v) but for all variables */
        void log();

    };
}


