#include "../src/open_world/finite_hold.h"

#include <iostream>
#include <limits>
#include <stdexcept>

using doax2::open_world::FiniteScriptHold;
using doax2::open_world::HoldAction;
void Check(bool value) {
  if (!value) throw std::runtime_error("Finite hold check failed");
}
int main() {
  try {
    using doax2::open_world::ActorTrackCanHold;
    Check(ActorTrackCanHold(30, 990));
    Check(!ActorTrackCanHold(988, 990));
    Check(!ActorTrackCanHold(0, 990));
    Check(!ActorTrackCanHold(30, 0));
    Check(!ActorTrackCanHold(std::numeric_limits<float>::quiet_NaN(), 990));
    Check(!ActorTrackCanHold(std::numeric_limits<float>::infinity(), 990));
    using doax2::open_world::CameraTrackCanHold;
    using doax2::open_world::CameraGateCanHold;
    Check(CameraTrackCanHold(3, 90, 30, 60));
    Check(!CameraTrackCanHold(3, 90, 89, 60));
    Check(!CameraTrackCanHold(3, 90, 0, 60));
    Check(!CameraTrackCanHold(3, 90, 30, 2));
    Check(!CameraTrackCanHold(0, 90, 30, 60));
    Check(!CameraTrackCanHold(9, 90, 30, 60));
    Check(!CameraTrackCanHold(3, 4097, 30, 60));
    Check(!CameraTrackCanHold(3, 90, std::numeric_limits<float>::quiet_NaN(), 60));
    Check(!CameraTrackCanHold(3, 90, 30, std::numeric_limits<float>::infinity()));
    Check(CameraGateCanHold(60));
    Check(CameraGateCanHold(-1));
    Check(!CameraGateCanHold(2));
    Check(!CameraGateCanHold(0));
    Check(!CameraGateCanHold(std::numeric_limits<float>::quiet_NaN()));
    FiniteScriptHold hold;
    Check(hold.Update(false, 0, 100).action == HoldAction::Idle);
    auto decision = hold.Update(true, 30, 101);
    Check(decision.action == HoldAction::Begin && decision.frame == 30);
    // Uneven callback intervals must retain the anchor and use host time.
    for (double now : {101.001, 101.5, 103.4, 105.999}) {
      decision = hold.Update(true, 31, now);
      Check(decision.action == HoldAction::Hold && decision.frame == 30);
    }
    decision = hold.Update(true, 31, 106);
    Check(decision.action == HoldAction::Release && decision.seconds == 5);
    Check(hold.Update(true, 30, 107).action == HoldAction::Idle);
    // Losing scene ownership ends this experiment and prevents a later retry.
    FiniteScriptHold longer;
    Check(longer.Update(true, 30, 100, 15).action == HoldAction::Begin);
    Check(longer.Update(true, 31, 105).action == HoldAction::Hold);
    Check(longer.Update(true, 31, 114.999).action == HoldAction::Hold);
    Check(longer.Update(true, 31, 115).action == HoldAction::Release);
    Check(longer.Update(true, 30, 116, 15).action == HoldAction::Idle);
    FiniteScriptHold longerExit;
    longerExit.Update(true, 30, 0, 15);
    Check(longerExit.Update(false, 31, 3).action == HoldAction::Release);
    Check(longerExit.Update(true, 30, 4, 15).action == HoldAction::Idle);
    FiniteScriptHold movement;
    Check(movement.Update(true, 30, 100, 120).action == HoldAction::Begin);
    Check(movement.Update(true, 31, 115).action == HoldAction::Hold);
    Check(movement.Update(true, 31, 219.999).action == HoldAction::Hold);
    Check(movement.Update(true, 31, 220).action == HoldAction::Release);
    Check(movement.Update(true, 30, 221, 120).action == HoldAction::Idle);
    FiniteScriptHold movementExit;
    Check(movementExit.Update(true, 30, 0, 120).action == HoldAction::Begin);
    Check(movementExit.Update(false, 31, 20).action == HoldAction::Release);
    FiniteScriptHold exploration;
    Check(exploration.Update(true,30,0,300).action == HoldAction::Begin);
    Check(exploration.Update(true,31,120).action == HoldAction::Hold);
    Check(exploration.Update(true,31,299.999).action == HoldAction::Hold);
    Check(exploration.Update(true,31,300).action == HoldAction::Release);
    for (double duration : {0.0, -1.0, 300.001, std::numeric_limits<double>::infinity(),
                            std::numeric_limits<double>::quiet_NaN()}) {
      FiniteScriptHold rejected;
      Check(rejected.Update(true, 30, 0, duration).action == HoldAction::Idle);
    }
    FiniteScriptHold cancelled;
    Check(cancelled.Update(true, 40, 0).action == HoldAction::Begin);
    Check(cancelled.Update(false, 41, 0.1).action == HoldAction::Release);
    Check(cancelled.Update(true, 40, 0.2).action == HoldAction::Idle);
    // A delayed callback releases immediately, without adding another hold.
    FiniteScriptHold delayed;
    delayed.Update(true, 30, 10);
    Check(delayed.Update(true, 31, 30).action == HoldAction::Release);
    FiniteScriptHold invalid;
    Check(invalid.Update(true, std::numeric_limits<float>::quiet_NaN(), 0).action == HoldAction::Idle);
    Check(invalid.Update(true, 30, 2).action == HoldAction::Begin);
    Check(invalid.Update(true, 31, 1).action == HoldAction::Release);
    FiniteScriptHold nonfinite;
    nonfinite.Update(true, 30, 0);
    Check(nonfinite.Update(true, 31, std::numeric_limits<double>::infinity()).action == HoldAction::Release);
    std::cout << "Finite hold checks passed\n";
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
