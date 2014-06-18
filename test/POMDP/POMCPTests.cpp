#define BOOST_TEST_MODULE POMDP_POMCP
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <AIToolbox/POMDP/Algorithms/IncrementalPruning.hpp>
#include <AIToolbox/POMDP/Algorithms/POMCP.hpp>
#include <AIToolbox/POMDP/Types.hpp>
#include <AIToolbox/POMDP/Policies/Policy.hpp>
#include <AIToolbox/POMDP/Utils.hpp>

#include <AIToolbox/ProbabilityUtils.hpp>

#include "TigerProblem.hpp"

BOOST_AUTO_TEST_CASE( discountedHorizon ) {
    using namespace AIToolbox;

    auto model = setupTigerProblem();
    model.setDiscount(0.85);

    // This indicates where the tiger is.
    std::vector<POMDP::Belief> beliefs{{0.5, 0.5}, {1.0, 0.0}, {0.25, 0.75}, {0.98, 0.02}, {0.33, 0.66}};

    unsigned maxHorizon = 7;

    // Compute theoretical solution. Since the tiger problem can be actually
    // solved in multiple ways with certain discounts, I chose a discount factor
    // that seems to work, although this is in no way substantiated with theory.
    // If there's a better way to test POMCP please let me know.
    POMDP::IncrementalPruning groundTruth(maxHorizon);
    auto solution = groundTruth(model);
    auto & vf = std::get<1>(solution);
    POMDP::Policy p(model.getS(), model.getA(), model.getO(), vf);

    for ( unsigned horizon = 1; horizon < maxHorizon; ++horizon ) {
        // Again, the exploration constant has been chosen to let the solver agree with
        // the ground truth rather than not. A lower constant results in LISTEN actions
        // being swapped for OPEN actions. This still could be due to the fact that in some
        // cases listening now vs opening later really does not change much.
        // The main problem is that the high exploration constant here is used to force
        // OPEN actions in high uncertainty situations, in any case. Otherwise, LISTEN actions
        // end up being way better, since POMCP averages across actions (not very smart).
        POMDP::POMCP<decltype(model)> solver(model, 1000, 10000, horizon * 100.0);

        for ( auto & b : beliefs ) {
            auto a = solver.sampleAction(b, horizon);
            auto trueA = p.sampleAction(b, horizon);

            BOOST_CHECK_EQUAL( std::get<0>(trueA), a);
        }
    }
}
